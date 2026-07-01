// SPDX-License-Identifier: GPL-2.0-only
/*
	mist module decompiled from stock rom module
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/cpumask.h>
#include <linux/atomic.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/of.h>

#define MIST_PARAM_ARRAY_SIZE 8

static void *mist_soc_freq_update_hook;
static void *mist_dflt_bw_update_cb;
static void *mist_dflt_lat_update_cb;
static atomic_t mist_enabled = ATOMIC_INIT(1);

static unsigned int dflt_ftrace_debug;
module_param(dflt_ftrace_debug, uint, 0644);
static unsigned int dflt_debug;
module_param(dflt_debug, uint, 0644);
static unsigned int need_boost_in_frame;
module_param(need_boost_in_frame, uint, 0644);
static unsigned int dflt_fpercent_other;
module_param(dflt_fpercent_other, uint, 0644);
static unsigned int max_ddr_ib;
module_param(max_ddr_ib, uint, 0644);
static unsigned int spm_thres;
module_param(spm_thres, uint, 0644);
static unsigned int dflt_ddr_ib_scale;
module_param(dflt_ddr_ib_scale, uint, 0644);
static unsigned int dflt_ddr_boost;
module_param(dflt_ddr_boost, uint, 0644);
static unsigned long dflt_cpu_mask_debug;
module_param(dflt_cpu_mask_debug, ulong, 0644);
static unsigned int dflt_spm_low_threshold;
module_param(dflt_spm_low_threshold, uint, 0644);
static unsigned int dflt_ddr_ib_scale_low;
module_param(dflt_ddr_ib_scale_low, uint, 0644);
static unsigned int dflt_ddr_ib_scale_high;
module_param(dflt_ddr_ib_scale_high, uint, 0644);
static unsigned int dflt_spm_high_threshold;
module_param(dflt_spm_high_threshold, uint, 0644);
static unsigned int dflt_memlat_vote_type;
module_param(dflt_memlat_vote_type, uint, 0644);
static unsigned int mist_memlat_vote_enable;
module_param(mist_memlat_vote_enable, uint, 0644);
static unsigned int dflt_stallratio_thresh[MIST_PARAM_ARRAY_SIZE];
static unsigned int dflt_stallratio_thresh_num;
module_param_array(dflt_stallratio_thresh, uint, &dflt_stallratio_thresh_num, 0644);
static unsigned int dflt_spm_thresh[MIST_PARAM_ARRAY_SIZE];
static unsigned int dflt_spm_thresh_num;
module_param_array(dflt_spm_thresh, uint, &dflt_spm_thresh_num, 0644);
static unsigned int dflt_ips_thresh[MIST_PARAM_ARRAY_SIZE];
static unsigned int dflt_ips_thresh_num;
module_param_array(dflt_ips_thresh, uint, &dflt_ips_thresh_num, 0644);
static char *mist_version = "3.5.0";
module_param(mist_version, charp, 0644);
static unsigned int be_procent;
module_param(be_procent, uint, 0644);
static unsigned int mist_soc_min_stall;
module_param(mist_soc_min_stall, uint, 0644);
static unsigned int mist_set_uid;
module_param(mist_set_uid, uint, 0644);
static unsigned int mist_soc_freq_thresh[MIST_PARAM_ARRAY_SIZE];
static unsigned int mist_soc_freq_thresh_num;
module_param_array(mist_soc_freq_thresh, uint, &mist_soc_freq_thresh_num, 0644);
static unsigned int gflt_debug;
module_param(gflt_debug, uint, 0644);
static unsigned int sync_cost_thresh;
module_param(sync_cost_thresh, uint, 0644);
static unsigned int gflt_slack_time_jif;
module_param(gflt_slack_time_jif, uint, 0644);
static unsigned int gflt_update_freq_failed;
module_param(gflt_update_freq_failed, uint, 0644);
static unsigned int lc_debug;
module_param(lc_debug, uint, 0644);
static unsigned int gflt_wait_threshold;
module_param(gflt_wait_threshold, uint, 0644);
static unsigned int dflt_bm_counter_enable;
module_param(dflt_bm_counter_enable, uint, 0644);
static unsigned int dflt_bw_enable;
module_param(dflt_bw_enable, uint, 0644);
static unsigned int dflt_lat_enable;
module_param(dflt_lat_enable, uint, 0644);
static unsigned int mist_soc_api_enable;
module_param(mist_soc_api_enable, uint, 0644);
static unsigned int gflt_enable;
module_param(gflt_enable, uint, 0644);


static int mist_store_hook(void **slot, void *hook)
{
	WRITE_ONCE(*slot, hook);
	smp_wmb();
	return 0;
}

unsigned int dlft_get_latency(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(dlft_get_latency);

unsigned int get_adreno_gflt_load(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(get_adreno_gflt_load);

unsigned int get_gpubw_gflt_load(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(get_gpubw_gflt_load);

void gpu_load_calibration(void)
{
}
EXPORT_SYMBOL_GPL(gpu_load_calibration);

void gpubw_load_calibration(void)
{
}
EXPORT_SYMBOL_GPL(gpubw_load_calibration);

void mi_dflt_bwmon_update_cur_freq_hook(void)
{
}
EXPORT_SYMBOL_GPL(mi_dflt_bwmon_update_cur_freq_hook);

void mi_dflt_calculate_dflt_lantancy_hook(void)
{
}
EXPORT_SYMBOL_GPL(mi_dflt_calculate_dflt_lantancy_hook);

void mi_dflt_calculate_mon_sampling_freq_hook(void)
{
}
EXPORT_SYMBOL_GPL(mi_dflt_calculate_mon_sampling_freq_hook);

unsigned int mi_dflt_get_bwmon_enable(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_dflt_get_bwmon_enable);

unsigned int mi_dflt_get_bwmon_sample_start(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_dflt_get_bwmon_sample_start);

unsigned int mi_dflt_get_ddr_frame_load(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_dflt_get_ddr_frame_load);

unsigned int mi_dflt_get_latency_enable(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_dflt_get_latency_enable);

unsigned int mi_dflt_get_latency_enable_from_user(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_dflt_get_latency_enable_from_user);

unsigned int mi_dflt_get_latency_sample_start(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_dflt_get_latency_sample_start);

unsigned int mi_dflt_get_latency_vote_start(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_dflt_get_latency_vote_start);

void mi_dflt_process_raw_ctrs_hook(void)
{
}
EXPORT_SYMBOL_GPL(mi_dflt_process_raw_ctrs_hook);

void mi_dflt_schedule_hook(void)
{
}
EXPORT_SYMBOL_GPL(mi_dflt_schedule_hook);

int mi_dflt_set_bwmon_enable(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_dflt_set_bwmon_enable);

int mi_dflt_set_bwmon_sample_start(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_dflt_set_bwmon_sample_start);

int mi_dflt_set_latency_enable(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_dflt_set_latency_enable);

int mi_dflt_set_latency_sample_start(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_dflt_set_latency_sample_start);

int mi_dflt_set_latency_vote_start(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_dflt_set_latency_vote_start);

void mi_dflt_update_memlat_fp_vote_hook(void)
{
}
EXPORT_SYMBOL_GPL(mi_dflt_update_memlat_fp_vote_hook);

unsigned int mi_gpu_flt_working(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mi_gpu_flt_working);

int mi_soc_register_freq_adjhook(void *hook)
{
	return mist_store_hook(&mist_soc_freq_update_hook, hook);
}
EXPORT_SYMBOL_GPL(mi_soc_register_freq_adjhook);

int mist_ddr_vote_enable(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mist_ddr_vote_enable);

void mist_dflt_init(void)
{
}
EXPORT_SYMBOL_GPL(mist_dflt_init);

void mist_gflt_init(void)
{
}
EXPORT_SYMBOL_GPL(mist_gflt_init);

void mist_gflt_start(void)
{
}
EXPORT_SYMBOL_GPL(mist_gflt_start);

void mist_gflt_stop(void)
{
}
EXPORT_SYMBOL_GPL(mist_gflt_stop);

unsigned int mist_perf_api_enable(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(mist_perf_api_enable);

void mist_set_next_freq_hook(void)
{
}
EXPORT_SYMBOL_GPL(mist_set_next_freq_hook);

void mist_update_ddr_freq(void)
{
}
EXPORT_SYMBOL_GPL(mist_update_ddr_freq);

void mist_update_rel_freq(void)
{
}
EXPORT_SYMBOL_GPL(mist_update_rel_freq);

int notify_gflt_frame_period_thresh(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(notify_gflt_frame_period_thresh);

int register_mi_dflt_bw_update_cb(void *hook)
{
	return mist_store_hook(&mist_dflt_bw_update_cb, hook);
}
EXPORT_SYMBOL_GPL(register_mi_dflt_bw_update_cb);

int register_mi_dflt_lat_update_cb(void *hook)
{
	return mist_store_hook(&mist_dflt_lat_update_cb, hook);
}
EXPORT_SYMBOL_GPL(register_mi_dflt_lat_update_cb);

int set_adreno_gflt_load(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(set_adreno_gflt_load);

int set_gpubw_gflt_load(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(set_gpubw_gflt_load);

int trigger_mist_dflt(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(trigger_mist_dflt);

int trigger_mist_gflt(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(trigger_mist_gflt);

int trigger_mist_load_tracking(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(trigger_mist_load_tracking);

int unregister_mi_dflt_bw_update_cb(void)
{
	return mist_store_hook(&mist_dflt_bw_update_cb, NULL);
}
EXPORT_SYMBOL_GPL(unregister_mi_dflt_bw_update_cb);

int unregister_mi_dflt_lat_update_cb(void)
{
	return mist_store_hook(&mist_dflt_lat_update_cb, NULL);
}
EXPORT_SYMBOL_GPL(unregister_mi_dflt_lat_update_cb);

/* Non-exported function stubs observed in mist.ko. */
static __maybe_unused int cpufreq_to_memfreq(void)
{
	return 0;
}

static __maybe_unused int queue_dflt_sample_start(void)
{
	return 0;
}

static __maybe_unused int queue_dflt_vote_start(void)
{
	return 0;
}

static __maybe_unused int queue_memlat_vote_start(void)
{
	return 0;
}

static __maybe_unused int parse_cluster_node_map(void)
{
	return 0;
}

static __maybe_unused int commit_fast_vote(void)
{
	return 0;
}

static __maybe_unused int set_dflt_bm_counter_enable(void)
{
	return 0;
}

static __maybe_unused int get_dflt_bm_counter_enable(void)
{
	return 0;
}

static __maybe_unused int set_dflt_bwmon_enable(void)
{
	return 0;
}

static __maybe_unused int get_dflt_bwmon_enable(void)
{
	return 0;
}

static __maybe_unused int set_dflt_latency_enable(void)
{
	return 0;
}

static __maybe_unused int get_dflt_latency_enable(void)
{
	return 0;
}

static __maybe_unused int set_soc_api_enable(void)
{
	return 0;
}

static __maybe_unused int get_soc_api_enable(void)
{
	return 0;
}

static __maybe_unused int history_load_open(void)
{
	return 0;
}

static __maybe_unused int history_load_show(void)
{
	return 0;
}

static __maybe_unused int dump_his_load_info(void)
{
	return 0;
}

static __maybe_unused int mi_get_gflt_enable(void)
{
	return 0;
}

static __maybe_unused int do_freq_adjwork(void)
{
	return 0;
}

static __maybe_unused int do_slack_work(void)
{
	return 0;
}

static __maybe_unused int set_gflt_enable(void)
{
	return 0;
}

static __maybe_unused int get_gflt_enable(void)
{
	return 0;
}

static __maybe_unused int cpu_load_calibration(void)
{
	return 0;
}

static __maybe_unused int ddr_load_calibration(void)
{
	return 0;
}

static int __init mist_init(void)
{
	atomic_set(&mist_enabled, 1);
	pr_info("mist: reconstructed external scaffold loaded\n");
	return 0;
}

static void __exit mist_exit(void)
{
	pr_info("mist: reconstructed external scaffold unloaded\n");
}

module_init(mist_init);
module_exit(mist_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("3.5.0");
MODULE_DESCRIPTION("soc performance tuner developed by David");
MODULE_INFO(scmversion, "gec8b9cedb7ab");
MODULE_INFO(intree, "Y");
MODULE_INFO(depends, "clk-qcom,qcom-pmu-lib,qcom-dcvs");
MODULE_INFO(srcversion, "2C6676ABECA7BFD2C0DE30B");
