// SPDX-License-Identifier: GPL-2.0-only

/*
    mi_schedule module decompiled extracted changes from stock module
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/uidgid.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/rwlock.h>
#include <linux/refcount.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/cpumask.h>
#include <linux/slab.h>
#include <linux/atomic.h>

#define MI_SCHED_NR_CPUS 8
#define MI_SCHED_NR_CLUSTERS 4

struct mi_user_struct;
struct mi_group_data;
struct ls_group;

struct mi_task_struct {
	struct task_struct *task;
	struct mi_user_struct *mi_user;
	struct list_head node;
	unsigned long flags;
	int pid;
	uid_t uid;
};

struct mi_group_data {
	struct list_head node;
	struct list_head mi_tasks;
	struct list_head ls_groups;
	refcount_t refcount;
	spinlock_t lock;
	int group_id;
	int group_type;
	int fps;
};

struct mi_user_struct {
	struct list_head node;
	struct list_head tasks;
	struct mi_group_data *default_group;
	refcount_t refcount;
	uid_t uid;
};

struct ls_group {
	struct list_head node;
	struct list_head mi_tasks;
	struct mi_group_data *parent_group;
	refcount_t refcount;
	int ls_group_id;
	int ls_type;
};

struct mi_rq {
	struct rq *rq;
	struct list_head vip_tasks;
	spinlock_t lock;
	int cpu;
	int cluster_id;
};

struct sched_load_update_hooks {
	void (*update_task_load_hook)(void);
};

static LIST_HEAD(mi_schedule_active_groups);
static LIST_HEAD(mi_schedule_active_ls_groups);
static DEFINE_SPINLOCK(mi_schedule_lock);
static struct mi_group_data mi_schedule_system_group;
static struct mi_group_data mi_schedule_back_group;
static struct mi_group_data mi_schedule_kernel_group;
static struct mi_user_struct mi_schedule_zero_user;
static void *sched_load_update_hook;
static void *task_group_change_update_hook;
static void *mi_update_global_period_hook;

static int ls_grp_prefer_cid;
module_param(ls_grp_prefer_cid, int, 0644);
static unsigned int group_debug;
module_param(group_debug, uint, 0644);
static unsigned int target_makeup_ns;
module_param(target_makeup_ns, uint, 0644);

struct mi_rq mi_runqueues[MI_SCHED_NR_CPUS];
EXPORT_SYMBOL_GPL(mi_runqueues);
int num_sched_clusters = MI_SCHED_NR_CLUSTERS;
EXPORT_SYMBOL_GPL(num_sched_clusters);
DEFINE_SPINLOCK(period_freq_list_lock);
EXPORT_SYMBOL_GPL(period_freq_list_lock);

static int mi_schedule_store_hook(void **slot, void *hook)
{
	WRITE_ONCE(*slot, hook);
	smp_wmb();
	return 0;
}

static void mi_schedule_init_group(struct mi_group_data *group, int id, int type)
{
	INIT_LIST_HEAD(&group->node);
	INIT_LIST_HEAD(&group->mi_tasks);
	INIT_LIST_HEAD(&group->ls_groups);
	spin_lock_init(&group->lock);
	refcount_set(&group->refcount, 1);
	group->group_id = id;
	group->group_type = type;
}

struct mi_group_data *create_ls_group(void)
{
	return NULL;
}
EXPORT_SYMBOL_GPL(create_ls_group);

void free_mi_task_struct(void)
{
}
EXPORT_SYMBOL_GPL(free_mi_task_struct);

struct mi_group_data *find_mi_group_by_task(void)
{
	return NULL;
}
EXPORT_SYMBOL_GPL(find_mi_group_by_task);

void mi_sched_tick(void)
{
}
EXPORT_SYMBOL_GPL(mi_sched_tick);

struct mi_group_data *find_mi_group_by_user(void)
{
	return NULL;
}
EXPORT_SYMBOL_GPL(find_mi_group_by_user);

void mi_task_fork(void)
{
}
EXPORT_SYMBOL_GPL(mi_task_fork);

struct list_head *get_active_mi_groups(void)
{
	return &mi_schedule_active_groups;
}
EXPORT_SYMBOL_GPL(get_active_mi_groups);

int register_sched_load_update_hook(void *hook)
{
	return mi_schedule_store_hook(&sched_load_update_hook, hook);
}
EXPORT_SYMBOL_GPL(register_sched_load_update_hook);

int get_cluster_type(int cpu)
{
	return 0;
}
EXPORT_SYMBOL_GPL(get_cluster_type);

int unregister_sched_load_update_hook(void)
{
	return mi_schedule_store_hook(&sched_load_update_hook, NULL);
}
EXPORT_SYMBOL_GPL(unregister_sched_load_update_hook);

struct mi_group_data *get_mi_back_group(void)
{
	return &mi_schedule_back_group;
}
EXPORT_SYMBOL_GPL(get_mi_back_group);

struct mi_group_data *get_mi_kern_group(void)
{
	return &mi_schedule_kernel_group;
}
EXPORT_SYMBOL_GPL(get_mi_kern_group);

struct mi_group_data *get_mi_system_group(void)
{
	return &mi_schedule_system_group;
}
EXPORT_SYMBOL_GPL(get_mi_system_group);

struct mi_task_struct *get_mi_task_struct(struct task_struct *task)
{
	return NULL;
}
EXPORT_SYMBOL_GPL(get_mi_task_struct);

struct mi_task_struct *get_mi_task_struct_unsafe(struct task_struct *task)
{
	return NULL;
}
EXPORT_SYMBOL_GPL(get_mi_task_struct_unsafe);

struct mi_user_struct *get_mi_user_struct(uid_t uid)
{
	return &mi_schedule_zero_user;
}
EXPORT_SYMBOL_GPL(get_mi_user_struct);

u64 get_pred_target_period(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(get_pred_target_period);

bool is_32B_user(uid_t uid)
{
	return false;
}
EXPORT_SYMBOL_GPL(is_32B_user);

int mi_group_ls_group(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_group_ls_group);

void mi_group_ref_dec(void)
{
}
EXPORT_SYMBOL_GPL(mi_group_ref_dec);

void mi_task_exiting(void)
{
}
EXPORT_SYMBOL_GPL(mi_task_exiting);

void oem_update_num_sched_clusters(void)
{
}
EXPORT_SYMBOL_GPL(oem_update_num_sched_clusters);

int pkg_enable(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(pkg_enable);

int register_mi_update_global_period_f(void *hook)
{
	return mi_schedule_store_hook(&mi_update_global_period_hook, hook);
}
EXPORT_SYMBOL_GPL(register_mi_update_global_period_f);

int register_task_group_change_update_f(void *hook)
{
	return mi_schedule_store_hook(&task_group_change_update_hook, hook);
}
EXPORT_SYMBOL_GPL(register_task_group_change_update_f);

int thread_change_ls_group_auto(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(thread_change_ls_group_auto);

int thread_change_ls_group_explicit(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(thread_change_ls_group_explicit);

int unregister_mi_update_global_period_f(void)
{
	return mi_schedule_store_hook(&mi_update_global_period_hook, NULL);
}
EXPORT_SYMBOL_GPL(unregister_mi_update_global_period_f);

int unregister_task_group_change_update_f(void)
{
	return mi_schedule_store_hook(&task_group_change_update_hook, NULL);
}
EXPORT_SYMBOL_GPL(unregister_task_group_change_update_f);

void update_pkg_load(void)
{
}
EXPORT_SYMBOL_GPL(update_pkg_load);

/* Non-exported function stubs observed in mi_schedule.ko. */
static __maybe_unused int init_ravg_info(void)
{
	return 0;
}

static __maybe_unused int free_mi_group(void)
{
	return 0;
}

static __maybe_unused int find_add_mi_group_id(void)
{
	return 0;
}

static __maybe_unused int alloc_add_mi_group(void)
{
	return 0;
}

static __maybe_unused int mi_group_ref_add(void)
{
	return 0;
}

static __maybe_unused int find_add_mi_group_by_user(void)
{
	return 0;
}

static __maybe_unused int mi_ls_group_dec(void)
{
	return 0;
}

static __maybe_unused int destory_ls_group(void)
{
	return 0;
}

static __maybe_unused int mi_ls_group_add(void)
{
	return 0;
}

static __maybe_unused int alloc_ls_group(void)
{
	return 0;
}

static __maybe_unused int find_ls_grp_by_task(void)
{
	return 0;
}

static __maybe_unused int APP_destory_ls_group(void)
{
	return 0;
}

static __maybe_unused int change_lsgroup(void)
{
	return 0;
}

static __maybe_unused int change_lsgroup_status(void)
{
	return 0;
}

static __maybe_unused int alloc_mi_default_group(void)
{
	return 0;
}

static __maybe_unused int create_mi_default_group(void)
{
	return 0;
}

static __maybe_unused int free_mi_default_group(void)
{
	return 0;
}

static __maybe_unused int change_group_fps(void)
{
	return 0;
}

static __maybe_unused int change_system_to_display(void)
{
	return 0;
}

static __maybe_unused int init_mi_group(void)
{
	return 0;
}

static __maybe_unused int exit_mi_group(void)
{
	return 0;
}

static __maybe_unused int set_target_makeup_ns(void)
{
	return 0;
}

static __maybe_unused int get_target_makeup_ns(void)
{
	return 0;
}

static __maybe_unused int mi_group_ioctl(void)
{
	return 0;
}

static __maybe_unused int mi_group_mmap(void)
{
	return 0;
}

static __maybe_unused int mi_group_open(void)
{
	return 0;
}

static __maybe_unused int mi_group_release(void)
{
	return 0;
}

static __maybe_unused int create_runtime_proc(void)
{
	return 0;
}

static __maybe_unused int delete_runtimeproc(void)
{
	return 0;
}

static __maybe_unused int runtime_open_all(void)
{
	return 0;
}

static __maybe_unused int runtime_show_all(void)
{
	return 0;
}

static __maybe_unused int top_package_on_b2core_open(void)
{
	return 0;
}

static __maybe_unused int package_runtime_show(void)
{
	return 0;
}

static __maybe_unused int top_package_on_b1core_open(void)
{
	return 0;
}

static __maybe_unused int top_package_on_lcore_open(void)
{
	return 0;
}

static __maybe_unused int top_package_open(void)
{
	return 0;
}

static __maybe_unused int package_runtime_open(void)
{
	return 0;
}

static __maybe_unused int trace_package_write(void)
{
	return 0;
}

static __maybe_unused int package_compact_open(void)
{
	return 0;
}

static __maybe_unused int package_compact_show(void)
{
	return 0;
}

static __maybe_unused int package_tracestat_open(void)
{
	return 0;
}

static __maybe_unused int package_trace_stat(void)
{
	return 0;
}

static __maybe_unused int package_open_traced_window(void)
{
	return 0;
}

static __maybe_unused int traced_window_write(void)
{
	return 0;
}

static __maybe_unused int traced_window_show(void)
{
	return 0;
}

static __maybe_unused int package_open_window_size(void)
{
	return 0;
}

static __maybe_unused int window_size_show(void)
{
	return 0;
}

static __maybe_unused int develop_mode_open(void)
{
	return 0;
}

static __maybe_unused int develop_mode_set(void)
{
	return 0;
}

static __maybe_unused int develop_mode_show(void)
{
	return 0;
}

static __maybe_unused int package_reset_traced_window(void)
{
	return 0;
}

static __maybe_unused int reset_traced_window_show(void)
{
	return 0;
}

static __maybe_unused int pause_mode_open(void)
{
	return 0;
}

static __maybe_unused int pause_mode_set(void)
{
	return 0;
}

static __maybe_unused int pause_mode_show(void)
{
	return 0;
}

static __maybe_unused int get_curr_runtime_items(void)
{
	return 0;
}

static __maybe_unused int get_uid_max_value(void)
{
	return 0;
}

static __maybe_unused int get_runtime_window_size(void)
{
	return 0;
}

static __maybe_unused int get_user_zero(void)
{
	return 0;
}

static __maybe_unused int mi_cpu_coregroup_mask(void)
{
	return 0;
}

static __maybe_unused int get_package_runtime_info(void)
{
	return 0;
}

static __maybe_unused int task_user(void)
{
	return 0;
}

static __maybe_unused int task_add_to_userlist(void)
{
	return 0;
}

static __maybe_unused int task_add_work(void)
{
	return 0;
}

static __maybe_unused int mi_user_roll_wk(void)
{
	return 0;
}

static __maybe_unused int mi_task_world_init(void)
{
	return 0;
}

static int __init mi_schedule_init(void)
{
	mi_schedule_init_group(&mi_schedule_system_group, 0, 0);
	mi_schedule_init_group(&mi_schedule_back_group, 1, 0);
	mi_schedule_init_group(&mi_schedule_kernel_group, 2, 0);
	INIT_LIST_HEAD(&mi_schedule_zero_user.node);
	INIT_LIST_HEAD(&mi_schedule_zero_user.tasks);
	refcount_set(&mi_schedule_zero_user.refcount, 1);
	spin_lock_init(&mi_schedule_lock);
	pr_info("mi_schedule: reconstructed external scaffold loaded\n");
	return 0;
}

static void __exit mi_schedule_exit(void)
{
	pr_info("mi_schedule: reconstructed external scaffold unloaded\n");
}

module_init(mi_schedule_init);
module_exit(mi_schedule_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("pkg runtime info calc by David");
MODULE_INFO(scmversion, "gec8b9cedb7ab");
MODULE_INFO(intree, "Y");
MODULE_INFO(depends, "");
