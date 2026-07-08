// SPDX-License-Identifier: GPL-2.0
/*
 * Reconstructed Xiaomi thermal control interface.
 *
 * Source reconstructed from mi_thermal_interface.ko metadata, symbols,
 * relocations and sysfs names.  It is not byte-identical original source, but
 * it preserves the observable ABI: sysfs attributes, exported globals,
 * cpu_limits freq_qos behavior, panel/USB notifications and Xiaomi MCA hooks.
 */

#include <linux/cpu.h>
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
#include <linux/workqueue.h>

#define MI_THERMAL_BUF_LEN	128
#define MI_THERMAL_NODE		"thermal-message"
#define MI_POWERSAVE_NODE	"mi_powersave"

struct drm_panel;

/*
 * Optional Xiaomi hooks.  Stock Xiaomi kernels provide these from
 * mca_smart_charge/mca_event/panel_event_notifier.  Keep them behind Kconfig so
 * the reconstructed module can also build on trees without those drivers.
 */
#if IS_ENABLED(CONFIG_MI_THERMAL_INTERFACE_XIAOMI_HOOKS)
extern int mca_smartchg_set_scene(const char *scene, int value);
extern int mca_event_block_notify(int type, int event, void *data);
extern void *panel_event_notifier_register(unsigned int tag,
					   unsigned int client,
					   struct device_node *node,
					   void *cb, void *data);
extern void panel_event_notifier_unregister(void *cookie);
extern struct drm_panel *of_drm_find_panel(const struct device_node *np);
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

enum {
	PANEL_EVENT_NOTIFICATION_PRIMARY = 1,
	PANEL_EVENT_NOTIFIER_CLIENT_THERMAL = 6,
	DRM_PANEL_EVENT_BLANK = 1,
	DRM_PANEL_EVENT_UNBLANK = 2,
};

struct panel_event_notification {
	unsigned int notif_type;
	void *notif_data;
};

struct cpufreq_device {
	struct list_head node;
	unsigned int cpu;
	struct cpufreq_policy *policy;
	struct freq_qos_request qos_req;
	bool qos_added;
};

struct mi_thermal_device {
	struct kobject *kobj;
};

struct mi_thermal_string_attr {
	char value[MI_THERMAL_BUF_LEN];
};

LIST_HEAD(cpufreq_dev_list);
static DEFINE_MUTEX(cpufreq_list_lock);

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
static atomic_t switch_mode = ATOMIC_INIT(-1);
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

static char boost_buf[MI_THERMAL_BUF_LEN] = "0";
static char board_sensor_temp[MI_THERMAL_BUF_LEN] = "0";
static char board_sensor_second_temp[MI_THERMAL_BUF_LEN] = "0";
static char board_sensor_other_temp[MI_THERMAL_BUF_LEN] = "0";
static char ambient_sensor_temp[MI_THERMAL_BUF_LEN] = "0";

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

int cpu_limits_set_level(unsigned int cpu, unsigned int max_freq)
{
	struct cpufreq_device *pos;
	int ret = -ENODEV;

	mutex_lock(&cpufreq_list_lock);
	list_for_each_entry(pos, &cpufreq_dev_list, node) {
		if (pos->cpu != cpu)
			continue;

		if (!pos->qos_added) {
			ret = -EINVAL;
			break;
		}

		ret = freq_qos_update_request(&pos->qos_req, max_freq);
		if (ret < 0)
			pr_err("%s: Failed to update freq constraint cpu=%u freq=%u ret=%d\n",
			       __func__, cpu, max_freq, ret);
		break;
	}
	mutex_unlock(&cpufreq_list_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(cpu_limits_set_level);

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

	cpu_limits_set_level(cpu, freq);
	return count;
}
static struct kobj_attribute dev_attr_cpu_limits =
	__ATTR(cpu_limits, 0664, cpu_limits_show, cpu_limits_store);

static ssize_t thermal_sconfig_show(struct kobject *kobj,
				    struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&switch_mode));
}

static ssize_t thermal_sconfig_store(struct kobject *kobj,
				     struct kobj_attribute *attr,
				     const char *buf, size_t count)
{
	int val;

	val = simple_strtol(buf, NULL, 10);
	atomic_set(&switch_mode, val);
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
	if (mi_thermal_dev.kobj)
		sysfs_notify(mi_thermal_dev.kobj, NULL, "usb_online");
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
	if (mi_thermal_dev.kobj)
		sysfs_notify(mi_thermal_dev.kobj, NULL, "screen_state");
}

#if IS_ENABLED(CONFIG_MI_THERMAL_INTERFACE_XIAOMI_HOOKS)
static void screen_state_for_thermal_callback(void *data,
					      struct panel_event_notification *notif)
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
#endif

static int mi_thermal_init_cpufreq(void)
{
	struct cpufreq_device *cdev;
	struct cpufreq_policy *policy;
	unsigned int cpu;
	int ret;

	for_each_possible_cpu(cpu) {
		policy = cpufreq_cpu_get(cpu);
		if (!policy) {
			pr_err("%s: cpufreq policy not found for cpu%d\n",
			       __func__, cpu);
			continue;
		}

		cdev = kzalloc(sizeof(*cdev), GFP_KERNEL);
		if (!cdev) {
			cpufreq_cpu_put(policy);
			return -ENOMEM;
		}

		cdev->cpu = cpu;
		cdev->policy = policy;
		ret = freq_qos_add_request(&policy->constraints, &cdev->qos_req,
					   FREQ_QOS_MAX,
					   FREQ_QOS_MAX_DEFAULT_VALUE);
		if (ret < 0) {
			pr_err("%s: Failed to add freq constraint (%d)\n",
			       __func__, ret);
			cpufreq_cpu_put(policy);
			kfree(cdev);
			continue;
		}

		cdev->qos_added = true;
		mutex_lock(&cpufreq_list_lock);
		list_add(&cdev->node, &cpufreq_dev_list);
		mutex_unlock(&cpufreq_list_lock);
	}

	return 0;
}

static void mi_thermal_destroy_cpufreq(void)
{
	struct cpufreq_device *pos, *tmp;

	mutex_lock(&cpufreq_list_lock);
	list_for_each_entry_safe(pos, tmp, &cpufreq_dev_list, node) {
		list_del(&pos->node);
		if (pos->qos_added)
			freq_qos_remove_request(&pos->qos_req);
		if (pos->policy)
			cpufreq_cpu_put(pos->policy);
		kfree(pos);
	}
	mutex_unlock(&cpufreq_list_lock);
}

static int create_thermal_message_node(void)
{
	int ret;

	mi_thermal_dev.kobj = kobject_create_and_add(MI_THERMAL_NODE,
						     kernel_kobj);
	if (!mi_thermal_dev.kobj) {
		pr_err("%s ERROR: Cannot create sysfs structure!\n", __func__);
		return -ENOMEM;
	}

	ret = sysfs_create_group(mi_thermal_dev.kobj, &mi_thermal_dev_attr_group);
	if (ret) {
		pr_err("%s ERROR: Cannot create sysfs structure!:%d\n",
		       __func__, ret);
		kobject_put(mi_thermal_dev.kobj);
		mi_thermal_dev.kobj = NULL;
		return ret;
	}

	return 0;
}

static void destroy_thermal_message_node(void)
{
	pr_info("%s:destroy_thermal_message_node\n", __func__);
	if (!mi_thermal_dev.kobj)
		return;

	sysfs_remove_group(mi_thermal_dev.kobj, &mi_thermal_dev_attr_group);
	kobject_put(mi_thermal_dev.kobj);
	mi_thermal_dev.kobj = NULL;
}

static int create_mi_powersave_node(void)
{
	int ret;

	mi_powersave_dev.kobj = kobject_create_and_add(MI_POWERSAVE_NODE,
						       kernel_kobj);
	if (!mi_powersave_dev.kobj)
		return -ENOMEM;

	ret = sysfs_create_group(mi_powersave_dev.kobj,
				 &mi_powersave_dev_attr_group);
	if (ret) {
		kobject_put(mi_powersave_dev.kobj);
		mi_powersave_dev.kobj = NULL;
		return ret;
	}

	return 0;
}

static void destroy_mi_powersave_node(void)
{
	pr_info("%s:destroy_mi_powersave_node\n", __func__);
	if (!mi_powersave_dev.kobj)
		return;

	sysfs_remove_group(mi_powersave_dev.kobj, &mi_powersave_dev_attr_group);
	kobject_put(mi_powersave_dev.kobj);
	mi_powersave_dev.kobj = NULL;
}

static int of_parse_thermal_message(void)
{
	struct device_node *np;

	np = of_find_node_by_name(NULL, "thermal-message");
	if (!np)
		return -ENODEV;

	of_property_read_string(np, "board-sensor",
				(const char **)&board_sensor);
	of_property_read_string(np, "ambient-sensor",
				(const char **)&ambient_sensor);

	if (board_sensor)
		pr_info("%s board sensor: %s\n", __func__, board_sensor);

	of_node_put(np);
	return 0;
}

static int __init mi_thermal_interface_init(void)
{
	int ret;

	pr_info("%s\n", __func__);

	ret = of_parse_thermal_message();
	if (ret)
		pr_err("%s:Thermal: Can not parse thermal message node, return %d\n",
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

	mi_thermal_init_cpufreq();

	usb_monitor.notifier_call = usb_online_callback;
	ret = power_supply_reg_notifier(&usb_monitor);
	if (ret)
		pr_err("usb online notifier registration error. return: %d\n",
		       ret);

#if IS_ENABLED(CONFIG_MI_THERMAL_INTERFACE_XIAOMI_HOOKS)
	panel_cookie = panel_event_notifier_register(
			PANEL_EVENT_NOTIFICATION_PRIMARY,
			PANEL_EVENT_NOTIFIER_CLIENT_THERMAL,
			NULL, screen_state_for_thermal_callback, NULL);
	if (!panel_cookie)
		pr_err("%s:Failed to register for prim_panel events\n",
		       __func__);
#endif

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

#if IS_ENABLED(CONFIG_MI_THERMAL_INTERFACE_XIAOMI_HOOKS)
	if (panel_cookie)
		panel_event_notifier_unregister(panel_cookie);
#endif

	destroy_thermal_message_node();
	destroy_mi_powersave_node();
	mi_thermal_destroy_cpufreq();
}
module_exit(mi_thermal_interface_exit);

MODULE_AUTHOR("Xiaomi thermal team");
MODULE_DESCRIPTION("Xiaomi thermal control interface");
MODULE_LICENSE("GPL v2");
