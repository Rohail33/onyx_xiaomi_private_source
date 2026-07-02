// SPDX-License-Identifier: GPL-2.0

//perf_helper.ko stock module decompiled


#include <linux/cpumask.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/uaccess.h>

#define PERF_HELPER_BUF_SIZE	1024
#define PERF_HELPER_LOG_SIZE	(PAGE_SIZE * 4)

static DEFINE_MUTEX(perflock_lock);
static DEFINE_MUTEX(mimd_lock);

static struct proc_dir_entry *perflock_dir;
static struct proc_dir_entry *mimd_dir;
static struct kobject *sched_assi_kobj;
static struct kobject *mimd_kobj;

static char perflock_records_buff[PERF_HELPER_LOG_SIZE];
static char perflock_exception_buff[PERF_HELPER_LOG_SIZE];
static char mimd_log_buff[PERF_HELPER_LOG_SIZE];
static char reclaim_buff[PERF_HELPER_BUF_SIZE];
static char kdamond_cpuset[64] = "0-7";

static bool sched_long_runnable_check;
static bool sched_long_running_check;
static int kdamond_pid = -1;
static int kswapd_pid = -1;
static int kcompactd_pid = -1;
static int sfui_pid = -1;
static int sfre_pid = -1;
static unsigned int scan_type;
static unsigned long reclaim_count;

static ssize_t perf_helper_copy_user(char *dst, size_t dst_size,
				     const char __user *buf, size_t count)
{
	size_t len = min(count, dst_size - 1);

	if (!dst || !dst_size)
		return -EINVAL;

	if (copy_from_user(dst, buf, len))
		return -EFAULT;

	dst[len] = '\0';
	strim(dst);
	return count;
}

static int perflock_records_show(struct seq_file *m, void *v)
{
	mutex_lock(&perflock_lock);
	seq_printf(m, "%s\n", perflock_records_buff);
	mutex_unlock(&perflock_lock);
	return 0;
}

static int perflock_exception_show(struct seq_file *m, void *v)
{
	mutex_lock(&perflock_lock);
	seq_printf(m, "%s\n", perflock_exception_buff);
	mutex_unlock(&perflock_lock);
	return 0;
}

static ssize_t perflock_records_write(struct file *file,
				      const char __user *buf,
				      size_t count, loff_t *ppos)
{
	ssize_t ret;

	mutex_lock(&perflock_lock);
	ret = perf_helper_copy_user(perflock_records_buff,
				    sizeof(perflock_records_buff), buf, count);
	mutex_unlock(&perflock_lock);
	return ret;
}

static ssize_t perflock_exception_write(struct file *file,
					const char __user *buf,
					size_t count, loff_t *ppos)
{
	ssize_t ret;

	if (count >= PERF_HELPER_BUF_SIZE)
		pr_warn("perflock_exception userbuf size longer than 1024\n");

	mutex_lock(&perflock_lock);
	ret = perf_helper_copy_user(perflock_exception_buff,
				    sizeof(perflock_exception_buff), buf, count);
	mutex_unlock(&perflock_lock);
	return ret;
}

static int perflock_records_open(struct inode *inode, struct file *file)
{
	return single_open(file, perflock_records_show, NULL);
}

static int perflock_exception_open(struct inode *inode, struct file *file)
{
	return single_open(file, perflock_exception_show, NULL);
}

static const struct proc_ops perflock_records_ops = {
	.proc_open = perflock_records_open,
	.proc_read = seq_read,
	.proc_write = perflock_records_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static const struct proc_ops perflock_exception_ops = {
	.proc_open = perflock_exception_open,
	.proc_read = seq_read,
	.proc_write = perflock_exception_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static int pid_show_common(struct seq_file *m, void *v)
{
	int *pid = m->private;

	seq_printf(m, "%d\n", pid ? *pid : -1);
	return 0;
}

static int kswapd_pid_open(struct inode *inode, struct file *file)
{
	return single_open(file, pid_show_common, &kswapd_pid);
}

static int kcompactd_pid_open(struct inode *inode, struct file *file)
{
	return single_open(file, pid_show_common, &kcompactd_pid);
}

static int sfui_pid_open(struct inode *inode, struct file *file)
{
	return single_open(file, pid_show_common, &sfui_pid);
}

static int sfre_pid_open(struct inode *inode, struct file *file)
{
	return single_open(file, pid_show_common, &sfre_pid);
}

static const struct proc_ops kswapd_pid_ops = {
	.proc_open = kswapd_pid_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static const struct proc_ops kcompactd_pid_ops = {
	.proc_open = kcompactd_pid_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static const struct proc_ops sfui_pid_ops = {
	.proc_open = sfui_pid_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static const struct proc_ops sfre_pid_ops = {
	.proc_open = sfre_pid_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

int perflock_init(void)
{
	perflock_dir = proc_mkdir("perflock", NULL);
	if (!perflock_dir)
		return -ENOMEM;

	if (!proc_create("perflock_records", 0664, perflock_dir,
			 &perflock_records_ops))
		pr_warn("%s: create perflock_records node failed\n", __func__);

	if (!proc_create("perflock_exception", 0664, perflock_dir,
			 &perflock_exception_ops))
		pr_warn("%s: create perflock_exception node failed\n", __func__);

	proc_create("kswapd_pid", 0444, perflock_dir, &kswapd_pid_ops);
	proc_create("kcompactd_pid", 0444, perflock_dir, &kcompactd_pid_ops);
	proc_create("sfui_pid", 0444, perflock_dir, &sfui_pid_ops);
	proc_create("sfre_pid", 0444, perflock_dir, &sfre_pid_ops);

	pr_info("perf_helper: perflock_init\n");
	return 0;
}

void perflock_exit(void)
{
	remove_proc_subtree("perflock", NULL);
	perflock_dir = NULL;
}

static int mimdlog_show(struct seq_file *m, void *v)
{
	mutex_lock(&mimd_lock);
	seq_printf(m, "%s\n", mimd_log_buff);
	mutex_unlock(&mimd_lock);
	return 0;
}

static ssize_t mimdlog_write(struct file *file, const char __user *buf,
			     size_t count, loff_t *ppos)
{
	ssize_t ret;

	mutex_lock(&mimd_lock);
	ret = perf_helper_copy_user(mimd_log_buff, sizeof(mimd_log_buff),
				    buf, count);
	mutex_unlock(&mimd_lock);
	return ret;
}

static int mimdlog_open(struct inode *inode, struct file *file)
{
	return single_open(file, mimdlog_show, NULL);
}

static const struct proc_ops mimdlog_ops = {
	.proc_open = mimdlog_open,
	.proc_read = seq_read,
	.proc_write = mimdlog_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static int global_reclaim_show(struct seq_file *m, void *v)
{
	seq_printf(m, "reclaimed=%lu\n%s\n", reclaim_count, reclaim_buff);
	return 0;
}

static ssize_t global_reclaim_write(struct file *file, const char __user *buf,
				    size_t count, loff_t *ppos)
{
	ssize_t ret;

	mutex_lock(&mimd_lock);
	ret = perf_helper_copy_user(reclaim_buff, sizeof(reclaim_buff),
				    buf, count);
	if (ret > 0)
		reclaim_count++;
	mutex_unlock(&mimd_lock);
	return ret;
}

static int global_reclaim_open(struct inode *inode, struct file *file)
{
	return single_open(file, global_reclaim_show, NULL);
}

static const struct proc_ops global_reclaim_ops = {
	.proc_open = global_reclaim_open,
	.proc_read = seq_read,
	.proc_write = global_reclaim_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static ssize_t mimdtrigger_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%lu\n", reclaim_count);
}

static ssize_t mimdtrigger_store(struct kobject *kobj,
				 struct kobj_attribute *attr,
				 const char *buf, size_t count)
{
	reclaim_count++;
	return count;
}

static ssize_t mimdnotifier_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%u\n", scan_type);
}

static ssize_t mimdnotifier_store(struct kobject *kobj,
				  struct kobj_attribute *attr,
				  const char *buf, size_t count)
{
	return count;
}

static struct kobj_attribute mimdtrigger_attr =
	__ATTR(mimdtrigger, 0664, mimdtrigger_show, mimdtrigger_store);
static struct kobj_attribute mimdnotifier_attr =
	__ATTR(mimdnotifier, 0664, mimdnotifier_show, mimdnotifier_store);

static struct attribute *mimd_attrs[] = {
	&mimdtrigger_attr.attr,
	&mimdnotifier_attr.attr,
	NULL,
};

static const struct attribute_group mimd_attr_group = {
	.attrs = mimd_attrs,
};

int mimd_init(void)
{
	int ret = 0;

	mimd_dir = proc_mkdir("mimd", NULL);
	if (mimd_dir) {
		proc_create("mimdlog", 0664, mimd_dir, &mimdlog_ops);
		proc_create("global_reclaim", 0664, mimd_dir,
			    &global_reclaim_ops);
	}

	mimd_kobj = kobject_create_and_add("mimd", kernel_kobj);
	if (!mimd_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(mimd_kobj, &mimd_attr_group);
	if (ret)
		pr_warn("%s: create mimd sysfs nodes group failed\n", __func__);

	pr_info("perf_helper: mimd_init\n");
	return ret;
}

void mimd_exit(void)
{
	if (mimd_kobj) {
		sysfs_remove_group(mimd_kobj, &mimd_attr_group);
		kobject_put(mimd_kobj);
		mimd_kobj = NULL;
	}

	remove_proc_subtree("mimd", NULL);
	mimd_dir = NULL;
}

bool is_entity_eligible(void *cfs_rq, void *se)
{
	return true;
}

void mi_cfs_enqueue_runnable_task(void *rq, void *p)
{
}

void mi_cfs_dequeue_runnable_task(void *rq, void *p)
{
}

static ssize_t sched_long_runnable_check_show(struct kobject *kobj,
					      struct kobj_attribute *attr,
					      char *buf)
{
	return sysfs_emit(buf, "%u\n", sched_long_runnable_check);
}

static ssize_t sched_long_runnable_check_store(struct kobject *kobj,
					       struct kobj_attribute *attr,
					       const char *buf, size_t count)
{
	return kstrtobool(buf, &sched_long_runnable_check) ?: count;
}

static ssize_t sched_long_running_check_show(struct kobject *kobj,
					     struct kobj_attribute *attr,
					     char *buf)
{
	return sysfs_emit(buf, "%u\n", sched_long_running_check);
}

static ssize_t sched_long_running_check_store(struct kobject *kobj,
					      struct kobj_attribute *attr,
					      const char *buf, size_t count)
{
	return kstrtobool(buf, &sched_long_running_check) ?: count;
}

static ssize_t kdamond_cpuset_show(struct kobject *kobj,
				   struct kobj_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%s\n", kdamond_cpuset);
}

static ssize_t kdamond_cpuset_store(struct kobject *kobj,
				    struct kobj_attribute *attr,
				    const char *buf, size_t count)
{
	size_t len = min(count, sizeof(kdamond_cpuset) - 1);

	memcpy(kdamond_cpuset, buf, len);
	kdamond_cpuset[len] = '\0';
	strim(kdamond_cpuset);
	pr_info("kdamond pid:%d cpu_set:%s\n", kdamond_pid, kdamond_cpuset);
	return count;
}

static struct kobj_attribute sched_long_runnable_attr =
	__ATTR(sched_long_runnable, 0664, sched_long_runnable_check_show,
	       sched_long_runnable_check_store);
static struct kobj_attribute sched_long_running_attr =
	__ATTR(sched_long_running, 0664, sched_long_running_check_show,
	       sched_long_running_check_store);
static struct kobj_attribute kdamond_cpuset_attr =
	__ATTR(kdamond_cpuset, 0664, kdamond_cpuset_show,
	       kdamond_cpuset_store);

static struct attribute *sched_assi_attrs[] = {
	&sched_long_runnable_attr.attr,
	&sched_long_running_attr.attr,
	&kdamond_cpuset_attr.attr,
	NULL,
};

static const struct attribute_group sched_assi_attr_group = {
	.name = "sched_assi",
	.attrs = sched_assi_attrs,
};

int sched_assi_init(void)
{
	int ret;

	sched_assi_kobj = kobject_create_and_add("perf_helper", kernel_kobj);
	if (!sched_assi_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(sched_assi_kobj, &sched_assi_attr_group);
	if (ret)
		pr_warn("%s create sched_assi sysfs nodes group failed\n",
			__func__);

	pr_info("perf_helper: sched_assi_init\n");
	return ret;
}

void sched_assi_exit(void)
{
	if (!sched_assi_kobj)
		return;

	sysfs_remove_group(sched_assi_kobj, &sched_assi_attr_group);
	kobject_put(sched_assi_kobj);
	sched_assi_kobj = NULL;
}

static int __init perf_helper_init(void)
{
	int ret;

	ret = perflock_init();
	if (ret)
		return ret;

	ret = mimd_init();
	if (ret)
		goto err_mimd;

	ret = sched_assi_init();
	if (ret)
		goto err_sched_assi;

	return 0;

err_sched_assi:
	mimd_exit();
err_mimd:
	perflock_exit();
	return ret;
}

static void __exit perf_helper_exit(void)
{
	sched_assi_exit();
	mimd_exit();
	perflock_exit();
}

module_init(perf_helper_init);
module_exit(perf_helper_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Reconstructed Xiaomi perf helper");
MODULE_AUTHOR("reconstructed");
MODULE_INFO(name, "perf_helper");
MODULE_INFO(depends, "sched-walt");
