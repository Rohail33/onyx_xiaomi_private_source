// SPDX-License-Identifier: GPL-2.0
/*
 *
 * Original module metadata:
 *   author:      huangqibo <huangqibo@xiaomi.com>
 *   description: exception log transfer.
 *   device:      /dev/miev
 *
 * This source is a clean-room compatible implementation based on the public
 * module ABI, exported symbols, strings, and observed object layout.
 */

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kfifo.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#define MIEV_DEVICE_NAME		"miev"
#define MIEV_CLASS_NAME			"miev"
#define MIEV_FIFO_SIZE			(128 * 1024)
#define MIEV_MAX_EVENT_SIZE		2048
#define MIEV_MAX_PARAM_NAME		64
#define MIEV_MAX_PARAM_STRING		512
#define MIEV_MAX_PARAMS			64

struct misight_mievent {
	int event_id;
	int para_cnt;
	size_t used_size;
	char *msg;
};

struct miev_device {
	dev_t devt;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct mutex lock;
	wait_queue_head_t wait_queue;
	struct kfifo fifo;
};

static struct miev_device *miev_dev;
static struct workqueue_struct *miev_workqueue;

static int write_buf_inner(const char *buf, size_t len)
{
	int ret;

	if (!miev_dev || !buf || !len)
		return -EINVAL;

	mutex_lock(&miev_dev->lock);
	ret = kfifo_in(&miev_dev->fifo, buf, len);
	mutex_unlock(&miev_dev->lock);

	if (ret != len)
		return -ENOSPC;

	wake_up_interruptible(&miev_dev->wait_queue);
	return 0;
}

struct misight_mievent *cdev_tevent_alloc(unsigned int event_id)
{
	struct misight_mievent *event;
	int ret;

	event = kzalloc(sizeof(*event), GFP_KERNEL);
	if (!event)
		return NULL;

	event->msg = kzalloc(MIEV_MAX_EVENT_SIZE, GFP_KERNEL);
	if (!event->msg) {
		kfree(event);
		return NULL;
	}

	event->event_id = event_id;
	ret = scnprintf(event->msg, MIEV_MAX_EVENT_SIZE,
		       "EventId %u -t %lld -paraList {",
		       event_id, (s64)ktime_get_real_seconds());
	if (ret <= 0 || ret >= MIEV_MAX_EVENT_SIZE) {
		pr_err("miev: miev_alloc:add json head failed\n");
		kfree(event->msg);
		kfree(event);
		return NULL;
	}

	event->used_size = ret;
	return event;
}
EXPORT_SYMBOL_GPL(cdev_tevent_alloc);

int get_integer_size(long value)
{
	int size = 1;

	if (value < 0) {
		size++;
		value = -value;
	}

	while (value >= 10) {
		value /= 10;
		size++;
	}

	return size;
}

int cdev_tevent_add_int(struct misight_mievent *event, const char *key,
			long value)
{
	int ret;
	const char *fmt;

	if (!event || !event->msg || !key)
		return -EINVAL;

	if (event->para_cnt >= MIEV_MAX_PARAMS)
		return -ENOSPC;

	if (strlen(key) >= MIEV_MAX_PARAM_NAME)
		return -EINVAL;

	fmt = event->para_cnt ? ",\"%s\":%ld" : "\"%s\":%ld";
	ret = scnprintf(event->msg + event->used_size,
		       MIEV_MAX_EVENT_SIZE - event->used_size,
		       fmt, key, value);
	if (ret <= 0 || ret >= MIEV_MAX_EVENT_SIZE - event->used_size) {
		pr_err("miev: add_int:%u add %s failed, len %d\n",
		       event->event_id, key, ret);
		return -ENOSPC;
	}

	event->used_size += ret;
	event->para_cnt++;
	return 0;
}
EXPORT_SYMBOL_GPL(cdev_tevent_add_int);

int cdev_tevent_add_str(struct misight_mievent *event, const char *key,
			const char *value)
{
	int ret;
	const char *fmt;

	if (!event || !event->msg || !key || !value)
		return -EINVAL;

	if (event->para_cnt >= MIEV_MAX_PARAMS)
		return -ENOSPC;

	if (strlen(key) >= MIEV_MAX_PARAM_NAME ||
	    strlen(value) >= MIEV_MAX_PARAM_STRING)
		return -EINVAL;

	fmt = event->para_cnt ? ",\"%s\":\"%s\"" : "\"%s\":\"%s\"";
	ret = scnprintf(event->msg + event->used_size,
		       MIEV_MAX_EVENT_SIZE - event->used_size,
		       fmt, key, value);
	if (ret <= 0 || ret >= MIEV_MAX_EVENT_SIZE - event->used_size) {
		pr_err("miev: add_str:%p add %s failed\n", event, key);
		return -ENOSPC;
	}

	event->used_size += ret;
	event->para_cnt++;
	return 0;
}
EXPORT_SYMBOL_GPL(cdev_tevent_add_str);

int cdev_tevent_write(struct misight_mievent *event)
{
	int ret;

	if (!event || !event->msg)
		return -EINVAL;

	ret = scnprintf(event->msg + event->used_size,
		       MIEV_MAX_EVENT_SIZE - event->used_size, "}\n");
	if (ret <= 0 || ret >= MIEV_MAX_EVENT_SIZE - event->used_size) {
		pr_err("miev: event_write:add json end str failed %p\n", event);
		return -ENOSPC;
	}

	event->used_size += ret;
	return write_buf_inner(event->msg, event->used_size);
}
EXPORT_SYMBOL_GPL(cdev_tevent_write);

void cdev_tevent_destroy(struct misight_mievent *event)
{
	if (!event)
		return;

	kfree(event->msg);
	kfree(event);
}
EXPORT_SYMBOL_GPL(cdev_tevent_destroy);

static ssize_t miev_read(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	unsigned int copied = 0;
	int ret;

	if (!miev_dev)
		return -ENODEV;

	if (kfifo_is_empty(&miev_dev->fifo)) {
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		ret = wait_event_interruptible(miev_dev->wait_queue,
					       !kfifo_is_empty(&miev_dev->fifo));
		if (ret)
			return ret;
	}

	mutex_lock(&miev_dev->lock);
	ret = kfifo_to_user(&miev_dev->fifo, buf, count, &copied);
	mutex_unlock(&miev_dev->lock);

	return ret ? ret : copied;
}

static ssize_t miev_write(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	char *kbuf;
	int ret;

	if (!count)
		return 0;

	if (count > MIEV_MAX_EVENT_SIZE)
		return -EINVAL;

	kbuf = kzalloc(count + 1, GFP_KERNEL);
	if (!kbuf)
		return -ENOMEM;

	if (copy_from_user(kbuf, buf, count)) {
		kfree(kbuf);
		return -EFAULT;
	}

	ret = write_buf_inner(kbuf, count);
	kfree(kbuf);

	return ret ? ret : count;
}

static __poll_t miev_poll(struct file *file, poll_table *wait)
{
	__poll_t mask = 0;

	if (!miev_dev)
		return EPOLLERR;

	poll_wait(file, &miev_dev->wait_queue, wait);

	if (!kfifo_is_empty(&miev_dev->fifo))
		mask |= EPOLLIN | EPOLLRDNORM;

	return mask;
}

static long miev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return -ENOTTY;
}

static int miev_open(struct inode *inode, struct file *file)
{
	if (!try_module_get(THIS_MODULE))
		return -ENODEV;

	return 0;
}

static int miev_release(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
	return 0;
}

static const struct file_operations miev_fops = {
	.owner		= THIS_MODULE,
	.read		= miev_read,
	.write		= miev_write,
	.poll		= miev_poll,
	.unlocked_ioctl	= miev_ioctl,
	.open		= miev_open,
	.release	= miev_release,
	.llseek		= no_llseek,
};

static int __init miev_init_module(void)
{
	int ret;

	miev_dev = kzalloc(sizeof(*miev_dev), GFP_KERNEL);
	if (!miev_dev)
		return -ENOMEM;

	mutex_init(&miev_dev->lock);
	init_waitqueue_head(&miev_dev->wait_queue);

	ret = alloc_chrdev_region(&miev_dev->devt, 0, 1, MIEV_DEVICE_NAME);
	if (ret) {
		pr_err("miev: Fail to register_chrdev_region\n");
		goto err_free_dev;
	}

	cdev_init(&miev_dev->cdev, &miev_fops);
	miev_dev->cdev.owner = THIS_MODULE;

	ret = cdev_add(&miev_dev->cdev, miev_dev->devt, 1);
	if (ret) {
		pr_err("miev: Fail to add miev_dev\n");
		goto err_unregister_chrdev;
	}

	ret = kfifo_alloc(&miev_dev->fifo, MIEV_FIFO_SIZE, GFP_KERNEL);
	if (ret) {
		pr_err("miev: kfifo_alloc create failed\n");
		goto err_cdev_del;
	}

	miev_dev->class = class_create(MIEV_CLASS_NAME);
	if (IS_ERR(miev_dev->class)) {
		ret = PTR_ERR(miev_dev->class);
		pr_err("miev: class create failed!\n");
		goto err_fifo_free;
	}

	miev_dev->device = device_create(miev_dev->class, NULL, miev_dev->devt,
					 NULL, MIEV_DEVICE_NAME);
	if (IS_ERR(miev_dev->device)) {
		ret = PTR_ERR(miev_dev->device);
		pr_err("miev: device create failed!\n");
		goto err_class_destroy;
	}

	miev_workqueue = alloc_workqueue("miev_workqueue", WQ_UNBOUND, 0);
	if (!miev_workqueue) {
		ret = -ENOMEM;
		pr_err("miev: create workqueue failed\n");
		goto err_device_destroy;
	}

	pr_info("miev: initialized\n");
	return 0;

err_device_destroy:
	device_destroy(miev_dev->class, miev_dev->devt);
err_class_destroy:
	class_destroy(miev_dev->class);
err_fifo_free:
	kfifo_free(&miev_dev->fifo);
err_cdev_del:
	cdev_del(&miev_dev->cdev);
err_unregister_chrdev:
	unregister_chrdev_region(miev_dev->devt, 1);
err_free_dev:
	kfree(miev_dev);
	miev_dev = NULL;
	return ret;
}

static void __exit miev_cleanup_module(void)
{
	if (miev_workqueue) {
		flush_workqueue(miev_workqueue);
		destroy_workqueue(miev_workqueue);
		miev_workqueue = NULL;
	}

	if (!miev_dev)
		return;

	device_destroy(miev_dev->class, miev_dev->devt);
	class_destroy(miev_dev->class);
	kfifo_free(&miev_dev->fifo);
	cdev_del(&miev_dev->cdev);
	unregister_chrdev_region(miev_dev->devt, 1);
	kfree(miev_dev);
	miev_dev = NULL;
}

module_init(miev_init_module);
module_exit(miev_cleanup_module);

MODULE_AUTHOR("huangqibo <huangqibo@xiaomi.com>");
MODULE_DESCRIPTION("exception log transfer.");
MODULE_LICENSE("GPL");
