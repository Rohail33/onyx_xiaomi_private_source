// SPDX-License-Identifier: GPL-2.0-only
/*
 * Xiaomi scheduler hook registration surface.
 *
 * These symbols are present in Xiaomi scheduler builds and are consumed by
 * companion vendor modules such as metis, miloadtrace, mi_schedule, and mist.
 * Keep this layer deliberately small: it owns registration storage and exports
 * the ABI surface without changing core scheduler policy by itself.
 */

#include <linux/compiler.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/types.h>

/*
 * Keep Xiaomi hook callbacks disabled by default while bringing up rebuilt
 * scheduler/vendor modules. The vendor modules register callbacks with
 * prototypes that are not recoverable from the exported symbol name alone;
 * storing/calling a callback with the wrong prototype can crash shortly after
 * boot.
 *
 * Set xiaomi_walt_hooks_enable=1 only after the exact hook prototypes have
 * been wired into scheduler call sites.
 */
static bool xiaomi_walt_hooks_enable;
module_param_named(xiaomi_walt_hooks_enable, xiaomi_walt_hooks_enable, bool, 0644);

static bool xiaomi_walt_hooks_warned;

#define xiaomi_store_hook(ptr, cb)					\
	do {								\
		if (xiaomi_walt_hooks_enable) {				\
			WRITE_ONCE(ptr, cb);				\
		} else {						\
			if (!xiaomi_walt_hooks_warned) {			\
				xiaomi_walt_hooks_warned = true;		\
				pr_warn("Xiaomi scheduler callbacks disabled for bring-up\n"); \
			}						\
			WRITE_ONCE(ptr, NULL);				\
		}							\
		smp_wmb();						\
	} while (0)

typedef void (*xiaomi_hook_t)(void);

xiaomi_hook_t mi_update_fravg_f;
int register_mi_update_fravg(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(mi_update_fravg_f, callback);
	return 0;
}
EXPORT_SYMBOL_GPL(register_mi_update_fravg);

int unregister_mi_update_fravg(void)
{
	xiaomi_store_hook(mi_update_fravg_f, NULL);
	return 0;
}
EXPORT_SYMBOL_GPL(unregister_mi_update_fravg);

xiaomi_hook_t mi_update_group_nr;
int register_mi_update_group_nr(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(mi_update_group_nr, callback);
	return 0;
}
EXPORT_SYMBOL_GPL(register_mi_update_group_nr);

int unregister_mi_update_group_nr(void)
{
	xiaomi_store_hook(mi_update_group_nr, NULL);
	return 0;
}
EXPORT_SYMBOL_GPL(unregister_mi_update_group_nr);

xiaomi_hook_t mi_migrate_fload_hooks;
int register_mi_migrate_hook(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(mi_migrate_fload_hooks, callback);
	return 0;
}
EXPORT_SYMBOL_GPL(register_mi_migrate_hook);

int unregister_mi_migrate_hook(void)
{
	xiaomi_store_hook(mi_migrate_fload_hooks, NULL);
	return 0;
}
EXPORT_SYMBOL_GPL(unregister_mi_migrate_hook);

xiaomi_hook_t mi_run_frequpdate_hooks;
int register_mi_run_frequpdate_hooks(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(mi_run_frequpdate_hooks, callback);
	return 0;
}
EXPORT_SYMBOL_GPL(register_mi_run_frequpdate_hooks);

int unregister_mi_run_frequpdate_hooks(void)
{
	xiaomi_store_hook(mi_run_frequpdate_hooks, NULL);
	return 0;
}
EXPORT_SYMBOL_GPL(unregister_mi_run_frequpdate_hooks);

xiaomi_hook_t mi_migarte_fix_fld_hooks;
int register_mi_migarte_fix_fld_hooks(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(mi_migarte_fix_fld_hooks, callback);
	return 0;
}
EXPORT_SYMBOL_GPL(register_mi_migarte_fix_fld_hooks);

int unregister_mi_migarte_fix_fld_hooks(void)
{
	xiaomi_store_hook(mi_migarte_fix_fld_hooks, NULL);
	return 0;
}
EXPORT_SYMBOL_GPL(unregister_mi_migarte_fix_fld_hooks);

xiaomi_hook_t walt_window_update_hooks;
int register_walt_update_hooks(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(walt_window_update_hooks, callback);
	return 0;
}
EXPORT_SYMBOL_GPL(register_walt_update_hooks);

int unregister_walt_update_hooks(void)
{
	xiaomi_store_hook(walt_window_update_hooks, NULL);
	return 0;
}
EXPORT_SYMBOL_GPL(unregister_walt_update_hooks);

xiaomi_hook_t oem_get_ravg_window_hook;
int register_game_ravg_window(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(oem_get_ravg_window_hook, callback);
	return 0;
}
EXPORT_SYMBOL(register_game_ravg_window);

int unregister_game_ravg_window(void)
{
	xiaomi_store_hook(oem_get_ravg_window_hook, NULL);
	return 0;
}
EXPORT_SYMBOL(unregister_game_ravg_window);

xiaomi_hook_t game_vip_hook;
int register_game_vip_hook(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(game_vip_hook, callback);
	return 0;
}
EXPORT_SYMBOL(register_game_vip_hook);

int unregister_game_vip_hook(void)
{
	xiaomi_store_hook(game_vip_hook, NULL);
	return 0;
}
EXPORT_SYMBOL(unregister_game_vip_hook);

xiaomi_hook_t oem_flt_enable_hook;
int register_oem_flt_enable_hook(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(oem_flt_enable_hook, callback);
	return 0;
}
EXPORT_SYMBOL_GPL(register_oem_flt_enable_hook);

int unregister_oem_flt_enable_hook(void)
{
	xiaomi_store_hook(oem_flt_enable_hook, NULL);
	return 0;
}
EXPORT_SYMBOL(unregister_oem_flt_enable_hook);

xiaomi_hook_t oem_flt_wakeup_hook;
int register_oem_flt_wakeup_hook(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(oem_flt_wakeup_hook, callback);
	return 0;
}
EXPORT_SYMBOL(register_oem_flt_wakeup_hook);

int unregister_oem_flt_wakeup_hook(void)
{
	xiaomi_store_hook(oem_flt_wakeup_hook, NULL);
	return 0;
}
EXPORT_SYMBOL(unregister_oem_flt_wakeup_hook);

xiaomi_hook_t oem_update_frame_load_hook;
int register_oem_update_frame_load_hook(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(oem_update_frame_load_hook, callback);
	return 0;
}
EXPORT_SYMBOL_GPL(register_oem_update_frame_load_hook);

int unregister_oem_update_frame_load_hook(void)
{
	xiaomi_store_hook(oem_update_frame_load_hook, NULL);
	return 0;
}
EXPORT_SYMBOL(unregister_oem_update_frame_load_hook);

xiaomi_hook_t oem_set_freq_hook;
int register_oem_freq_hook(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(oem_set_freq_hook, callback);
	return 0;
}
EXPORT_SYMBOL_GPL(register_oem_freq_hook);

int unregister_oem_freq_hook(void)
{
	xiaomi_store_hook(oem_set_freq_hook, NULL);
	return 0;
}
EXPORT_SYMBOL_GPL(unregister_oem_freq_hook);

xiaomi_hook_t soc_update_flt_load_hook;
int register_soc_update_flt_load(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(soc_update_flt_load_hook, callback);
	return 0;
}
EXPORT_SYMBOL(register_soc_update_flt_load);

int unregister_soc_update_flt_load(void)
{
	xiaomi_store_hook(soc_update_flt_load_hook, NULL);
	return 0;
}
EXPORT_SYMBOL(unregister_soc_update_flt_load);

xiaomi_hook_t soc_update_flt_reset_hook;
int register_soc_update_flt_reset(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(soc_update_flt_reset_hook, callback);
	return 0;
}
EXPORT_SYMBOL(register_soc_update_flt_reset);

int unregister_soc_update_flt_reset(void)
{
	xiaomi_store_hook(soc_update_flt_reset_hook, NULL);
	return 0;
}
EXPORT_SYMBOL(unregister_soc_update_flt_reset);

xiaomi_hook_t oem_check_migt_rt_hook;
int register_migt_check_rt_hook(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(oem_check_migt_rt_hook, callback);
	return 0;
}
EXPORT_SYMBOL(register_migt_check_rt_hook);

int unregister_migt_check_rt_hook(void)
{
	xiaomi_store_hook(oem_check_migt_rt_hook, NULL);
	return 0;
}
EXPORT_SYMBOL(unregister_migt_check_rt_hook);

xiaomi_hook_t game_migt_should_skip_migrate_hook;
int register_game_migt_should_skip_migrate_hook(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(game_migt_should_skip_migrate_hook, callback);
	return 0;
}
EXPORT_SYMBOL(register_game_migt_should_skip_migrate_hook);

int unregister_game_migt_should_skip_migrate_hook(void)
{
	xiaomi_store_hook(game_migt_should_skip_migrate_hook, NULL);
	return 0;
}
EXPORT_SYMBOL(unregister_game_migt_should_skip_migrate_hook);

xiaomi_hook_t game_migt_should_skip_this_cpu_rt_hook;
int register_game_migt_should_skip_this_cpu_rt_hook(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(game_migt_should_skip_this_cpu_rt_hook, callback);
	return 0;
}
EXPORT_SYMBOL(register_game_migt_should_skip_this_cpu_rt_hook);

int unregister_game_migt_should_skip_this_cpu_rt_hook(void)
{
	xiaomi_store_hook(game_migt_should_skip_this_cpu_rt_hook, NULL);
	return 0;
}
EXPORT_SYMBOL(unregister_game_migt_should_skip_this_cpu_rt_hook);

xiaomi_hook_t game_migt_skip_cpus_rt_hook;
int register_game_migt_skip_cpus_rt_hook(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(game_migt_skip_cpus_rt_hook, callback);
	return 0;
}
EXPORT_SYMBOL(register_game_migt_skip_cpus_rt_hook);

int unregister_game_migt_skip_cpus_rt_hook(void)
{
	xiaomi_store_hook(game_migt_skip_cpus_rt_hook, NULL);
	return 0;
}
EXPORT_SYMBOL(unregister_game_migt_skip_cpus_rt_hook);

xiaomi_hook_t game_migt_cpus_hook;
int register_game_migt_cpus_hook(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(game_migt_cpus_hook, callback);
	return 0;
}
EXPORT_SYMBOL(register_game_migt_cpus_hook);

int unregister_game_migt_cpus_hook(void)
{
	xiaomi_store_hook(game_migt_cpus_hook, NULL);
	return 0;
}
EXPORT_SYMBOL(unregister_game_migt_cpus_hook);

xiaomi_hook_t game_migt_skip_cpus_hook;
int register_game_migt_skip_cpus_hook(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(game_migt_skip_cpus_hook, callback);
	return 0;
}
EXPORT_SYMBOL(register_game_migt_skip_cpus_hook);

int unregister_game_migt_skip_cpus_hook(void)
{
	xiaomi_store_hook(game_migt_skip_cpus_hook, NULL);
	return 0;
}
EXPORT_SYMBOL(unregister_game_migt_skip_cpus_hook);

xiaomi_hook_t game_migt_exclusive_cpus_hook;
int register_game_migt_exclusive_cpus_hook(void (*callback)(void))
{
	if (!callback)
		return -EINVAL;

	xiaomi_store_hook(game_migt_exclusive_cpus_hook, callback);
	return 0;
}
EXPORT_SYMBOL(register_game_migt_exclusive_cpus_hook);

int unregister_game_migt_exclusive_cpus_hook(void)
{
	xiaomi_store_hook(game_migt_exclusive_cpus_hook, NULL);
	return 0;
}
EXPORT_SYMBOL(unregister_game_migt_exclusive_cpus_hook);
