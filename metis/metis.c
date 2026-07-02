// SPDX-License-Identifier: GPL-2.0-only
/*
   metis module decompiled it 
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/atomic.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/sysctl.h>
#include <linux/cpufreq.h>

#define METIS_NR_CPUS 8

static bool metis_safe_boot = true;
module_param(metis_safe_boot, bool, 0644);
MODULE_PARM_DESC(metis_safe_boot, "Keep reconstructed Metis hooks disabled for boot safety");

static unsigned int link_debug;
module_param(link_debug, uint, 0644);
static unsigned int mi_link_enable;
module_param(mi_link_enable, uint, 0644);
static unsigned int mi_async_binder_link_enable;
module_param(mi_async_binder_link_enable, uint, 0644);
static unsigned int mi_lock_blocked_pid;
module_param(mi_lock_blocked_pid, uint, 0644);
static unsigned int minor_window_app;
module_param(minor_window_app, uint, 0644);
static unsigned int freedom_window_app;
module_param(freedom_window_app, uint, 0644);
static unsigned int tsched_debug;
module_param(tsched_debug, uint, 0644);
static unsigned int bug_detect;
module_param(bug_detect, uint, 0644);
static unsigned int cluaff_control;
module_param(cluaff_control, uint, 0644);
static unsigned int mi_boost_duration;
module_param(mi_boost_duration, uint, 0644);
static int min_cluster_freqs;
module_param(min_cluster_freqs, int, 0644);
static unsigned int fboost_debug;
module_param(fboost_debug, uint, 0644);
static unsigned int mi_freq_enable;
module_param(mi_freq_enable, uint, 0644);
static unsigned int mi_switch_enable;
module_param(mi_switch_enable, uint, 0644);
static int user_min_freq;
module_param(user_min_freq, int, 0644);
static unsigned int fperiod;
module_param(fperiod, uint, 0644);
static unsigned int metis_debug;
module_param(metis_debug, uint, 0644);
static unsigned int mi_fboost_enable;
module_param(mi_fboost_enable, uint, 0644);
static unsigned int limit_bgtask_sched;
module_param(limit_bgtask_sched, uint, 0644);
static unsigned int thermal_break_enable;
module_param(thermal_break_enable, uint, 0644);
static int version;
module_param(version, int, 0644);
static unsigned int force_forbidden_walt_lb;
module_param(force_forbidden_walt_lb, uint, 0644);
static unsigned int force_viptask_select_rq;
module_param(force_viptask_select_rq, uint, 0644);
static unsigned int task_enqueue_time_toolong;
module_param(task_enqueue_time_toolong, uint, 0644);
static unsigned int force_cluster_sched_enable;
module_param(force_cluster_sched_enable, uint, 0644);
static unsigned int doublecyc_debug;
module_param(doublecyc_debug, uint, 0644);
static int sched_doctor_enable;
module_param(sched_doctor_enable, int, 0644);
static int suspend_vip_enable;
module_param(suspend_vip_enable, int, 0644);
static int normal_running_too_long_thresh;
module_param(normal_running_too_long_thresh, int, 0644);
static int rt_running_too_long_thresh;
module_param(rt_running_too_long_thresh, int, 0644);
static unsigned int metis_lb_debug;
module_param(metis_lb_debug, uint, 0644);
static unsigned int metis_tp_debug;
module_param(metis_tp_debug, uint, 0644);
static unsigned int mi_viptask_balance;
module_param(mi_viptask_balance, uint, 0644);
static unsigned int metis_schlat_enable;
module_param(metis_schlat_enable, uint, 0644);
static unsigned int metis_schctx_enable;
module_param(metis_schctx_enable, uint, 0644);
static unsigned int protect_cs;
module_param(protect_cs, uint, 0644);
static unsigned int coldstart_set_mmlock_holder_vip;
module_param(coldstart_set_mmlock_holder_vip, uint, 0644);
static unsigned int cs_protect_dur;
module_param(cs_protect_dur, uint, 0644);
static unsigned int cs_stat_type;
module_param(cs_stat_type, uint, 0644);
static unsigned int low_lt_preempt_thres;
module_param(low_lt_preempt_thres, uint, 0644);
static unsigned int low_lt_preempt_thres_cold_start;
module_param(low_lt_preempt_thres_cold_start, uint, 0644);
static unsigned int set_cold_start_render_vip;
module_param(set_cold_start_render_vip, uint, 0644);
static unsigned int override_schedboost_in_coldstart;
module_param(override_schedboost_in_coldstart, uint, 0644);
static unsigned int set_possible_notifythr_vip;
module_param(set_possible_notifythr_vip, uint, 0644);
static unsigned int rwsem_normal_spin_thres;
module_param(rwsem_normal_spin_thres, uint, 0644);
static unsigned int rwsem_vip_spin_thres;
module_param(rwsem_vip_spin_thres, uint, 0644);
static unsigned int mutex_normal_spin_thres;
module_param(mutex_normal_spin_thres, uint, 0644);
static unsigned int mutex_vip_spin_thres;
module_param(mutex_vip_spin_thres, uint, 0644);
static unsigned int read_trylock_reserved_jif;
module_param(read_trylock_reserved_jif, uint, 0644);
static unsigned int ui_blocked_reader_jif;
module_param(ui_blocked_reader_jif, uint, 0644);
static unsigned int reader_biased_jif;
module_param(reader_biased_jif, uint, 0644);
static unsigned int reader_biased_jif_for_vip;
module_param(reader_biased_jif_for_vip, uint, 0644);
static unsigned long rtg_handoff_min_inter;
module_param(rtg_handoff_min_inter, ulong, 0644);
static unsigned int stask_prefer_cpu;
module_param(stask_prefer_cpu, uint, 0644);
static unsigned int cs_debug;
module_param(cs_debug, uint, 0644);
static unsigned int metis_wakeup_enable;
module_param(metis_wakeup_enable, uint, 0644);
static unsigned int metis_wakeup_debug;
module_param(metis_wakeup_debug, uint, 0644);

/* Exported data symbols observed in metis (2).ko. */
void *mi_try_to_wake_up_func;
EXPORT_SYMBOL_GPL(mi_try_to_wake_up_func);
unsigned int in_perf_mod;
module_param(in_perf_mod, uint, 0644);
EXPORT_SYMBOL_GPL(in_perf_mod);
unsigned long perf_min_freq;
EXPORT_SYMBOL_GPL(perf_min_freq);
int choose_cpu_overutiled;
EXPORT_SYMBOL_GPL(choose_cpu_overutiled);
int show_traced_uid;
EXPORT_SYMBOL_GPL(show_traced_uid);
unsigned int flt_enable_other;
module_param(flt_enable_other, uint, 0644);
EXPORT_SYMBOL_GPL(flt_enable_other);
int ui_run_cpu;
EXPORT_SYMBOL_GPL(ui_run_cpu);
int render_run_cpu;
EXPORT_SYMBOL_GPL(render_run_cpu);
int speed_touch_producer_in_boost;
EXPORT_SYMBOL_GPL(speed_touch_producer_in_boost);
int speed_touch_consumer_in_boost;
EXPORT_SYMBOL_GPL(speed_touch_consumer_in_boost);
void *mi_walt_get_indicies_func;
EXPORT_SYMBOL_GPL(mi_walt_get_indicies_func);
void *wake_render_func;
EXPORT_SYMBOL_GPL(wake_render_func);
void *mi_enqueue_task_fair_func;
EXPORT_SYMBOL_GPL(mi_enqueue_task_fair_func);
void *mi_dequeue_task_fair_func;
EXPORT_SYMBOL_GPL(mi_dequeue_task_fair_func);
unsigned int low_frame_op;
module_param(low_frame_op, uint, 0644);
EXPORT_SYMBOL_GPL(low_frame_op);
int get_cpu_util_without;
EXPORT_SYMBOL_GPL(get_cpu_util_without);
int in_scroll;
EXPORT_SYMBOL_GPL(in_scroll);
void *metis_runqueues;
EXPORT_SYMBOL_GPL(metis_runqueues);
int metis_lb_check_for_higher_capacity;
EXPORT_SYMBOL_GPL(metis_lb_check_for_higher_capacity);
int metis_lb_cpu_overutilized;
EXPORT_SYMBOL_GPL(metis_lb_cpu_overutilized);
unsigned long metis_lb_cpu_halt_mask;
EXPORT_SYMBOL_GPL(metis_lb_cpu_halt_mask);
int metis_lb_cpu_array[8];
EXPORT_SYMBOL_GPL(metis_lb_cpu_array);
int metis_lb_min_possible_cluster_id;
EXPORT_SYMBOL_GPL(metis_lb_min_possible_cluster_id);
int get_cpu_util;
EXPORT_SYMBOL_GPL(get_cpu_util);

static int metis_store_hook(void **slot, void *hook)
{
	if (metis_safe_boot)
		return 0;

	WRITE_ONCE(*slot, hook);
	smp_wmb();
	return 0;
}

static void *game_binder_f_ptr;
static void *rvh_mi_try_to_wake_up_ptr;
static void *corectl_boost_hook_ptr;
static void *update_perflock_freq_hook_ptr;
static void *get_perflock_last_freqs_hook_ptr;
static void *game_metis_skip_cpus_hook_ptr;
static void *mi_enqueue_entity_hook_ptr;
static void *mi_enqueue_task_fair_hook_ptr;
static void *mi_lb_detect_func_ptr;
static void *ui_task_detect_ptr;
static void *render_task_detect_ptr;
static void *vip_task_detect_ptr;
static void *mi_walt_get_indicies_ptr;
static void *mi_find_energy_efficient_cpu_ptr;
static void *mi_dequeue_task_fair_hook_ptr;
static void *mi_sched_enable_ptr;
static void *choose_cpu_overutiled_ptr;
static void *override_schedboost_ptr;
//static void *soc_update_flt_load_ptr;
//static void *soc_update_flt_reset_ptr;

/* Exported function stubs. Prototypes are intentionally generic
 * where the exact vendor prototype is not recoverable from ELF alone.
 */
int register_game_binder_f(void *hook)
{
	return metis_store_hook(&game_binder_f_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_game_binder_f);

int unregister_game_binder_f(void)
{
	return metis_store_hook(&game_binder_f_ptr, NULL);
}
EXPORT_SYMBOL_GPL(unregister_game_binder_f);

int mi_resched_vip_task(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_resched_vip_task);

int mi_lock_vip_task(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_lock_vip_task);

int mi_binder_vip_task(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_binder_vip_task);

int mi_link_vip_task(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_link_vip_task);

int mi_mutex_list_add_hook(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_mutex_list_add_hook);

int mi_art_mutex_vip_task(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_art_mutex_vip_task);

int register_rvh_mi_try_to_wake_up(void *hook)
{
	return metis_store_hook(&rvh_mi_try_to_wake_up_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_rvh_mi_try_to_wake_up);

int unregister_rvh_mi_try_to_wake_up(void)
{
	return metis_store_hook(&rvh_mi_try_to_wake_up_ptr, NULL);
}
EXPORT_SYMBOL_GPL(unregister_rvh_mi_try_to_wake_up);

int metis_link_init(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_link_init);

int metis_link_exit(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_link_exit);

int mi_select_task_rq_hook(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_select_task_rq_hook);

int clu_aff_init(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(clu_aff_init);

int clu_aff_exit(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(clu_aff_exit);

int register_corectl_boost_hook(void *hook)
{
	return metis_store_hook(&corectl_boost_hook_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_corectl_boost_hook);

int register_update_perflock_freq_hook(void *hook)
{
	return metis_store_hook(&update_perflock_freq_hook_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_update_perflock_freq_hook);

int register_get_perflock_last_freqs_hook(void *hook)
{
	return metis_store_hook(&get_perflock_last_freqs_hook_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_get_perflock_last_freqs_hook);

int cpu_to_cluster(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(cpu_to_cluster);

int trigger_fboost_work(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(trigger_fboost_work);

int del_cluster(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(del_cluster);

int boost_cluster(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(boost_cluster);

int restore_boost_cluster(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(restore_boost_cluster);

int store_boost_cluster(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(store_boost_cluster);

int get_frmboost_sta_hook(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(get_frmboost_sta_hook);

int oem_rvh_try_to_wake_up_success(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(oem_rvh_try_to_wake_up_success);

int oem_should_update_freq(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(oem_should_update_freq);

int metis_fboost_init(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_fboost_init);

int metis_fboost_exit(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_fboost_exit);

int register_game_metis_skip_cpus_hook(void *hook)
{
	return metis_store_hook(&game_metis_skip_cpus_hook_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_game_metis_skip_cpus_hook);

int unregister_game_metis_skip_cpus_hook(void)
{
	return metis_store_hook(&game_metis_skip_cpus_hook_ptr, NULL);
}
EXPORT_SYMBOL_GPL(unregister_game_metis_skip_cpus_hook);

int set_prefer_cluster_with_check(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(set_prefer_cluster_with_check);

int set_prefer_cpu_with_check(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(set_prefer_cpu_with_check);

int get_mi_fboost_enable(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(get_mi_fboost_enable);

int mi_disable_qcom_pipeline(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_disable_qcom_pipeline);

int get_mi_link_boost_enable(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(get_mi_link_boost_enable);

int in_interest_input_event(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(in_interest_input_event);

int fas_sched_clock(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(fas_sched_clock);

int mi_interface_set_dynamic_vip_task(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_interface_set_dynamic_vip_task);

int mi_interface_restore_dynamic_vip_task(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_interface_restore_dynamic_vip_task);

int mi_set_fas_req(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_set_fas_req);

int mi_clean_fas_req(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_clean_fas_req);

int mi_check_preempt_wakeup_hook(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_check_preempt_wakeup_hook);

int register_mi_enqueue_entity_hook(void *hook)
{
	return metis_store_hook(&mi_enqueue_entity_hook_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_mi_enqueue_entity_hook);

int unregister_mi_enqueue_entity_hook(void)
{
	return metis_store_hook(&mi_enqueue_entity_hook_ptr, NULL);
}
EXPORT_SYMBOL_GPL(unregister_mi_enqueue_entity_hook);

int speed_touch_store_freq(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(speed_touch_store_freq);

int speed_touch_restore_freq(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(speed_touch_restore_freq);

int mi_enqueue_task_fair_hook(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_enqueue_task_fair_hook);

int mi_dequeue_task_fair_hook(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_dequeue_task_fair_hook);

int mi_pick_next_task_fair_hook(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_pick_next_task_fair_hook);

int get_foreground_uid(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(get_foreground_uid);

int set_foreground_app(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(set_foreground_app);

int register_mi_enqueue_task_fair_hook(void *hook)
{
	return metis_store_hook(&mi_enqueue_task_fair_hook_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_mi_enqueue_task_fair_hook);

int unregister_mi_enqueue_task_fair_hook(void)
{
	return metis_store_hook(&mi_enqueue_task_fair_hook_ptr, NULL);
}
EXPORT_SYMBOL_GPL(unregister_mi_enqueue_task_fair_hook);

int sched_boost_should_disable(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(sched_boost_should_disable);

int mi_ui_task(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_ui_task);

int mi_render_task(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_render_task);

int mi_vip_task(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_vip_task);

int mi_ui_pipeline(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_ui_pipeline);

int mi_render_pipeline(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_render_pipeline);

int set_mi_rq_balance_irq_task(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(set_mi_rq_balance_irq_task);

int walt_mi_irq_task_rq_balance_hook(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(walt_mi_irq_task_rq_balance_hook);

int get_vtask_monitor_status(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(get_vtask_monitor_status);

int mi_lb_enable(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_lb_enable);

int register_mi_lb_detect_func(void *hook)
{
	return metis_store_hook(&mi_lb_detect_func_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_mi_lb_detect_func);

int unregister_mi_lb_detect_func(void)
{
	return metis_store_hook(&mi_lb_detect_func_ptr, NULL);
}
EXPORT_SYMBOL_GPL(unregister_mi_lb_detect_func);

int metis_choose_cpu_fastpath(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_choose_cpu_fastpath);

int register_ui_task_detect(void *hook)
{
	return metis_store_hook(&ui_task_detect_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_ui_task_detect);

int register_render_task_detect(void *hook)
{
	return metis_store_hook(&render_task_detect_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_render_task_detect);

int register_vip_task_detect(void *hook)
{
	return metis_store_hook(&vip_task_detect_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_vip_task_detect);

int unregister_ui_task_detect(void)
{
	return metis_store_hook(&ui_task_detect_ptr, NULL);
}
EXPORT_SYMBOL_GPL(unregister_ui_task_detect);

int unregister_render_task_detect(void)
{
	return metis_store_hook(&render_task_detect_ptr, NULL);
}
EXPORT_SYMBOL_GPL(unregister_render_task_detect);

int unregister_vip_task_detect(void)
{
	return metis_store_hook(&vip_task_detect_ptr, NULL);
}
EXPORT_SYMBOL_GPL(unregister_vip_task_detect);

int register_mi_walt_get_indicies(void *hook)
{
	return metis_store_hook(&mi_walt_get_indicies_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_mi_walt_get_indicies);

int unregister_mi_walt_get_indicies(void)
{
	return metis_store_hook(&mi_walt_get_indicies_ptr, NULL);
}
EXPORT_SYMBOL_GPL(unregister_mi_walt_get_indicies);

int register_mi_find_energy_efficient_cpu(void *hook)
{
	return metis_store_hook(&mi_find_energy_efficient_cpu_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_mi_find_energy_efficient_cpu);

int unregister_mi_find_energy_efficient_cpu(void)
{
	return metis_store_hook(&mi_find_energy_efficient_cpu_ptr, NULL);
}
EXPORT_SYMBOL_GPL(unregister_mi_find_energy_efficient_cpu);

int register_mi_dequeue_task_fair_hook(void *hook)
{
	return metis_store_hook(&mi_dequeue_task_fair_hook_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_mi_dequeue_task_fair_hook);

int unregister_mi_dequeue_task_fair_hook(void)
{
	return metis_store_hook(&mi_dequeue_task_fair_hook_ptr, NULL);
}
EXPORT_SYMBOL_GPL(unregister_mi_dequeue_task_fair_hook);

int register_mi_sched_enable(void *hook)
{
	return metis_store_hook(&mi_sched_enable_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_mi_sched_enable);

int unregister_mi_sched_enable(void)
{
	return metis_store_hook(&mi_sched_enable_ptr, NULL);
}
EXPORT_SYMBOL_GPL(unregister_mi_sched_enable);

int register_choose_cpu_overutiled(void *hook)
{
	return metis_store_hook(&choose_cpu_overutiled_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_choose_cpu_overutiled);

int unregister_choose_cpu_overutiled(void)
{
	return metis_store_hook(&choose_cpu_overutiled_ptr, NULL);
}
EXPORT_SYMBOL_GPL(unregister_choose_cpu_overutiled);

int register_override_schedboost(void *hook)
{
	return metis_store_hook(&override_schedboost_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_override_schedboost);

int unregister_override_schedboost(void)
{
	return metis_store_hook(&override_schedboost_ptr, NULL);
}
EXPORT_SYMBOL_GPL(unregister_override_schedboost);

int get_metis_limit(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(get_metis_limit);

int get_fboost_duration(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(get_fboost_duration);

#if !IS_ENABLED(CONFIG_XIAOMI_MIGT)
/*int register_soc_update_flt_load(void *hook)
{
	return metis_store_hook(&soc_update_flt_load_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_soc_update_flt_load);*/

/*int unregister_soc_update_flt_load(void)
{
	return metis_store_hook(&soc_update_flt_load_ptr, NULL);
}
EXPORT_SYMBOL_GPL(unregister_soc_update_flt_load);*/

/*int register_soc_update_flt_reset(void *hook)
{
	return metis_store_hook(&soc_update_flt_reset_ptr, hook);
}
EXPORT_SYMBOL_GPL(register_soc_update_flt_reset);*/

/*int unregister_soc_update_flt_reset(void)
{
	return metis_store_hook(&soc_update_flt_reset_ptr, NULL);
}
EXPORT_SYMBOL_GPL(unregister_soc_update_flt_reset);*/
#endif

int metis_update_top_task_info(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_update_top_task_info);

int rq_deadloop_detect(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(rq_deadloop_detect);

int mi_scheduler_tick_hook(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_scheduler_tick_hook);

int mi_schedule_hook(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_schedule_hook);

int metis_doublecyc_init(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_doublecyc_init);

int metis_doublecyc_exit(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_doublecyc_exit);

int metis_lb_pull_tasks(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_lb_pull_tasks);

int metis_lb_find_busiest_cpu(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_lb_find_busiest_cpu);

int metis_lb_init(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_lb_init);

int metis_lb_exit(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_lb_exit);

int metis_tp_is_vip_task(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_tp_is_vip_task);

int metis_tp_mi_viptask_balance(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_tp_mi_viptask_balance);

int metis_tp_check_cpu_layered_load(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_tp_check_cpu_layered_load);

int metis_tp_fixup_cumulative_runnable_avg_hook(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_tp_fixup_cumulative_runnable_avg_hook);

int metis_tp_walt_find_best_target_hook(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_tp_walt_find_best_target_hook);

int metis_tp_init(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_tp_init);

int metis_tp_exit(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_tp_exit);

int metis_schlat_schedule_hook(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_schlat_schedule_hook);

int metis_schlat_dequeue_entity_hook(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_schlat_dequeue_entity_hook);

int metis_schlat_init(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_schlat_init);

int metis_schlat_exit(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_schlat_exit);

int mi_wake_up_new_task_hook(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_wake_up_new_task_hook);

int in_coldstart(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(in_coldstart);

int coldstart_app_tasks(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(coldstart_app_tasks);

int metis_coldstart_skip_thiscpu_rt(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_coldstart_skip_thiscpu_rt);

int metis_coldstart_spread_rt_task(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_coldstart_spread_rt_task);

int metis_coldstart_rt_find_lowest_rq(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_coldstart_rt_find_lowest_rq);

int metis_coldstart_skip_lb(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_coldstart_skip_lb);

int metis_wakeup_cold_start_skip_ui_cpumask(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_wakeup_cold_start_skip_ui_cpumask);

int metis_wakeup_cold_start_skip_ui_cpu(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_wakeup_cold_start_skip_ui_cpu);

int walt_do_sched_yield_hook(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(walt_do_sched_yield_hook);

int metis_set_common_persist_vip(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_set_common_persist_vip);

int metis_common_api_init(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_common_api_init);

int metis_common_api_exit(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(metis_common_api_exit);

/* Non-exported function stubs observed in metis (2).ko. */
static __maybe_unused int fboost_run(void)
{
	return 0;
}

static __maybe_unused int set_mi_vip_task_common_noresched(void)
{
	return 0;
}

static __maybe_unused int set_mi_vip_task_common(void)
{
	return 0;
}

static __maybe_unused int clean_mi_vip_task_common(void)
{
	return 0;
}

static __maybe_unused int mi_vip_task_common(void)
{
	return 0;
}

static __maybe_unused int do_set_binder_vip(void)
{
	return 0;
}

static __maybe_unused int mi_mutex_wait_start_hook(void)
{
	return 0;
}

static __maybe_unused int mi_mutex_unlock_slowpath(void)
{
	return 0;
}

static __maybe_unused int mi_rwsem_list_add_hook(void)
{
	return 0;
}

static __maybe_unused int mi_rwsem_write_wait_start_hook(void)
{
	return 0;
}

static __maybe_unused int mi_rwsem_read_wait_start_hook(void)
{
	return 0;
}

static __maybe_unused int mi_binder_restore_vip_hook(void)
{
	return 0;
}

static __maybe_unused int mi_binder_set_vip_hook(void)
{
	return 0;
}

static __maybe_unused int mi_rwsem_lock_starttime(void)
{
	return 0;
}

static __maybe_unused int mi_mutex_lock_starttime(void)
{
	return 0;
}

static __maybe_unused int mi_rtmutex_lock_starttime(void)
{
	return 0;
}

static __maybe_unused int mi_pcpu_rwsem_lock_starttime(void)
{
	return 0;
}

static __maybe_unused int mi_pcpu_rwsem_wq_add(void)
{
	return 0;
}

static __maybe_unused int mi_rwsem_read_wait_finish_hook(void)
{
	return 0;
}

static __maybe_unused int mi_rwsem_write_wait_finish_hook(void)
{
	return 0;
}

static __maybe_unused int mi_mutex_wait_finish_hook(void)
{
	return 0;
}

static __maybe_unused int mi_rwsem_read_trylock_failed(void)
{
	return 0;
}

static __maybe_unused int mi_lock_rwsem_wake_finish_hook(void)
{
	return 0;
}

static __maybe_unused int metis_link_try_to_wake_up_hook(void)
{
	return 0;
}

static __maybe_unused int mi_alter_futex_plist_add(void)
{
	return 0;
}

static __maybe_unused int mi_do_futex(void)
{
	return 0;
}

static __maybe_unused int mi_futex_sleep_start(void)
{
	return 0;
}

static __maybe_unused int mi_futex_wake_traverse_plist(void)
{
	return 0;
}

static __maybe_unused int mi_futex_wait_end(void)
{
	return 0;
}

static __maybe_unused int set_link_vip_enable(void)
{
	return 0;
}

static __maybe_unused int get_link_vip_enable(void)
{
	return 0;
}

static __maybe_unused int set_async_binder_link_vip_enable(void)
{
	return 0;
}

static __maybe_unused int get_async_binder_link_vip_enable(void)
{
	return 0;
}

static __maybe_unused int do_set_lock_vip(void)
{
	return 0;
}

static __maybe_unused int mi_lock_vip_finish_common(void)
{
	return 0;
}

static __maybe_unused int task_prio_for_rwsem_insert(void)
{
	return 0;
}

static __maybe_unused int do_rwsem_list_insert(void)
{
	return 0;
}

static __maybe_unused int mi_rwsem_wait_start_common(void)
{
	return 0;
}

static __maybe_unused int is_futex_vip_task(void)
{
	return 0;
}

static __maybe_unused int metis_link_ioctl(void)
{
	return 0;
}

static __maybe_unused int metis_link_mmap(void)
{
	return 0;
}

static __maybe_unused int metis_link_open(void)
{
	return 0;
}

static __maybe_unused int metis_link_release(void)
{
	return 0;
}

static __maybe_unused int mi_art_mutex_debug_open(void)
{
	return 0;
}

static __maybe_unused int mi_art_mutex_debug_show(void)
{
	return 0;
}

static __maybe_unused int mi_cpu_coregroup_mask(void)
{
	return 0;
}

static __maybe_unused int mi_cpuset_fork(void)
{
	return 0;
}

static __maybe_unused int mi_sched_setaffinity(void)
{
	return 0;
}

static __maybe_unused int unbind_cpu_affinity_task(void)
{
	return 0;
}

static __maybe_unused int mi_set_cpus_allowed_comm(void)
{
	return 0;
}

static __maybe_unused int set_rebind_task(void)
{
	return 0;
}

static __maybe_unused int get_rebind_task(void)
{
	return 0;
}

static __maybe_unused int clear_rebind_task_list(void)
{
	return 0;
}

static __maybe_unused int add_to_rebind_task_list(void)
{
	return 0;
}

static __maybe_unused int rebind_task_req(void)
{
	return 0;
}

static __maybe_unused int set_clus_affinity_uidlist(void)
{
	return 0;
}

static __maybe_unused int get_clus_affinity_uidlist(void)
{
	return 0;
}

static __maybe_unused int del_uid_from_list(void)
{
	return 0;
}

static __maybe_unused int add_uid_to_list(void)
{
	return 0;
}

static __maybe_unused int set_freedom_cpumask_val(void)
{
	return 0;
}

static __maybe_unused int get_freedom_cpumask_val(void)
{
	return 0;
}

static __maybe_unused int uid_type(void)
{
	return 0;
}

static __maybe_unused int proc_viptask_cpus_set(void)
{
	return 0;
}

static __maybe_unused int get_frame_boost_end(void)
{
	return 0;
}

static __maybe_unused int queue_for_boost_cluster(void)
{
	return 0;
}

static __maybe_unused int do_adjust_freq_delayed(void)
{
	return 0;
}

static __maybe_unused int metis_update_policy_online(void)
{
	return 0;
}

static __maybe_unused int metis_freq_qos_request_init(void)
{
	return 0;
}

static __maybe_unused int cpufreq_notifier_max(void)
{
	return 0;
}

static __maybe_unused int finish_fboost_workfn(void)
{
	return 0;
}

static __maybe_unused int metis_sched_enable(void)
{
	return 0;
}

static __maybe_unused int metis_ui_and_ui_pipeline_select_rq(void)
{
	return 0;
}

static __maybe_unused int mi_sched_choose_cpu_overutiled(void)
{
	return 0;
}

static __maybe_unused int set_mi_vip_task(void)
{
	return 0;
}

static __maybe_unused int set_foreground_pid(void)
{
	return 0;
}

static __maybe_unused int set_mi_dynamic_vip_task(void)
{
	return 0;
}

static __maybe_unused int clean_mi_dynamic_vip_task(void)
{
	return 0;
}

static __maybe_unused int metis_ui_pipeline(void)
{
	return 0;
}

static __maybe_unused int metis_render_pipeline(void)
{
	return 0;
}

static __maybe_unused int metis_set_ui_pipeline(void)
{
	return 0;
}

static __maybe_unused int metis_clean_ui_pipeline(void)
{
	return 0;
}

static __maybe_unused int mi_dynamic_vip_task(void)
{
	return 0;
}

static __maybe_unused int mi_dynamic_vtask_prio_sched_start(void)
{
	return 0;
}

static __maybe_unused int mi_dynamic_vtask_prio_sched_end(void)
{
	return 0;
}

static __maybe_unused int set_mi_req_async_binder_link_task(void)
{
	return 0;
}

static __maybe_unused int mi_req_async_binder_link_sched_start(void)
{
	return 0;
}

static __maybe_unused int clean_mi_req_async_binder_link_task(void)
{
	return 0;
}

static __maybe_unused int mi_req_async_binder_link_sched_end(void)
{
	return 0;
}

static __maybe_unused int mi_preempt_wakeup(void)
{
	return 0;
}

static __maybe_unused int get_preempt_prio(void)
{
	return 0;
}

static __maybe_unused int prio_sched_check(void)
{
	return 0;
}

static __maybe_unused int before_enqueue_vip_task(void)
{
	return 0;
}

static __maybe_unused int metis_ren_task(void)
{
	return 0;
}

static __maybe_unused int dequeue_vip_task(void)
{
	return 0;
}

static __maybe_unused int mi_monitor_hook(void)
{
	return 0;
}

static __maybe_unused int metis_sup_task(void)
{
	return 0;
}

static __maybe_unused int mi_pick_task_fair(void)
{
	return 0;
}

static __maybe_unused int fas_init(void)
{
	return 0;
}

static __maybe_unused int mi_rb_enqueue_entity_hook(void)
{
	return 0;
}

static __maybe_unused int mi_rb_dequeue_entity_hook(void)
{
	return 0;
}

static __maybe_unused int mi_update_deadline_preempt_hook(void)
{
	return 0;
}

static __maybe_unused int fas_exit(void)
{
	return 0;
}

static __maybe_unused int metis_set_ui_task(void)
{
	return 0;
}

static __maybe_unused int metis_ip_task(void)
{
	return 0;
}

static __maybe_unused int is_uuser_set_vip(void)
{
	return 0;
}

static __maybe_unused int is_uuser_set_ip(void)
{
	return 0;
}

static __maybe_unused int set_trace_motion_event(void)
{
	return 0;
}

static __maybe_unused int set_speed_touch_enable(void)
{
	return 0;
}

static __maybe_unused int set_mi_vip_task_req(void)
{
	return 0;
}

static __maybe_unused int get_mi_viptask(void)
{
	return 0;
}

static __maybe_unused int mi_vip_task_req(void)
{
	return 0;
}

static __maybe_unused int set_efboost(void)
{
	return 0;
}

static __maybe_unused int set_mi_efboost(void)
{
	return 0;
}

static __maybe_unused int set_therm_break_enable(void)
{
	return 0;
}

static __maybe_unused int get_therm_break_enable(void)
{
	return 0;
}

static __maybe_unused int proc_open_fas(void)
{
	return 0;
}

static __maybe_unused int proc_write_fas(void)
{
	return 0;
}

static __maybe_unused int proc_show_fas(void)
{
	return 0;
}

static __maybe_unused int fas_suspend(void)
{
	return 0;
}

static __maybe_unused int fas_resume(void)
{
	return 0;
}

static __maybe_unused int metis_xxx_task(void)
{
	return 0;
}

static __maybe_unused int metis_clean_vip_thread(void)
{
	return 0;
}

static __maybe_unused int set_fboost_enable(void)
{
	return 0;
}

static __maybe_unused int get_fboost_enable(void)
{
	return 0;
}

static __maybe_unused int soc_timeout_func(void)
{
	return 0;
}

static __maybe_unused int soc_scroll_timeout_func(void)
{
	return 0;
}

static __maybe_unused int find_energy_hook(void)
{
	return 0;
}

static __maybe_unused int walt_get_indicies_hook(void)
{
	return 0;
}

static __maybe_unused int mi_after_enqueue_task_hook(void)
{
	return 0;
}

static __maybe_unused int mi_after_dequeue_task_hook(void)
{
	return 0;
}

static __maybe_unused int metis_vvip_task(void)
{
	return 0;
}

static __maybe_unused int metis_ui_task_quick(void)
{
	return 0;
}

static __maybe_unused int metis_ioctl(void)
{
	return 0;
}

static __maybe_unused int metis_mmap(void)
{
	return 0;
}

static __maybe_unused int metis_open(void)
{
	return 0;
}

static __maybe_unused int metis_release(void)
{
	return 0;
}

static __maybe_unused int metis_set_vip_thread(void)
{
	return 0;
}

static __maybe_unused int get_thread_type(void)
{
	return 0;
}

static __maybe_unused int history_task_info_init(void)
{
	return 0;
}

static __maybe_unused int runtime_info_debug_open(void)
{
	return 0;
}

static __maybe_unused int runtime_info_debug_show(void)
{
	return 0;
}

static __maybe_unused int do_print_record_abnorm_task_info(void)
{
	return 0;
}

static __maybe_unused int kmsg_runtime_info_debug_show(void)
{
	return 0;
}

static __maybe_unused int pull_tasks_fast_path(void)
{
	return 0;
}

static __maybe_unused int find_vip_busiest_similar_cap_cpu(void)
{
	return 0;
}

static __maybe_unused int get_cpu_vip_longest_runnable_time(void)
{
	return 0;
}

static __maybe_unused int find_vip_runnable_longest_cpu(void)
{
	return 0;
}

static __maybe_unused int get_cpu_layered_load(void)
{
	return 0;
}

static __maybe_unused int get_cpu_vip_tasks_load(void)
{
	return 0;
}

static __maybe_unused int metis_tp_choose_cpu_fastpath(void)
{
	return 0;
}

static __maybe_unused int set_metis_schlat_clear(void)
{
	return 0;
}

static __maybe_unused int get_metis_schlat_clear(void)
{
	return 0;
}

static __maybe_unused int metis_set_schlat_monitor(void)
{
	return 0;
}

static __maybe_unused int metis_get_schlat_monitor(void)
{
	return 0;
}

static __maybe_unused int metis_schlat_show(void)
{
	return 0;
}

static __maybe_unused int get_protect_dur_us(void)
{
	return 0;
}

static __maybe_unused int task_in_cs_protect(void)
{
	return 0;
}

static __maybe_unused int clear_preemption_protect(void)
{
	return 0;
}

static __maybe_unused int task_in_cs(void)
{
	return 0;
}

static __maybe_unused int protect_pre_cs_task(void)
{
	return 0;
}

static __maybe_unused int protect_cs_task(void)
{
	return 0;
}

static __maybe_unused int mark_task_low_lt(void)
{
	return 0;
}

static __maybe_unused int clear_task_low_lt(void)
{
	return 0;
}

static __maybe_unused int is_low_lt_task(void)
{
	return 0;
}

static __maybe_unused int is_preempted_cs_task(void)
{
	return 0;
}

static __maybe_unused int lowlt_list_add_task(void)
{
	return 0;
}

static __maybe_unused int lowlt_list_del_task(void)
{
	return 0;
}

static __maybe_unused int lowlt_list_first_task(void)
{
	return 0;
}

static __maybe_unused int lowlt_tsk_preempt_normal(void)
{
	return 0;
}

static __maybe_unused int set_mutex_vip_blocked(void)
{
	return 0;
}

static __maybe_unused int mutex_reset_data_no_waiter(void)
{
	return 0;
}

static __maybe_unused int vip_blocked_on_mutex(void)
{
	return 0;
}

static __maybe_unused int set_rwsem_vip_blocked(void)
{
	return 0;
}

static __maybe_unused int record_rwsem_blocked_info(void)
{
	return 0;
}

static __maybe_unused int clear_rwsem_blocked_info(void)
{
	return 0;
}

static __maybe_unused int record_mutex_blocked_info(void)
{
	return 0;
}

static __maybe_unused int clear_mutex_blocked_info(void)
{
	return 0;
}

static __maybe_unused int coldstart_app_ui_task(void)
{
	return 0;
}

static __maybe_unused int task_maybe_notify_thread(void)
{
	return 0;
}

static __maybe_unused int register_cs_hooks(void)
{
	return 0;
}

static __maybe_unused int mi_mutex_opt_spin_start(void)
{
	return 0;
}

static __maybe_unused int mi_mutex_opt_spin_finish(void)
{
	return 0;
}

static __maybe_unused int mi_mutex_can_spin_on_owner(void)
{
	return 0;
}

static __maybe_unused int mi_rwsem_opt_spin_start(void)
{
	return 0;
}

static __maybe_unused int mi_rwsem_opt_spin_finish(void)
{
	return 0;
}

static __maybe_unused int mi_rwsem_can_spin_on_owner(void)
{
	return 0;
}

static __maybe_unused int mi_mutex_init(void)
{
	return 0;
}

static __maybe_unused int mi_rwsem_init(void)
{
	return 0;
}

static __maybe_unused int mi_rwsem_direct_rsteal(void)
{
	return 0;
}

static __maybe_unused int mi_rwsem_optimistic_rspin(void)
{
	return 0;
}

static __maybe_unused int unregister_cs_hooks(void)
{
	return 0;
}

static __maybe_unused int clean_coldstart_data(void)
{
	return 0;
}

static __maybe_unused int in_coldstart_period(void)
{
	return 0;
}

static __maybe_unused int coldstart_app_ui_pid(void)
{
	return 0;
}

static __maybe_unused int get_coldstart_app_tgid(void)
{
	return 0;
}

static __maybe_unused int coldstart_app_ren_task(void)
{
	return 0;
}

static __maybe_unused int coldstart_override_schedboost(void)
{
	return 0;
}

static __maybe_unused int coldstart_mark_mmlock_holder(void)
{
	return 0;
}

static __maybe_unused int coldstart_unmark_mmlock_holder(void)
{
	return 0;
}

static __maybe_unused int coldstart_tsk_ready_to_run(void)
{
	return 0;
}

static __maybe_unused int set_cold_start_app_tgid(void)
{
	return 0;
}

static __maybe_unused int register_coldstart_hooks(void)
{
	return 0;
}

static __maybe_unused int unregister_coldstart_hooks(void)
{
	return 0;
}

static __maybe_unused int metis_cs_init(void)
{
	return 0;
}

static __maybe_unused int metis_cs_exit(void)
{
	return 0;
}

static __maybe_unused int rwsem_spin_on_owner(void)
{
	return 0;
}

static __maybe_unused int in_cold_start_wakeup(void)
{
	return 0;
}

static __maybe_unused int cold_start_ui_recent_used_cpu(void)
{
	return 0;
}

static __maybe_unused int cold_start_ui_sched_state(void)
{
	return 0;
}

static __maybe_unused int cold_start_ui_task(void)
{
	return 0;
}

static __maybe_unused int cold_start_competing_with_ui(void)
{
	return 0;
}

static __maybe_unused int cold_start_task_common(void)
{
	return 0;
}

static __maybe_unused int cold_start_wakeup_mark(void)
{
	return 0;
}

static __maybe_unused int cold_start_wakeup_task(void)
{
	return 0;
}

static __maybe_unused int cold_start_ui_pipeline(void)
{
	return 0;
}

static __maybe_unused int cold_start_link_task(void)
{
	return 0;
}

static __maybe_unused int metis_wakeup_ui_pipeline_select_rq(void)
{
	return 0;
}

static __maybe_unused int metis_wakeup_futex_plist_insert(void)
{
	return 0;
}

static __maybe_unused int metis_wakeup_enqueue_task_hook(void)
{
	return 0;
}

static __maybe_unused int set_coldstart_wakeup_task(void)
{
	return 0;
}

static __maybe_unused int wake_up_new_task_hook(void)
{
	return 0;
}

static __maybe_unused int metis_wakeup_try_to_wakeup(void)
{
	return 0;
}

static __maybe_unused int metis_wakeup_clear_wakeup_task(void)
{
	return 0;
}

static __maybe_unused int metis_wakeup_do_futex_hook(void)
{
	return 0;
}

static __maybe_unused int metis_wakeup_init(void)
{
	return 0;
}

static __maybe_unused int metis_wakeup_exit(void)
{
	return 0;
}

static __maybe_unused int set_metis_wakeup_task(void)
{
	return 0;
}

static __maybe_unused int set_common_persist_vip_task(void)
{
	return 0;
}

static __maybe_unused int get_common_persist_vip_task(void)
{
	return 0;
}

static __maybe_unused int set_clear_persist_vip_task(void)
{
	return 0;
}

static int __init metis_init(void)
{
	pr_info("metis: reconstructed external scaffold loaded\n");
	return 0;
}

static void __exit metis_exit(void)
{
	pr_info("metis: reconstructed external scaffold unloaded\n");
}

module_init(metis_init);
module_exit(metis_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("metis-driver by David");
MODULE_INFO(scmversion, "gec8b9cedb7ab");
MODULE_INFO(depends, "mi_schedule,mist");
