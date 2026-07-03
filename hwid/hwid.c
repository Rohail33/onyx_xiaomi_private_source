/*
   hwid.ko module exracted from stock rom 
*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <soc/qcom/socinfo.h>

static unsigned int hwid_value;
static unsigned int project;
static unsigned int build_adc;
static unsigned int project_adc;
static struct kobject *hwid_kobj;

module_param(hwid_value, uint, 0444);
MODULE_PARM_DESC(hwid_value, "xiaomi hwid value correspondingly different build");
module_param(project, uint, 0444);
MODULE_PARM_DESC(project, "xiaomi project serial num predefine");
module_param(build_adc, uint, 0444);
MODULE_PARM_DESC(build_adc, "xiaomi adc value of build resistance");
module_param(project_adc, uint, 0444);
MODULE_PARM_DESC(project_adc, "xiaomi adc value of project resistance");

struct product_entry {
	unsigned int project;
	const char *name;
};

static const struct product_entry product_names[] = {
	{ 0, "unknown" },
	{ 1, "annibale" },
	{ 2, "manet" },
	{ 3, "aurora" },
	{ 4, "cupid" },
	{ 5, "miro" },
	{ 6, "piano" },
	{ 7, "zeus" },
	{ 8, "nirvana" },
	{ 9, "ruyi" },
	{ 10, "caiwei" },
	{ 11, "diting" },
	{ 12, "yupei" },
	{ 13, "chenfeng" },
	{ 14, "babylon" },
	{ 15, "ingres" },
	{ 16, "loki" },
	{ 17, "unicorn" },
	{ 18, "fuxi" },
	{ 19, "luming" },
	{ 20, "wangshu" },
	{ 21, "zizhan" },
	{ 22, "haotian" },
	{ 23, "bixi" },
	{ 24, "onyx" },
	{ 25, "houji" },
	{ 26, "socrates" },
	{ 27, "ziyi" },
	{ 28, "xuanyuan" },
	{ 29, "katyusha" },
	{ 30, "dada" },
	{ 31, "peridot" },
	{ 32, "shennong" },
	{ 33, "thor" },
	{ 34, "suiren" },
	{ 35, "ishtar" },
	{ 36, "mayfly" },
	{ 37, "goku" },
	{ 38, "nuwa" },
};

static const char *lookup_product_name(unsigned int id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(product_names); i++) {
		if (product_names[i].project == id)
			return product_names[i].name;
	}

	return "unknown";
}

const char *product_name_get(void)
{
	return lookup_product_name(project);
}
EXPORT_SYMBOL_GPL(product_name_get);

unsigned int get_hw_project_adc(void)
{
	return project_adc;
}
EXPORT_SYMBOL_GPL(get_hw_project_adc);

unsigned int get_hw_build_adc(void)
{
	return build_adc;
}
EXPORT_SYMBOL_GPL(get_hw_build_adc);

unsigned int get_hw_version_platform(void)
{
	return project;
}
EXPORT_SYMBOL_GPL(get_hw_version_platform);

unsigned int get_hw_id_value(void)
{
	return hwid_value;
}
EXPORT_SYMBOL_GPL(get_hw_id_value);

unsigned int get_hw_country_version(void)
{
	return (hwid_value >> 24) & 0xff;
}
EXPORT_SYMBOL_GPL(get_hw_country_version);

unsigned int get_hw_version_major(void)
{
	return (hwid_value >> 16) & 0xff;
}
EXPORT_SYMBOL_GPL(get_hw_version_major);

unsigned int get_hw_version_minor(void)
{
	return (hwid_value >> 8) & 0xff;
}
EXPORT_SYMBOL_GPL(get_hw_version_minor);

unsigned int get_hw_version_build(void)
{
	return hwid_value & 0xff;
}
EXPORT_SYMBOL_GPL(get_hw_version_build);

static ssize_t hwid_project_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%x\n", project);
}

static ssize_t hwid_value_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%x\n", hwid_value);
}

static ssize_t hwid_project_adc_show(struct kobject *kobj,
				     struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%x\n", project_adc);
}

static ssize_t hwid_build_adc_show(struct kobject *kobj,
				   struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%x\n", build_adc);
}

static struct kobj_attribute hwid_project_attr =
	__ATTR(hwid_project, 0444, hwid_project_show, NULL);
static struct kobj_attribute hwid_value_attr =
	__ATTR(hwid_value, 0444, hwid_value_show, NULL);
static struct kobj_attribute hwid_project_adc_attr =
	__ATTR(hwid_project_adc, 0444, hwid_project_adc_show, NULL);
static struct kobj_attribute hwid_build_adc_attr =
	__ATTR(hwid_build_adc, 0444, hwid_build_adc_show, NULL);

static struct attribute *hwid_attrs[] = {
	&hwid_project_attr.attr,
	&hwid_value_attr.attr,
	&hwid_project_adc_attr.attr,
	&hwid_build_adc_attr.attr,
	NULL,
};

static const struct attribute_group attr_group = {
	.attrs = hwid_attrs,
};

static int __init hwid_init(void)
{
	int ret;


	hwid_kobj = kobject_create_and_add("hwid", kernel_kobj);
	if (!hwid_kobj) {
		pr_err("hwid: hwid module init failed\n");
		return -ENOMEM;
	}

	ret = sysfs_create_group(hwid_kobj, &attr_group);
	if (ret) {
		pr_err("hwid: sysfs register failed\n");
		kobject_del(hwid_kobj);
		hwid_kobj = NULL;
		return ret;
	}

	return 0;
}

static void __exit hwid_exit(void)
{
	if (hwid_kobj) {
		sysfs_remove_group(hwid_kobj, &attr_group);
		kobject_del(hwid_kobj);
		hwid_kobj = NULL;
	}

	pr_info("hwid: hwid module exit success\n");
}

module_init(hwid_init);
module_exit(hwid_exit);

MODULE_AUTHOR("weixiaotian1@xiaomi.com");
MODULE_DESCRIPTION("Hwid Module Driver for Xiaomi Corporation");
MODULE_LICENSE("GPL v2");
