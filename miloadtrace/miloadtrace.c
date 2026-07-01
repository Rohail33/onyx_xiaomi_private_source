// SPDX-License-Identifier: GPL-2.0
/*
 * Reconstructed Xiaomi miloadtrace module.
 *
 * Original module metadata:
 *   name=miloadtrace
 *   description=frame-load by David
 *   depends=mi_schedule,sched-walt,metis
 *
 * This source preserves the external ABI and registration surface recovered
 * from the vendor .ko.  The heavy frame-load accounting logic is intentionally
 * conservative so the module can be brought up safely while the exact policy is
 * reimplemented incrementally.
 */

#include <linux/cpufreq.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/irq_work.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/percpu.h>
#include <linux/proc_fs.h>
#include <linux/printk.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>

#define MILOAD_MAX_CLUSTERS 4
#define MILOAD_MAX_CPUS 8
#define MILOAD_DEFAULT_HISPEED_LOAD 90

struct mi_load_data {
	u32 cmd;
	pid_t pid;
	u32 fps;
	u64 period;
	u64 pred_period;
};

struct mi_group_frame {
	u32 id;
	u32 cid;
	pid_t pid;
	atomic_t nr_running;
	u64 prev_running;
	u64 curr_running;
	u64 prev_runnable;
	u64 curr_runnable;
	u64 window_start;
	u64 window_period;
	struct list_head node;
};

struct migov_tunables {
	struct gov_attr_set attr_set;
	unsigned int up_rate_limit_us;
	unsigned int down_rate_limit_us;
	unsigned int hispeed_load;
	unsigned int hispeed_freq;
	unsigned int hispeed_cond_freq;
	unsigned int target_load_thresh;
	unsigned int adaptive_level_1;
	unsigned int adaptive_low_freq;
	unsigned int adaptive_high_freq;
	unsigned int freq_shift;
	unsigned int avg_load;
};

struct migov_policy {
	struct cpufreq_policy *policy;
	struct migov_tunables *tunables;
	struct list_head tunables_hook;
	raw_spinlock_t update_lock;
	unsigned long avg_cap;
	unsigned long hispeed_util;
	unsigned long hispeed_cond_util;
	s64 min_rate_limit_ns;
	s64 up_rate_delay_ns;
	s64 down_rate_delay_ns;
	unsigned int driving_cpu;
};

struct migov_cpu {
	struct migov_policy *mg_policy;
	unsigned int cpu;
	unsigned long util;
	unsigned int flags;
};

typedef void (*miload_hook_t)(void);

extern int register_mi_update_fravg(void (*callback)(void));
extern int unregister_mi_update_fravg(void);
extern int register_walt_update_hooks(void (*callback)(void));
extern int unregister_walt_update_hooks(void);
extern int register_task_group_change_update_f(void (*callback)(void));
extern int unregister_task_group_change_update_f(void);
extern int register_mi_migrate_hook(void (*callback)(void));
extern int unregister_mi_migrate_hook(void);
extern int register_mi_update_group_nr(void (*callback)(void));
extern int unregister_mi_update_group_nr(void);
extern int register_mi_migarte_fix_fld_hooks(void (*callback)(void));
extern int unregister_mi_migarte_fix_fld_hooks(void);
extern int register_ui_task_detect(void (*callback)(void));
extern int unregister_ui_task_detect(void);
extern int register_mi_update_global_period_f(void (*callback)(void));
extern int unregister_mi_update_global_period_f(void);
extern int register_mi_run_frequpdate_hooks(void (*callback)(void));
extern int unregister_mi_run_frequpdate_hooks(void);
extern u64 walt_sched_clock(void);

static unsigned int util_policy;
static unsigned int grp_percent;
static unsigned int flw_debug;
static unsigned int choose_result_util_policy;
static unsigned int back_percent;
static unsigned int sys_percent;
static unsigned int kern_percent;
static unsigned int expand_shift;
static unsigned int target_delta_flag;
static unsigned int freq_target_delta_flag;
static unsigned int cal_running_time_type;
static unsigned int frame_vip_boost;
static unsigned int frame_drop_boost;
static unsigned int frame_util_boost[MILOAD_MAX_CLUSTERS];
static unsigned int util_boost_percent[MILOAD_MAX_CLUSTERS];
static unsigned int util_zero_boost_percent[MILOAD_MAX_CLUSTERS];
static unsigned int down_util_margin[MILOAD_MAX_CLUSTERS];
static unsigned int update_frame_fps = 60;

unsigned int mi_maxfreq[MILOAD_MAX_CPUS];
unsigned int mi_minfreq[MILOAD_MAX_CPUS];

static unsigned int cal_cpufreq_policy;
static unsigned int migov_debug;
static unsigned int in_perf_mod;
static unsigned int freq_mode;
static unsigned int out_capacity_thresh;
static unsigned int big_maxfreq_limit;

bool skip_walt_result;

DEFINE_PER_CPU(void *, migov_cb_data);

static DEFINE_PER_CPU(struct migov_cpu, mi_gov_cpu);
static DEFINE_MUTEX(min_rate_lock);
static LIST_HEAD(period_list);
static LIST_HEAD(period_freq_list);
//static DEFINE_RAW_SPINLOCK(period_list_lock);
DEFINE_RAW_SPINLOCK(period_freq_list_lock);

static struct proc_dir_entry *miload_dir;
static struct proc_dir_entry *frame_load_dir;
static struct proc_dir_entry *group_dir;
static struct proc_dir_entry *all_group_show_entry;
static struct proc_dir_entry *taskgrpid_entry;
static struct proc_dir_entry *usergrpid_entry;

static atomic_t flw_work_in_process = ATOMIC_INIT(0);
static struct irq_work flw_change_status_irq_work;
static struct irq_work flw_cpufreq_irq_work;
static struct irq_work flw_async_cpufreq_irq_work;
static u64 flw_irq_work_lastq_ws;

module_param(util_policy, uint, 0644);
module_param(grp_percent, uint, 0644);
module_param(flw_debug, uint, 0644);
module_param(choose_result_util_policy, uint, 0644);
module_param(back_percent, uint, 0644);
module_param(sys_percent, uint, 0644);
module_param(kern_percent, uint, 0644);
module_param(expand_shift, uint, 0644);
module_param(target_delta_flag, uint, 0644);
module_param(freq_target_delta_flag, uint, 0644);
module_param(cal_running_time_type, uint, 0644);
module_param(frame_vip_boost, uint, 0644);
module_param(frame_drop_boost, uint, 0644);
module_param_array(frame_util_boost, uint, NULL, 0644);
module_param_array(util_boost_percent, uint, NULL, 0644);
module_param_array(util_zero_boost_percent, uint, NULL, 0644);
module_param_array(down_util_margin, uint, NULL, 0644);
module_param(update_frame_fps, uint, 0644);
module_param_array(mi_maxfreq, uint, NULL, 0644);
module_param_array(mi_minfreq, uint, NULL, 0644);
module_param(cal_cpufreq_policy, uint, 0644);
module_param(migov_debug, uint, 0644);
module_param(in_perf_mod, uint, 0644);
module_param(freq_mode, uint, 0644);
module_param(out_capacity_thresh, uint, 0644);
module_param(big_maxfreq_limit, uint, 0644);

static inline void miload_debug(const char *fmt, ...)
{
	va_list args;

	if (!flw_debug)
		return;

	va_start(args, fmt);
	vprintk(fmt, args);
	va_end(args);
}

int find_cal_divisor(unsigned int fps)
{
	if (!fps)
		fps = update_frame_fps ?: 60;

	if (fps >= 120)
		return 2;
	if (fps >= 90)
		return 3;
	if (fps >= 60)
		return 4;

	return 6;
}

unsigned int get_group_util(int cid, int grp_id)
{
	unsigned int boost = 0;

	if (cid >= 0 && cid < MILOAD_MAX_CLUSTERS)
		boost = util_boost_percent[cid];

	miload_debug("flw-cluster%d, grp-id%d, util boost %u\n",
		     cid, grp_id, boost);
	return boost;
}

void transfer_APP_status(int pid, int status)
{
	miload_debug("pid%d status%d\n", pid, status);
}

void mi_frame_load_ravg(void)
{
	miload_debug("mi_frame_load_ravg\n");
}

void group_frame_cpufreq_transition(void)
{
	miload_debug("group_frame_cpufreq_transition\n");
}
EXPORT_SYMBOL_GPL(group_frame_cpufreq_transition);

static void skip_walt_window_update(void)
{
	skip_walt_result = true;
}

static void migrate_inter_cluster_fload(void)
{
	miload_debug("migrate inter cluster frame load\n");
}

static void mi_update_group_group(void)
{
	miload_debug("mi_update_group_group\n");
}

static void fix_fload_in_irq(void)
{
	miload_debug("fix_fload_in_irq\n");
}

static void grp_enq_boost_task(void)
{
	miload_debug("grp_enq_boost_task\n");
}

static void grp_enq_boost_pipeline(void)
{
	miload_debug("grp_enq_boost_pipeline\n");
}

static void update_gloabal_target_period(void)
{
	miload_debug("update global target period\n");
}

static void flw_irq_work(struct irq_work *work)
{
	flw_irq_work_lastq_ws = walt_sched_clock();
	atomic_set(&flw_work_in_process, 0);
}

static int __maybe_unused set_update_fps(const char *val,
					 const struct kernel_param *kp)
{
	unsigned int fps;
	int ret = kstrtouint(val, 0, &fps);

	if (ret)
		return ret;

	update_frame_fps = fps;
	return 0;
}

static int __maybe_unused get_update_fps(char *buf, const struct kernel_param *kp)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", update_frame_fps);
}

static const struct kernel_param_ops __maybe_unused param_ops_fps = {
	.set = set_update_fps,
	.get = get_update_fps,
};

static ssize_t mi_frame_ioctl(struct file *file, const char __user *buf,
			      size_t size, loff_t *ppos)
{
	struct mi_load_data data;

	if (size != sizeof(data))
		return -EINVAL;
	if (copy_from_user(&data, buf, sizeof(data)))
		return -EFAULT;

	miload_debug("ioctl-cmd%u,pid%d fps%u\n", data.cmd, data.pid, data.fps);
	update_frame_fps = data.fps ?: update_frame_fps;
	return sizeof(data);
}

static int mi_frame_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int mi_frame_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations miframe_fops = {
	.owner = THIS_MODULE,
	.open = mi_frame_open,
	.release = mi_frame_release,
	.write = mi_frame_ioctl,
};

static struct miscdevice miframe_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mi_frameload",
	.fops = &miframe_fops,
};

static int all_group_show(struct seq_file *m, void *v)
{
	seq_printf(m, "miloadtrace reconstructed\n");
	seq_printf(m, "fps=%u skip_walt=%u\n", update_frame_fps, skip_walt_result);
	return 0;
}

static int all_group_show_open(struct inode *inode, struct file *file)
{
	return single_open(file, all_group_show, NULL);
}

static const struct proc_ops all_group_show_fops = {
	.proc_open = all_group_show_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static ssize_t task_grp_id_write(struct file *file, const char __user *ubuf,
				 size_t count, loff_t *ppos)
{
	char buf[32];
	long pid;

	if (count >= sizeof(buf))
		return -EINVAL;
	if (copy_from_user(buf, ubuf, count))
		return -EFAULT;
	buf[count] = '\0';

	pid = simple_strtol(buf, NULL, 10);
	miload_debug("traced pid %ld\n", pid);
	return count;
}

static int task_grp_id_show(struct seq_file *m, void *v)
{
	seq_puts(m, "0\n");
	return 0;
}

static int task_grp_id_open(struct inode *inode, struct file *file)
{
	return single_open(file, task_grp_id_show, NULL);
}

static const struct proc_ops task_grp_id_fops = {
	.proc_open = task_grp_id_open,
	.proc_read = seq_read,
	.proc_write = task_grp_id_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static ssize_t user_grp_id_write(struct file *file, const char __user *ubuf,
				 size_t count, loff_t *ppos)
{
	char buf[32];
	long uid;

	if (count >= sizeof(buf))
		return -EINVAL;
	if (copy_from_user(buf, ubuf, count))
		return -EFAULT;
	buf[count] = '\0';

	uid = simple_strtol(buf, NULL, 10);
	miload_debug("traced uid %ld\n", uid);
	return count;
}

static int user_grp_id_show(struct seq_file *m, void *v)
{
	seq_puts(m, "0\n");
	return 0;
}

static int user_grp_id_open(struct inode *inode, struct file *file)
{
	return single_open(file, user_grp_id_show, NULL);
}

static const struct proc_ops user_grp_id_fops = {
	.proc_open = user_grp_id_open,
	.proc_read = seq_read,
	.proc_write = user_grp_id_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static void update_freq_record(unsigned int cpu, unsigned int freq)
{
	if (cpu < MILOAD_MAX_CPUS)
		mi_maxfreq[cpu] = max(mi_maxfreq[cpu], freq);
}

static void init_grp_freq_record(void)
{
	memset(mi_maxfreq, 0, sizeof(mi_maxfreq));
	memset(mi_minfreq, 0, sizeof(mi_minfreq));
}

static int cpu_gov_init(struct cpufreq_policy *policy)
{
	struct migov_policy *mg_policy;
	struct migov_tunables *tunables;

	mg_policy = kzalloc(sizeof(*mg_policy), GFP_KERNEL);
	if (!mg_policy)
		return -ENOMEM;

	tunables = kzalloc(sizeof(*tunables), GFP_KERNEL);
	if (!tunables) {
		kfree(mg_policy);
		return -ENOMEM;
	}

	tunables->hispeed_load = MILOAD_DEFAULT_HISPEED_LOAD;
	tunables->up_rate_limit_us = 0;
	tunables->down_rate_limit_us = 0;
	INIT_LIST_HEAD(&mg_policy->tunables_hook);
	raw_spin_lock_init(&mg_policy->update_lock);
	mg_policy->policy = policy;
	mg_policy->tunables = tunables;
	policy->governor_data = mg_policy;
	return 0;
}

static void cpu_gov_exit(struct cpufreq_policy *policy)
{
	struct migov_policy *mg_policy = policy->governor_data;

	if (!mg_policy)
		return;

	kfree(mg_policy->tunables);
	kfree(mg_policy);
	policy->governor_data = NULL;
}

static int cpu_gov_start(struct cpufreq_policy *policy)
{
	unsigned int cpu;

	for_each_cpu(cpu, policy->cpus) {
		struct migov_cpu *mg_cpu = &per_cpu(mi_gov_cpu, cpu);

		mg_cpu->cpu = cpu;
		mg_cpu->mg_policy = policy->governor_data;
	}

	return 0;
}

static void cpu_gov_stop(struct cpufreq_policy *policy)
{
	synchronize_rcu();
}

static void cpu_gov_limits(struct cpufreq_policy *policy)
{
	if (policy && policy->cur)
		update_freq_record(policy->cpu, policy->cur);
}

static struct cpufreq_governor mi_gov = {
	.name = "migov",
	.init = cpu_gov_init,
	.exit = cpu_gov_exit,
	.start = cpu_gov_start,
	.stop = cpu_gov_stop,
	.limits = cpu_gov_limits,
	.owner = THIS_MODULE,
};

int migtgov_register(void)
{
	pr_info("MI-GOV: migt gov register!\n");
	return cpufreq_register_governor(&mi_gov);
}

void migtgov_unregister(void)
{
	pr_info("MI-GOV: migt gov unregister!\n");
	cpufreq_unregister_governor(&mi_gov);
}

static int miload_register_hooks(void)
{
	int ret;

	ret = register_mi_update_fravg(mi_frame_load_ravg);
	if (ret)
		return ret;
	ret = register_walt_update_hooks(skip_walt_window_update);
	if (ret)
		goto err_walt;
	ret = register_task_group_change_update_f(grp_enq_boost_task);
	if (ret)
		goto err_group_change;
	ret = register_mi_migrate_hook(migrate_inter_cluster_fload);
	if (ret)
		goto err_migrate;
	ret = register_mi_update_group_nr(mi_update_group_group);
	if (ret)
		goto err_group_nr;
	ret = register_mi_migarte_fix_fld_hooks(fix_fload_in_irq);
	if (ret)
		goto err_fix;
	ret = register_ui_task_detect(grp_enq_boost_pipeline);
	if (ret)
		goto err_ui;
	ret = register_mi_update_global_period_f(update_gloabal_target_period);
	if (ret)
		goto err_global;
	ret = register_mi_run_frequpdate_hooks(group_frame_cpufreq_transition);
	if (ret)
		goto err_freq;

	return 0;

err_freq:
	unregister_mi_update_global_period_f();
err_global:
	unregister_ui_task_detect();
err_ui:
	unregister_mi_migarte_fix_fld_hooks();
err_fix:
	unregister_mi_update_group_nr();
err_group_nr:
	unregister_mi_migrate_hook();
err_migrate:
	unregister_task_group_change_update_f();
err_group_change:
	unregister_walt_update_hooks();
err_walt:
	unregister_mi_update_fravg();
	return ret;
}

static void miload_unregister_hooks(void)
{
	unregister_mi_run_frequpdate_hooks();
	unregister_mi_update_global_period_f();
	unregister_ui_task_detect();
	unregister_mi_migarte_fix_fld_hooks();
	unregister_mi_update_group_nr();
	unregister_mi_migrate_hook();
	unregister_task_group_change_update_f();
	unregister_walt_update_hooks();
	unregister_mi_update_fravg();
}

static int miload_proc_init(void)
{
	miload_dir = proc_mkdir("miload", NULL);
	if (!miload_dir)
		return -ENOMEM;

	frame_load_dir = proc_mkdir("frame_load", miload_dir);
	group_dir = proc_mkdir("group", miload_dir);

	all_group_show_entry = proc_create("all_group", 0444, group_dir ?: miload_dir,
					   &all_group_show_fops);
	taskgrpid_entry = proc_create("task_grp_id", 0664, group_dir ?: miload_dir,
				      &task_grp_id_fops);
	usergrpid_entry = proc_create("user_grp_id", 0664, group_dir ?: miload_dir,
				      &user_grp_id_fops);
	return 0;
}

static void miload_proc_exit(void)
{
	proc_remove(usergrpid_entry);
	proc_remove(taskgrpid_entry);
	proc_remove(all_group_show_entry);
	proc_remove(group_dir);
	proc_remove(frame_load_dir);
	proc_remove(miload_dir);
}

static int __init miloadtrace_init(void)
{
	int ret;

	init_grp_freq_record();
	init_irq_work(&flw_change_status_irq_work, flw_irq_work);
	init_irq_work(&flw_cpufreq_irq_work, flw_irq_work);
	init_irq_work(&flw_async_cpufreq_irq_work, flw_irq_work);

	ret = misc_register(&miframe_misc);
	if (ret)
		return ret;

	ret = miload_proc_init();
	if (ret)
		goto err_misc;

	ret = miload_register_hooks();
	if (ret)
		goto err_proc;

	ret = migtgov_register();
	if (ret)
		goto err_hooks;

	pr_info("frame load init success\n");
	return 0;

err_hooks:
	miload_unregister_hooks();
err_proc:
	miload_proc_exit();
err_misc:
	misc_deregister(&miframe_misc);
	return ret;
}

static void __exit miloadtrace_exit(void)
{
	migtgov_unregister();
	irq_work_sync(&flw_change_status_irq_work);
	irq_work_sync(&flw_cpufreq_irq_work);
	irq_work_sync(&flw_async_cpufreq_irq_work);
	miload_unregister_hooks();
	miload_proc_exit();
	misc_deregister(&miframe_misc);
	pr_info("frame load exit!!!\n");
}

module_init(miloadtrace_init);
module_exit(miloadtrace_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("frame-load by David");
MODULE_INFO(name, "miloadtrace");
MODULE_INFO(intree, "Y");
MODULE_INFO(scmversion, "gec8b9cedb7ab");
MODULE_INFO(depends, "mi_schedule,sched-walt,metis");
