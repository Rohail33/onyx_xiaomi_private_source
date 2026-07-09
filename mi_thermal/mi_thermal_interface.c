// SPDX-License-Identifier: GPL-2.0
/*
 * Reconstructed Xiaomi thermal control interface.
 *
 * Source reconstructed from mi_thermal_interface.ko metadata, symbols,
 * relocations and sysfs names.  It is not byte-identical original source, but
 * it preserves the observable ABI: sysfs attributes, exported globals,
 * cpu_limits freq_qos behavior, panel/USB notifications and Xiaomi MCA hooks.
 */

/*#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/pm_qos.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/workqueue.h>*/

#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/kdev_t.h>
#include <linux/idr.h>
#include <linux/thermal.h>
#include <linux/reboot.h>
#include <linux/string.h>
#include <linux/of.h>
#include <net/netlink.h>
#include <net/genetlink.h>
#include <linux/suspend.h>
#include <linux/cpu_cooling.h>
#include <linux/soc/qcom/panel_event_notifier.h>
#include <linux/pm_qos.h>
#include <linux/cpufreq.h>
#include <linux/kobject.h>
#include <linux/kernfs.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>



#define MI_THERMAL_BUF_LEN	128
#define MI_THERMAL_CLASS_NODE	"thermal_message"
#define MI_THERMAL_NODE		"thermal-message"
#define MI_POWERSAVE_NODE	"mi_powersave"
#define MI_POWERSAVE_CLASS_NODE	"power_save"

struct drm_panel;

/*
 * Optional Xiaomi hooks.  Stock Xiaomi kernels provide these from
 * mca_smart_charge/mca_event/panel_event_notifier.  Keep them behind Kconfig so
 * the reconstructed module can also build on trees without those drivers.
 */
#if IS_ENABLED(CONFIG_MI_THERMAL_INTERFACE_XIAOMI_HOOKS)
extern int mca_smartchg_set_scene(const char *scene, int value);
extern int mca_event_block_notify(int type, int event, void *data);
#else
static inline int mca_smartchg_set_scene(const char *scene, int value)
{
	return 0;
}

static inline int mca_event_block_notify(int type, int event, void *data)
{
	return 0;
}
#endif

enum {
	MCA_EVENT_TYPE_THERMAL_TEMP = 6,
	MCA_EVENT_THERMAL_BOARD_TEMP_CHANGE = 62,
};



struct freq_table {
	u32 frequency;
};

struct cpufreq_device {
	struct list_head node;
	int id;
	unsigned int cpufreq_state;
	unsigned int max_level;
	struct freq_table *freq_table;
	struct cpufreq_policy *policy;
	struct freq_qos_request *qos_req;
};

struct mi_thermal_device {
	struct kobject *kobj;
	struct kobject *class_kobj;
};

struct mi_thermal_string_attr {
	char value[MI_THERMAL_BUF_LEN];
};

LIST_HEAD(cpufreq_dev_list);
static DEFINE_MUTEX(cpufreq_list_lock);
static DEFINE_PER_CPU(struct freq_qos_request, qos_req);

char *board_sensor;
EXPORT_SYMBOL_GPL(board_sensor);
char *ambient_sensor;
EXPORT_SYMBOL_GPL(ambient_sensor);

static struct workqueue_struct *screen_state_wq;
static struct delayed_work screen_state_dw;
static struct delayed_work usb_state_work;
static struct notifier_block usb_monitor;
static struct power_supply *usb_psy;
//static void *prim_panel;
static void *panel_cookie;

static struct mi_thermal_device mi_thermal_dev;
static struct mi_thermal_device mi_powersave_dev;

static int temp_state;
static atomic_t switch_mode = ATOMIC_INIT(0);
static bool screen_light;
static bool screen_state;
//static int screen_last_status = -1;
static int usb_online;
static int balance_mode;
static int modem_limit;
static int wifi_limit;
static int flash_state;
static int torch_level;
static int torch_real_level;
static int thermal_max_brightness;
static int display_therm_temp;
static int voice_limit;
static int super_hdr;
static int cloud_game;
static int dynamic_tj;
static int market_download_limit;
static int board_sensor_temp_comp_default;
static int poor_modem_limit;
static int cpu_nolimit_temp_default;
static int ntn_limit;
static int modem_level;
static int boot_complete;
static int charger_mode;
static int powersave_mode;
static int power_level;

static bool cpu_limits_enable;
module_param(cpu_limits_enable, bool, 0644);
MODULE_PARM_DESC(cpu_limits_enable,
		 "Enable Xiaomi thermal cpu_limits freq_qos caps (default: off)");

static char boost_buf[MI_THERMAL_BUF_LEN] = "0";
static char board_sensor_temp[MI_THERMAL_BUF_LEN] = "0";
static char board_sensor_second_temp[MI_THERMAL_BUF_LEN] = "0";
static char board_sensor_other_temp[MI_THERMAL_BUF_LEN] = "0";
static char ambient_sensor_temp[MI_THERMAL_BUF_LEN] = "0";

static struct kobject *mi_thermal_get_class_kobj(const char *class_name)
{
	struct kernfs_node *sysfs_sd;
	struct kernfs_node *class_sd;
	struct kernfs_node *target_sd;
	struct kobject *kobj = NULL;

	if (!kernel_kobj || !kernel_kobj->sd || !kernel_kobj->sd->parent)
		return NULL;

	sysfs_sd = kernel_kobj->sd->parent;
	class_sd = kernfs_find_and_get(sysfs_sd, "class");
	if (!class_sd)
		return NULL;

	target_sd = kernfs_find_and_get(class_sd, class_name);
	kernfs_put(class_sd);
	if (!target_sd)
		return NULL;

	if (target_sd->priv)
		kobj = kobject_get(target_sd->priv);
	kernfs_put(target_sd);

	return kobj;
}

static int mi_thermal_create_class_node(struct mi_thermal_device *mdev,
					const char *name,
					const struct attribute_group *group)
{
	struct kobject *thermal_kobj;
	int ret;

	thermal_kobj = mi_thermal_get_class_kobj("thermal");
	if (!thermal_kobj)
		return -ENODEV;

	mdev->class_kobj = kobject_create_and_add(name, thermal_kobj);
	kobject_put(thermal_kobj);
	if (!mdev->class_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(mdev->class_kobj, group);
	if (ret) {
		kobject_put(mdev->class_kobj);
		mdev->class_kobj = NULL;
	}

	return ret;
}

static void mi_thermal_destroy_class_node(struct mi_thermal_device *mdev,
					  const struct attribute_group *group)
{
	if (!mdev->class_kobj)
		return;

	sysfs_remove_group(mdev->class_kobj, group);
	kobject_put(mdev->class_kobj);
	mdev->class_kobj = NULL;
}

static void mi_thermal_notify(struct mi_thermal_device *mdev, const char *attr)
{
	if (mdev->class_kobj)
		sysfs_notify(mdev->class_kobj, NULL, attr);
	if (mdev->kobj)
		sysfs_notify(mdev->kobj, NULL, attr);
}

static ssize_t mi_int_show(int *value, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", *value);
}

static ssize_t mi_int_store(int *value, const char *buf, size_t count)
{
	*value = simple_strtol(buf, NULL, 10);
	return count;
}

static ssize_t mi_string_show(char *value, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n", value);
}

static ssize_t mi_string_store(char *value, const char *buf, size_t count)
{
	size_t len = min(count, (size_t)MI_THERMAL_BUF_LEN - 1);

	memcpy(value, buf, len);
	value[len] = '\0';
	strim(value);
	return count;
}

#define MI_INT_ATTR(_name, _var)						\
static ssize_t thermal_##_name##_show(struct kobject *kobj,		\
				      struct kobj_attribute *attr,	\
				      char *buf)				\
{										\
	return mi_int_show(&_var, buf);					\
}										\
static ssize_t thermal_##_name##_store(struct kobject *kobj,		\
				       struct kobj_attribute *attr,	\
				       const char *buf, size_t count)	\
{										\
	return mi_int_store(&_var, buf, count);				\
}										\
static struct kobj_attribute dev_attr_##_name =				\
	__ATTR(_name, 0664, thermal_##_name##_show, thermal_##_name##_store)

#define MI_STRING_ATTR(_name, _var)					\
static ssize_t thermal_##_name##_show(struct kobject *kobj,		\
				      struct kobj_attribute *attr,	\
				      char *buf)				\
{										\
	return mi_string_show(_var, buf);				\
}										\
static ssize_t thermal_##_name##_store(struct kobject *kobj,		\
				       struct kobj_attribute *attr,	\
				       const char *buf, size_t count)	\
{										\
	return mi_string_store(_var, buf, count);			\
}										\
static struct kobj_attribute dev_attr_##_name =				\
	__ATTR(_name, 0664, thermal_##_name##_show, thermal_##_name##_store)

static int cpufreq_set_level(struct cpufreq_device *cdev, unsigned long state)
{
	if (WARN_ON(state > cdev->max_level))
		return -EINVAL;

	if (cdev->cpufreq_state == state)
		return 0;

	cdev->cpufreq_state = state;
	return freq_qos_update_request(cdev->qos_req,
				       cdev->freq_table[state].frequency);
}

int cpu_limits_set_level(unsigned int cpu, unsigned int max_freq)
{
	struct cpufreq_device *cpufreq_dev;
	unsigned int level;
	int ret = -ENODEV;
	if (!cpu_limits_enable) {
		return 0;
	}

	mutex_lock(&cpufreq_list_lock);
	list_for_each_entry(cpufreq_dev, &cpufreq_dev_list, node) {
		if (cpufreq_dev->id != cpu)
			continue;

		for (level = 0; level <= cpufreq_dev->max_level; level++) {
			unsigned int target_freq =
				cpufreq_dev->freq_table[level].frequency;

			if (max_freq >= target_freq) {
				ret = cpufreq_set_level(cpufreq_dev, level);
				break;
			}
		}
		if (ret == -ENODEV)
			ret = cpufreq_set_level(cpufreq_dev,
						cpufreq_dev->max_level);
		break;
	}
	mutex_unlock(&cpufreq_list_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(cpu_limits_set_level);

static unsigned int find_next_max(struct cpufreq_frequency_table *table,
				  unsigned int prev_max)
{
	struct cpufreq_frequency_table *pos;
	unsigned int max = 0;

	cpufreq_for_each_valid_entry(pos, table) {
		if (pos->frequency > max && pos->frequency < prev_max){
			max = pos->frequency;
		}
	}

	return max;
}

static ssize_t cpu_limits_show(struct kobject *kobj, struct kobj_attribute *attr,
			       char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "cpu%u %u\n", 0, UINT_MAX);
}

static ssize_t cpu_limits_store(struct kobject *kobj, struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	unsigned int cpu, freq;

	if (sscanf(buf, "cpu%u %u", &cpu, &freq) != 2 &&
	    sscanf(buf, "%u %u", &cpu, &freq) != 2) {
		pr_err("input param error, can not prase param\n");
		return -EINVAL;
	}
	if (!cpu_limits_enable) {
		pr_info("mi_thermal_interface: cpu_limits disabled, ignore cpu%u max %u\n",
			cpu, freq);
		return count;
 	}

	cpu_limits_set_level(cpu, freq);
	return count;
}
static struct kobj_attribute dev_attr_cpu_limits =
	__ATTR(cpu_limits, 0664, cpu_limits_show, cpu_limits_store);

static ssize_t thermal_sconfig_show(struct kobject *kobj,
				    struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t thermal_sconfig_store(struct kobject *kobj,
				     struct kobj_attribute *attr,
				     const char *buf, size_t count)
{
	int val;

	val = simple_strtol(buf, NULL, 10);
	atomic_set(&switch_mode, 0);
	return count;
}
static struct kobj_attribute dev_attr_sconfig =
	__ATTR(sconfig, 0664, thermal_sconfig_show, thermal_sconfig_store);

static ssize_t thermal_screen_state_show(struct kobject *kobj,
					 struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", screen_state);
}
static struct kobj_attribute dev_attr_screen_state =
	__ATTR(screen_state, 0444, thermal_screen_state_show, NULL);

static ssize_t thermal_usb_online_show(struct kobject *kobj,
				       struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", usb_online);
}
static struct kobj_attribute dev_attr_usb_online =
	__ATTR(usb_online, 0444, thermal_usb_online_show, NULL);

static ssize_t thermal_board_sensor_show(struct kobject *kobj,
					 struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n", board_sensor ?: "invalid");
}
static struct kobj_attribute dev_attr_board_sensor =
	__ATTR(board_sensor, 0444, thermal_board_sensor_show, NULL);

static ssize_t thermal_ambient_sensor_show(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n", ambient_sensor ?: "invalid");
}
static struct kobj_attribute dev_attr_ambient_sensor =
	__ATTR(ambient_sensor, 0444, thermal_ambient_sensor_show, NULL);

MI_INT_ATTR(temp_state, temp_state);
MI_STRING_ATTR(boost, boost_buf);
MI_STRING_ATTR(board_sensor_second_temp, board_sensor_second_temp);
MI_STRING_ATTR(board_sensor_other_temp, board_sensor_other_temp);
MI_INT_ATTR(balance_mode, balance_mode);
MI_INT_ATTR(modem_limit, modem_limit);
MI_INT_ATTR(wifi_limit, wifi_limit);
MI_INT_ATTR(flash_state, flash_state);
MI_INT_ATTR(torch_level, torch_level);
MI_INT_ATTR(torch_real_level, torch_real_level);
MI_INT_ATTR(thermal_max_brightness, thermal_max_brightness);
MI_INT_ATTR(display_therm_temp, display_therm_temp);
MI_INT_ATTR(voice_limit, voice_limit);
MI_INT_ATTR(super_hdr, super_hdr);
MI_INT_ATTR(cloud_game, cloud_game);
MI_INT_ATTR(dynamic_tj, dynamic_tj);
MI_INT_ATTR(market_download_limit, market_download_limit);
MI_INT_ATTR(board_sensor_temp_comp, board_sensor_temp_comp_default);
MI_INT_ATTR(poor_modem_limit, poor_modem_limit);
MI_INT_ATTR(cpu_nolimit_temp, cpu_nolimit_temp_default);
MI_INT_ATTR(ntn_limit, ntn_limit);
MI_INT_ATTR(modem_level, modem_level);
MI_STRING_ATTR(ambient_sensor_temp, ambient_sensor_temp);
MI_INT_ATTR(boot_complete, boot_complete);
MI_INT_ATTR(charger_temp, charger_mode);
MI_INT_ATTR(powersave_mode, powersave_mode);
MI_INT_ATTR(power_level, power_level);

static ssize_t thermal_board_sensor_temp_show(struct kobject *kobj,
					      struct kobj_attribute *attr,
					      char *buf)
{
	return mi_string_show(board_sensor_temp, buf);
}

static ssize_t thermal_board_sensor_temp_store(struct kobject *kobj,
					       struct kobj_attribute *attr,
					       const char *buf, size_t count)
{
	int temp;

	mi_string_store(board_sensor_temp, buf, count);
	temp = simple_strtol(board_sensor_temp, NULL, 10);

	return count;
}
static struct kobj_attribute dev_attr_board_sensor_temp =
	__ATTR(board_sensor_temp, 0664, thermal_board_sensor_temp_show,
	       thermal_board_sensor_temp_store);

static struct attribute *mi_thermal_attrs[] = {
	&dev_attr_temp_state.attr,
	&dev_attr_cpu_limits.attr,
	&dev_attr_sconfig.attr,
	&dev_attr_screen_state.attr,
	&dev_attr_boost.attr,
	&dev_attr_board_sensor.attr,
	&dev_attr_board_sensor_temp.attr,
	&dev_attr_board_sensor_second_temp.attr,
	&dev_attr_board_sensor_other_temp.attr,
	&dev_attr_balance_mode.attr,
	&dev_attr_modem_limit.attr,
	&dev_attr_wifi_limit.attr,
	&dev_attr_flash_state.attr,
	&dev_attr_torch_level.attr,
	&dev_attr_torch_real_level.attr,
	&dev_attr_thermal_max_brightness.attr,
	&dev_attr_display_therm_temp.attr,
	&dev_attr_voice_limit.attr,
	&dev_attr_super_hdr.attr,
	&dev_attr_cloud_game.attr,
	&dev_attr_dynamic_tj.attr,
	&dev_attr_market_download_limit.attr,
	&dev_attr_board_sensor_temp_comp.attr,
	&dev_attr_poor_modem_limit.attr,
	&dev_attr_cpu_nolimit_temp.attr,
	&dev_attr_ntn_limit.attr,
	&dev_attr_modem_level.attr,
	&dev_attr_ambient_sensor.attr,
	&dev_attr_ambient_sensor_temp.attr,
	&dev_attr_boot_complete.attr,
	&dev_attr_charger_temp.attr,
	&dev_attr_usb_online.attr,
	NULL,
};

static const struct attribute_group mi_thermal_dev_attr_group = {
	.attrs = mi_thermal_attrs,
};

static struct attribute *mi_powersave_attrs[] = {
	&dev_attr_powersave_mode.attr,
	&dev_attr_power_level.attr,
	NULL,
};

static const struct attribute_group mi_powersave_dev_attr_group = {
	.attrs = mi_powersave_attrs,
};

static void usb_online_work(struct work_struct *work)
{
	union power_supply_propval val = { 0 };
	int ret;

	if (!usb_psy)
		usb_psy = power_supply_get_by_name("usb");
	if (!usb_psy)
		return;

	ret = power_supply_get_property(usb_psy, POWER_SUPPLY_PROP_ONLINE,
					&val);
	if (ret) {
		pr_err("usb online read error:%d\n", ret);
		return;
	}

	usb_online = val.intval;
	mi_thermal_notify(&mi_thermal_dev, "usb_online");
}

static int usb_online_callback(struct notifier_block *nb,
			       unsigned long event, void *data)
{
	struct power_supply *psy = data;

	if (psy && !strcmp(psy->desc->name, "usb"))
		queue_delayed_work(system_wq, &usb_state_work, 0);

	return NOTIFY_OK;
}

static void screen_state_check(struct work_struct *work)
{
	bool new_state = screen_light;

	if (screen_state == new_state)
		return;

	screen_state = new_state;
	mi_thermal_notify(&mi_thermal_dev, "screen_state");
}

static void screen_state_for_thermal_callback(enum panel_event_notifier_tag tag,
					      struct panel_event_notification *notif,
					      void *data)
{
	if (!notif)
		return;

	switch (notif->notif_type) {
	case DRM_PANEL_EVENT_UNBLANK:
		screen_light = true;
		break;
	case DRM_PANEL_EVENT_BLANK:
		screen_light = false;
		break;
	default:
		pr_err("%s:Invalid notification\n", __func__);
		return;
	}

	queue_delayed_work(screen_state_wq, &screen_state_dw, 0);
}


static int mi_thermal_init_cpufreq(void)
{
	struct cpufreq_policy *policy;
	struct freq_qos_request *req;
	unsigned int cpu;
	int ret = 0;

	for_each_possible_cpu(cpu) {
		struct cpufreq_device *cpufreq_dev;
		unsigned int count;
		unsigned int freq;
		unsigned int i;

		req = &per_cpu(qos_req, cpu);
		policy = cpufreq_cpu_get(cpu);
		if (!policy) {
			pr_err("%s: cpufreq policy not found for cpu%d\n",
			       __func__, cpu);
			return -ESRCH;
		}
		pr_debug("%s cpu=%d\n", __func__, cpu);

		count = cpufreq_table_count_valid_entries(policy);
		if (!count) {
			pr_debug("%s: CPUFreq table not found or has no valid entries\n",
				 __func__);
			cpufreq_cpu_put(policy);
			return -ENODEV;
		}

		cpufreq_dev = kzalloc(sizeof(*cpufreq_dev), GFP_KERNEL);
		if (!cpufreq_dev) {
			cpufreq_cpu_put(policy);
			return -ENOMEM;
		}

		cpufreq_dev->policy = policy;
		cpufreq_dev->qos_req = req;
		cpufreq_dev->max_level = count - 1;
		cpufreq_dev->id = policy->cpu;
		cpufreq_dev->freq_table = kmalloc_array(count,
						sizeof(*cpufreq_dev->freq_table),
						GFP_KERNEL);
		if (!cpufreq_dev->freq_table) {
			cpufreq_cpu_put(policy);
			kfree(cpufreq_dev);
			return -ENOMEM;
		}

		for (i = 0, freq = UINT_MAX; i <= cpufreq_dev->max_level; i++) {
			freq = find_next_max(policy->freq_table, freq);
			cpufreq_dev->freq_table[i].frequency = freq;

			if (!freq)
				pr_warn("%s: table has duplicate entries\n",
					__func__);
			else
				pr_debug("%s: freq:%u KHz\n", __func__, freq);
		}

		ret = freq_qos_add_request(&policy->constraints,
					   cpufreq_dev->qos_req, FREQ_QOS_MAX,
					   cpufreq_dev->freq_table[0].frequency);
		if (ret < 0) {
			pr_err("%s: Failed to add freq constraint (%d)\n",
			       __func__, ret);
			cpufreq_cpu_put(policy);
			kfree(cpufreq_dev->freq_table);
			kfree(cpufreq_dev);
			return ret;
		}

		mutex_lock(&cpufreq_list_lock);
		list_add(&cpufreq_dev->node, &cpufreq_dev_list);
		mutex_unlock(&cpufreq_list_lock);
	}

	return ret;
}

static void mi_thermal_destroy_cpufreq(void)
{
	struct cpufreq_device *pos, *tmp;

	mutex_lock(&cpufreq_list_lock);
	list_for_each_entry_safe(pos, tmp, &cpufreq_dev_list, node) {
		list_del(&pos->node);
		if (pos->qos_req)
			freq_qos_remove_request(pos->qos_req);
		if (pos->policy)
			cpufreq_cpu_put(pos->policy);
		kfree(pos->freq_table);
		kfree(pos);
	}
	mutex_unlock(&cpufreq_list_lock);
}

static int create_thermal_message_node(void)
{
	int ret;
	int fallback_ret;

	ret = mi_thermal_create_class_node(&mi_thermal_dev,
					   MI_THERMAL_CLASS_NODE,
					   &mi_thermal_dev_attr_group);
	if (ret)
		pr_info("%s: /sys/class/thermal/%s unavailable, using fallback: %d\n",
			__func__, MI_THERMAL_CLASS_NODE, ret);

	mi_thermal_dev.kobj = kobject_create_and_add(MI_THERMAL_NODE,
						     kernel_kobj);
	if (!mi_thermal_dev.kobj) {
		if (!mi_thermal_dev.class_kobj) {
			pr_err("%s ERROR: Cannot create sysfs structure!\n",
			       __func__);
			return -ENOMEM;
		}
		return 0;
	}

	fallback_ret = sysfs_create_group(mi_thermal_dev.kobj,
					  &mi_thermal_dev_attr_group);
	if (fallback_ret) {
		pr_err("%s ERROR: Cannot create sysfs structure!:%d\n",
		       __func__, fallback_ret);
		kobject_put(mi_thermal_dev.kobj);
		mi_thermal_dev.kobj = NULL;
		if (!mi_thermal_dev.class_kobj)
			return fallback_ret;
	}

	return 0;
}

static void destroy_thermal_message_node(void)
{
	pr_info("%s:destroy_thermal_message_node\n", __func__);
	mi_thermal_destroy_class_node(&mi_thermal_dev,
				      &mi_thermal_dev_attr_group);

	if (mi_thermal_dev.kobj) {
		sysfs_remove_group(mi_thermal_dev.kobj,
				   &mi_thermal_dev_attr_group);
		kobject_put(mi_thermal_dev.kobj);
		mi_thermal_dev.kobj = NULL;
		}
}

static int create_mi_powersave_node(void)
{
	int ret;
	int fallback_ret;
	ret = mi_thermal_create_class_node(&mi_powersave_dev,
					   MI_POWERSAVE_CLASS_NODE,
					   &mi_powersave_dev_attr_group);
	if (ret)
		pr_info("%s: /sys/class/thermal/%s unavailable, using fallback: %d\n",
			__func__, MI_POWERSAVE_CLASS_NODE, ret);

	mi_powersave_dev.kobj = kobject_create_and_add(MI_POWERSAVE_NODE,
						       kernel_kobj);
	if (!mi_powersave_dev.kobj) {
		if (!mi_powersave_dev.class_kobj)
			return -ENOMEM;
		return 0;
	}

	fallback_ret = sysfs_create_group(mi_powersave_dev.kobj,
					  &mi_powersave_dev_attr_group);
	if (fallback_ret) {
		kobject_put(mi_powersave_dev.kobj);
		mi_powersave_dev.kobj = NULL;
		if (!mi_powersave_dev.class_kobj)
			return fallback_ret;
	}

	return 0;
}

static void destroy_mi_powersave_node(void)
{
	pr_info("%s:destroy_mi_powersave_node\n", __func__);
	mi_thermal_destroy_class_node(&mi_powersave_dev,
				      &mi_powersave_dev_attr_group);


	if (mi_powersave_dev.kobj) {
		sysfs_remove_group(mi_powersave_dev.kobj,
				   &mi_powersave_dev_attr_group);
		kobject_put(mi_powersave_dev.kobj);
		mi_powersave_dev.kobj = NULL;
		}
}

static int of_parse_thermal_message(void)
{
	struct device_node *np;

	np = of_find_node_by_name(NULL, "thermal-message");
	if (!np) {
		board_sensor = "board-sensor";
		ambient_sensor = "ABT-SENSOR";
		pr_info("%s: thermal-message node missing, using defaults\n",
			__func__);
		return 0;
	}

	of_property_read_string(np, "board-sensor",
				(const char **)&board_sensor);
	of_property_read_string(np, "ambient-sensor",
				(const char **)&ambient_sensor);

	if (!board_sensor)
		board_sensor = "board-sensor";
	if (!ambient_sensor)
		ambient_sensor = "ABT-SENSOR";

	if (board_sensor)
		pr_info("%s board sensor: %s\n", __func__, board_sensor);

	of_node_put(np);
	return 0;
}

static int __init mi_thermal_interface_init(void)
{
	int ret;

	pr_info("%s\n", __func__);
	if (cpu_limits_enable)
		mi_thermal_init_cpufreq();
	else
		pr_info("mi_thermal_interface: cpu_limits disabled by default\n");

	ret = of_parse_thermal_message();
	if (ret)
		pr_info("%s:Thermal: thermal message node unavailable, return %d\n",
			__func__, ret);

	screen_state_wq = alloc_workqueue("screen_state_wq",
					  WQ_UNBOUND | WQ_HIGHPRI, 0);
	if (!screen_state_wq)
		return -ENOMEM;

	INIT_DELAYED_WORK(&screen_state_dw, screen_state_check);
	INIT_DELAYED_WORK(&usb_state_work, usb_online_work);

	ret = create_thermal_message_node();
	if (ret)
		goto err_wq;

	ret = create_mi_powersave_node();
	if (ret)
		pr_err("%s:Thermal: Can not parse mi_powersave node, return %d\n",
		       __func__, ret);


	usb_monitor.notifier_call = usb_online_callback;
	ret = power_supply_reg_notifier(&usb_monitor);
	if (ret)
		pr_err("usb online notifier registration error. return: %d\n",
		       ret);

	panel_cookie = panel_event_notifier_register(
			PANEL_EVENT_NOTIFICATION_PRIMARY,
			PANEL_EVENT_NOTIFIER_CLIENT_THERMAL,
			NULL, screen_state_for_thermal_callback, NULL);
	if (!panel_cookie)
		pr_err("%s:Failed to register for prim_panel events\n",
		       __func__);

	return 0;

err_wq:
	destroy_workqueue(screen_state_wq);
	screen_state_wq = NULL;
	return ret;
}
module_init(mi_thermal_interface_init);

static void __exit mi_thermal_interface_exit(void)
{
	pr_info("%s\n", __func__);

	cancel_delayed_work_sync(&screen_state_dw);
	cancel_delayed_work_sync(&usb_state_work);
	if (screen_state_wq)
		destroy_workqueue(screen_state_wq);


	if (panel_cookie)
		panel_event_notifier_unregister(panel_cookie);

	destroy_thermal_message_node();
	destroy_mi_powersave_node();
	mi_thermal_destroy_cpufreq();
}
module_exit(mi_thermal_interface_exit);

MODULE_AUTHOR("Xiaomi thermal team");
MODULE_DESCRIPTION("Xiaomi thermal control interface");
MODULE_LICENSE("GPL v2");
