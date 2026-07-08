// SPDX-License-Identifier: GPL-2.0-only

/*
	migt module from stock rom decompiled 
*/

#include <linux/atomic.h>
#include <linux/cpufreq.h>
#include <linux/fs.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/kobject.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/percpu.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/timekeeping.h>
#include <linux/uaccess.h>

struct migt_frame_record {
	u64 frame_load;
	u64 frame_load_max;
	u64 cpu_util;
	u64 target_freq;
	u64 flt_load;
	u64 max;
	u64 capacity;
	u32 render_scaling;
	u32 adaptive_low_freq;
	u32 adaptive_high_freq;
	u64 next_freq;
};

struct migt_runtime_record {
	u64 cpu_util;
	u64 vutil;
	u64 cpu_util_cur;
	u64 cpu_util_prev;
	u64 max_cur_load;
	u64 max_load;
	int boost_reason;
	u64 need_target_delta;
	u64 prev_frame_period;
	u64 cur_frame_period;
	u64 max_time;
};

struct migt_state {
	atomic_t enabled;
	atomic_t gtc_enabled;
	atomic_t glk_enabled;
	atomic_t flt_enabled;
	atomic_t pmc_enabled;

	struct proc_dir_entry *proc_dir;
	struct miscdevice misc;
	struct mutex lock;
	struct task_struct *worker;
	struct workqueue_struct *wq;
	struct delayed_work freq_work;

	int choose_cluster_disable;
	int force_cluster_sched_enable;
	int need_reset_runtime_everytime;
	int force_reset_runtime;
	int force_forbidden_walt_lb;
	int force_viptask_select_rq;
	int glk_disable;
	int glk_fbreak_enable;
	u32 glk_freq_limit_start;
	u32 glk_freq_limit_walt;
	u32 glk_maxfreq;
	u32 glk_minfreq;
	int vip_prefer_cluster;
	int render_prefer_cluster;
	int stask_prefer_cluster;
	int ip_task_max_num;
	int vip_task_max_num;
	int stask_candidate_num;
};

static struct migt_state g_migt;

/* Module parameters mirrored from the binary surface. */
static int migt_debug;
static int migt_ms;
static int migt_thresh;
static int itask_detect_interval;
static int over_thresh_count;
static int cpu_boost_cycle;
static int soc_pmu_track_enable;
static int render_update;
static int migt_viptask_thresh;
static int fps_mexec_update_policy;
static int fps_variance_ratio;
static int enable_pkg_monitor;
static int set_render_as_stask;
static int high_resolution_enable;
static int first_lunch_fps;
static int ceiling_slack_ms;
static int ceiling_from_user;

module_param(migt_debug, int, 0644);
module_param(migt_ms, int, 0644);
module_param(migt_thresh, int, 0644);
module_param(itask_detect_interval, int, 0644);
module_param(over_thresh_count, int, 0644);
module_param(cpu_boost_cycle, int, 0644);
module_param(soc_pmu_track_enable, int, 0644);
module_param(render_update, int, 0644);
module_param(migt_viptask_thresh, int, 0644);
module_param(fps_mexec_update_policy, int, 0644);
module_param(fps_variance_ratio, int, 0644);
module_param(enable_pkg_monitor, int, 0644);
module_param(set_render_as_stask, int, 0644);
module_param(high_resolution_enable, int, 0644);
module_param(first_lunch_fps, int, 0644);
module_param(ceiling_slack_ms, int, 0644);
module_param(ceiling_from_user, int, 0644);

/* Xiaomi hook imports exported by companion modules. */
extern int register_choose_cpu_overutiled(void);
extern void unregister_choose_cpu_overutiled(void);
extern int register_mi_find_energy_efficient_cpu(void);
extern void unregister_mi_find_energy_efficient_cpu(void);
extern int register_mi_lb_detect_func(void);
extern void unregister_mi_lb_detect_func(void);
extern int register_mi_sched_enable(void);
extern void unregister_mi_sched_enable(void);
extern int register_mi_walt_get_indicies(void);
extern void unregister_mi_walt_get_indicies(void);
extern int register_render_task_detect(void);
extern void unregister_render_task_detect(void);
extern int register_sched_load_update_hook(void);
extern void unregister_sched_load_update_hook(void);
extern int register_ui_task_detect(void);
extern void unregister_ui_task_detect(void);
extern int register_vip_task_detect(void);
extern void unregister_vip_task_detect(void);
extern int register_mi_dflt_lat_update_cb(void *cb);
extern int unregister_mi_dflt_lat_update_cb(void);
extern int register_mi_dflt_bw_update_cb(void *cb);
extern int unregister_mi_dflt_bw_update_cb(void);
extern int register_corectl_boost_hook(void);



static void migt_trace(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vprintk(fmt, args);
	va_end(args);
}

static int migt_startup(void)
{
	int ret = 0;

	/*ret |= register_game_migt_cpus_hook();
	ret |= register_game_migt_exclusive_cpus_hook();
	ret |= register_game_migt_should_skip_migrate_hook();
	ret |= register_game_migt_should_skip_this_cpu_rt_hook();
	ret |= register_game_migt_skip_cpus_hook();
	ret |= register_game_migt_skip_cpus_rt_hook();
	ret |= register_game_ravg_window();
	ret |= register_game_vip_hook();
	ret |= register_soc_update_flt_load();
	ret |= register_soc_update_flt_reset();
	ret |= register_choose_cpu_overutiled();
	ret |= register_mi_find_energy_efficient_cpu();
	ret |= register_mi_lb_detect_func();
	ret |= register_mi_sched_enable();
	ret |= register_mi_walt_get_indicies();
	ret |= register_migt_check_rt_hook();
	ret |= register_oem_flt_enable_hook();
	ret |= register_oem_flt_wakeup_hook();
	ret |= register_oem_freq_hook();
	ret |= register_oem_update_frame_load_hook();
	ret |= register_render_task_detect();
	ret |= register_sched_load_update_hook();
	ret |= register_ui_task_detect();
	ret |= register_vip_task_detect();
	ret |= register_corectl_boost_hook();*/

	return ret;
}

static void migt_shutdown(void)
{
	/*unregister_vip_task_detect();
	unregister_ui_task_detect();
	unregister_sched_load_update_hook();
	unregister_render_task_detect();
	unregister_oem_update_frame_load_hook();
	unregister_oem_freq_hook();
	unregister_oem_flt_wakeup_hook();
	unregister_oem_flt_enable_hook();
	unregister_migt_check_rt_hook();
	unregister_mi_walt_get_indicies();
	unregister_mi_sched_enable();
	unregister_mi_lb_detect_func();
	unregister_mi_find_energy_efficient_cpu();
	unregister_choose_cpu_overutiled();
	unregister_soc_update_flt_reset();
	unregister_soc_update_flt_load();
	unregister_game_vip_hook();
	unregister_game_ravg_window();
	unregister_game_migt_skip_cpus_rt_hook();
	unregister_game_migt_skip_cpus_hook();
	unregister_game_migt_should_skip_this_cpu_rt_hook();
	unregister_game_migt_should_skip_migrate_hook();
	unregister_game_migt_exclusive_cpus_hook();
	unregister_game_migt_cpus_hook();*/
}

int migt_sched_init(void)
{
	mutex_init(&g_migt.lock);
	atomic_set(&g_migt.enabled, 1);
	return migt_startup();
}
EXPORT_SYMBOL_GPL(migt_sched_init);

void migt_sched_exit(void)
{
	migt_shutdown();
}
EXPORT_SYMBOL_GPL(migt_sched_exit);

int migt_freq_qos_init(void)
{
	migt_trace("6migt: freq qos init\n");
	return 0;
}
EXPORT_SYMBOL_GPL(migt_freq_qos_init);

int migt_freq_qos_request_init_first(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(migt_freq_qos_request_init_first);

void migt_set_prefer_cluster(int cid)
{
	g_migt.stask_prefer_cluster = cid;
}
EXPORT_SYMBOL_GPL(migt_set_prefer_cluster);

int migt_get_prefer_cluster(void)
{
	return g_migt.stask_prefer_cluster;
}
EXPORT_SYMBOL_GPL(migt_get_prefer_cluster);

u64 get_frame_period(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(get_frame_period);

void add_work_to_migt(void)
{
}
EXPORT_SYMBOL_GPL(add_work_to_migt);

void flt_update_sf(void)
{
}
EXPORT_SYMBOL_GPL(flt_update_sf);

void flt_irq_work(void)
{
}
EXPORT_SYMBOL_GPL(flt_irq_work);

int flt_enable(int enable)
{
	atomic_set(&g_migt.flt_enabled, !!enable);
	return 0;
}
EXPORT_SYMBOL_GPL(flt_enable);

int glk_enable(int enable)
{
	atomic_set(&g_migt.glk_enabled, !!enable);
	return 0;
}
EXPORT_SYMBOL_GPL(glk_enable);

int get_migt_thresh(void)
{
	return migt_thresh;
}
EXPORT_SYMBOL_GPL(get_migt_thresh);

bool is_render_thread(struct task_struct *p)
{
	return false;
}
EXPORT_SYMBOL_GPL(is_render_thread);

bool is_frender_sibling(struct task_struct *p)
{
	return false;
}
EXPORT_SYMBOL_GPL(is_frender_sibling);

int migt_load_tracking_enabled(void)
{
	return atomic_read(&g_migt.enabled);
}
EXPORT_SYMBOL_GPL(migt_load_tracking_enabled);

static int __init migt_init(void)
{
	pr_info("migt: Xiaomi MIGT compatibility layer loaded\n");
	return migt_sched_init();
}

static void __exit migt_exit(void)
{
	migt_sched_exit();
	pr_info("migt: Xiaomi MIGT compatibility layer unloaded\n");
}

module_init(migt_init);
module_exit(migt_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Xiaomi MIGT reconstructed source export");

MODULE_SOFTDEP("pre: sched-walt");
