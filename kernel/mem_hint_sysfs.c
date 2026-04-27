/* SPDX-License-Identifier: GPL-2.0-only
 * mem_hint_sysfs.c - sysfs control plane for AI Memory Systems
 * Patent Pending: Indian Patent Application No. 202641053160
 * Inventor: Manish KL, filed 26 April 2026
 * Reference implementation - sysfs tree models the interface contract.
 */

#include <linux/device.h>
#include <linux/sysfs.h>

#include "mem_hint.h"

u32 decode_trcd = 18;
u32 decode_vswing_mv = 280;
u32 decode_dfe_tap1 = 0x14;
u32 prefill_vswing_mv = 300;
s32 prefill_ctle_gain_db = 2;
u32 agentic_priority = 5;
u32 idle_pll_reduction = 1;
u32 idle_vswing_mv = 240;

static ssize_t u32_show(struct device *dev, char *buf, u32 *value)
{
	(void)dev;
	return sysfs_emit(buf, "%u\n", *value);
}

static ssize_t s32_show(struct device *dev, char *buf, s32 *value)
{
	(void)dev;
	return sysfs_emit(buf, "%d\n", *value);
}

static int parse_u32_value(const char *buf, unsigned long min,
			   unsigned long max, u32 *value)
{
	unsigned long parsed;
	int ret;

	ret = kstrtoul(buf, 0, &parsed);
	if (ret)
		return ret;
	if (parsed < min || parsed > max)
		return -ERANGE;

	*value = (u32)parsed;
	return 0;
}

static int parse_s32_value(const char *buf, long min, long max, s32 *value)
{
	long parsed;
	int ret;

	ret = kstrtol(buf, 0, &parsed);
	if (ret)
		return ret;
	if (parsed < min || parsed > max)
		return -ERANGE;

	*value = (s32)parsed;
	return 0;
}

#define MEM_HINT_RW_U32(_name, _var, _min, _max)				\
static ssize_t _name##_show(struct device *dev,				\
			    struct device_attribute *attr, char *buf)	\
{										\
	(void)attr;								\
	return u32_show(dev, buf, &_var);					\
}										\
static ssize_t _name##_store(struct device *dev,				\
			     struct device_attribute *attr,			\
			     const char *buf, size_t count)			\
{										\
	int ret;								\
										\
	(void)dev;								\
	(void)attr;								\
	ret = parse_u32_value(buf, _min, _max, &_var);				\
	if (ret)								\
		return ret;							\
	return count;								\
}										\
static DEVICE_ATTR_RW(_name)

#define MEM_HINT_RW_S32(_name, _var, _min, _max)				\
static ssize_t _name##_show(struct device *dev,				\
			    struct device_attribute *attr, char *buf)	\
{										\
	(void)attr;								\
	return s32_show(dev, buf, &_var);					\
}										\
static ssize_t _name##_store(struct device *dev,				\
			     struct device_attribute *attr,			\
			     const char *buf, size_t count)			\
{										\
	int ret;								\
										\
	(void)dev;								\
	(void)attr;								\
	ret = parse_s32_value(buf, _min, _max, &_var);				\
	if (ret)								\
		return ret;							\
	return count;								\
}										\
static DEVICE_ATTR_RW(_name)

MEM_HINT_RW_U32(decode_trcd, decode_trcd, 14, 32);
MEM_HINT_RW_U32(decode_vswing_mv, decode_vswing_mv, 200, 400);
MEM_HINT_RW_U32(decode_dfe_tap1, decode_dfe_tap1, 0, 255);
MEM_HINT_RW_U32(prefill_vswing_mv, prefill_vswing_mv, 200, 400);
MEM_HINT_RW_S32(prefill_ctle_gain_db, prefill_ctle_gain_db, -12, 12);
MEM_HINT_RW_U32(agentic_priority, agentic_priority, 0, 7);
MEM_HINT_RW_U32(idle_pll_reduction, idle_pll_reduction, 0, 1);
MEM_HINT_RW_U32(idle_vswing_mv, idle_vswing_mv, 200, 400);

MEM_HINT_RW_U32(prefill_write_bw_gbps, prefill_wr_thresh, 1, 1024);
MEM_HINT_RW_U32(decode_write_bw_ceiling, decode_wr_ceil, 0, 64);
MEM_HINT_RW_U32(decode_llc_miss_floor, decode_llc_floor, 1, 1000000);
MEM_HINT_RW_U32(agentic_bw_variance_pct, agentic_var_thresh, 1, 100);
MEM_HINT_RW_U32(idle_cmd_rate_floor, idle_cmd_thresh, 0, 1000000);

static ssize_t current_phase_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	(void)dev;
	(void)attr;
	return sysfs_emit(buf, "%d\n", atomic_read(&current_phase_id));
}

static ssize_t ecc_correctable_rate_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	(void)dev;
	(void)attr;
	return sysfs_emit(buf, "%llu\n", ecc_state.correctable_rate);
}

static ssize_t ecc_uncorrectable_count_show(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	(void)dev;
	(void)attr;
	return sysfs_emit(buf, "%llu\n", ecc_state.uncorrectable_count);
}

static ssize_t read_retry_count_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	(void)dev;
	(void)attr;
	return sysfs_emit(buf, "%llu\n", ecc_state.read_retry_count);
}

static ssize_t last_transition_ms_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	s64 delta_ms;

	(void)dev;
	(void)attr;
	delta_ms = ktime_to_ms(ktime_sub(ktime_get(), last_transition_time));
	if (delta_ms < 0)
		delta_ms = 0;

	return sysfs_emit(buf, "%lld\n", delta_ms);
}

static ssize_t p99_latency_ns_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	(void)dev;
	(void)attr;
	return sysfs_emit(buf, "%llu\n", ecc_state.p99_latency_ns);
}

static ssize_t active_channel_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	(void)dev;
	(void)attr;
	switch (active_channel) {
	case CH_MSR:
		return sysfs_emit(buf, "MSR\n");
	case CH_MMIO:
		return sysfs_emit(buf, "MMIO\n");
	case CH_CXL_DVSEC:
		return sysfs_emit(buf, "CXL_DVSEC\n");
	default:
		return sysfs_emit(buf, "UNKNOWN\n");
	}
}

static DEVICE_ATTR_RO(current_phase);
static DEVICE_ATTR_RO(ecc_correctable_rate);
static DEVICE_ATTR_RO(ecc_uncorrectable_count);
static DEVICE_ATTR_RO(read_retry_count);
static DEVICE_ATTR_RO(last_transition_ms);
static DEVICE_ATTR_RO(p99_latency_ns);
static DEVICE_ATTR_RO(active_channel);

static struct attribute *policy_attrs[] = {
	&dev_attr_decode_trcd.attr,
	&dev_attr_decode_vswing_mv.attr,
	&dev_attr_decode_dfe_tap1.attr,
	&dev_attr_prefill_vswing_mv.attr,
	&dev_attr_prefill_ctle_gain_db.attr,
	&dev_attr_agentic_priority.attr,
	&dev_attr_idle_pll_reduction.attr,
	&dev_attr_idle_vswing_mv.attr,
	NULL,
};

static struct attribute *threshold_attrs[] = {
	&dev_attr_prefill_write_bw_gbps.attr,
	&dev_attr_decode_write_bw_ceiling.attr,
	&dev_attr_decode_llc_miss_floor.attr,
	&dev_attr_agentic_bw_variance_pct.attr,
	&dev_attr_idle_cmd_rate_floor.attr,
	NULL,
};

static struct attribute *status_attrs[] = {
	&dev_attr_current_phase.attr,
	&dev_attr_ecc_correctable_rate.attr,
	&dev_attr_ecc_uncorrectable_count.attr,
	&dev_attr_read_retry_count.attr,
	&dev_attr_last_transition_ms.attr,
	&dev_attr_p99_latency_ns.attr,
	&dev_attr_active_channel.attr,
	NULL,
};

static const struct attribute_group policy_group = {
	.name = "policy",
	.attrs = policy_attrs,
};

static const struct attribute_group threshold_group = {
	.name = "thresholds",
	.attrs = threshold_attrs,
};

static const struct attribute_group status_group = {
	.name = "status",
	.attrs = status_attrs,
};

int mem_hint_sysfs_init(void)
{
	int ret;

	if (!mem_hint_pdev)
		return 0;

	ret = sysfs_create_group(&mem_hint_pdev->dev.kobj, &policy_group);
	if (ret)
		return ret;
	ret = sysfs_create_group(&mem_hint_pdev->dev.kobj, &threshold_group);
	if (ret) {
		sysfs_remove_group(&mem_hint_pdev->dev.kobj, &policy_group);
		return ret;
	}
	ret = sysfs_create_group(&mem_hint_pdev->dev.kobj, &status_group);
	if (ret) {
		sysfs_remove_group(&mem_hint_pdev->dev.kobj, &threshold_group);
		sysfs_remove_group(&mem_hint_pdev->dev.kobj, &policy_group);
		return ret;
	}

	return 0;
}

void mem_hint_sysfs_notify_phase_change(void)
{
	if (!mem_hint_pdev)
		return;

	sysfs_notify(&mem_hint_pdev->dev.kobj, "status", "current_phase");
	sysfs_notify(&mem_hint_pdev->dev.kobj, "status", "last_transition_ms");
}

void mem_hint_sysfs_exit(void)
{
	if (!mem_hint_pdev)
		return;

	sysfs_remove_group(&mem_hint_pdev->dev.kobj, &status_group);
	sysfs_remove_group(&mem_hint_pdev->dev.kobj, &threshold_group);
	sysfs_remove_group(&mem_hint_pdev->dev.kobj, &policy_group);
}
