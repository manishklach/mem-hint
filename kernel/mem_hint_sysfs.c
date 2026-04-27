/* SPDX-License-Identifier: GPL-2.0-only
 * mem_hint_sysfs.c — /dev/mem_hint workload-aware memory hint interface
 * Patent Pending: Indian Patent Application No. 202641053160
 * Inventor: Manish KL, filed 26 April 2026
 * Reference implementation — illustrative MSR addresses, not
 * production silicon register definitions.
 */

#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/module.h>

#include "mem_hint.h"

/* ── Policy variables (mirrors platform_limits defaults) ───────── */

static u8  policy_decode_trcd          = 18;
static u16 policy_decode_vswing_mv     = 280;
static u8  policy_decode_dfe_tap1      = 0x14;
static u16 policy_prefill_vswing_mv    = 300;
static s8  policy_prefill_ctle_gain_db = 2;
static u8  policy_agentic_priority     = 5;
static u8  policy_idle_pll_reduction   = 1;
static u16 policy_idle_vswing_mv       = 240;

/* ── Threshold variables ───────────────────────────────────────── */

static u32 thresh_prefill_wr_bw        = 10;
static u32 thresh_decode_wr_ceil       = 1;
static u32 thresh_decode_llc_floor     = 5000;
static u32 thresh_agentic_var_pct      = 50;
static u32 thresh_idle_cmd_floor       = 1000;

/* ── Policy show/store functions ───────────────────────────────── */

/* decode_trcd */
static ssize_t decode_trcd_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "%u\n", policy_decode_trcd);
}
static ssize_t decode_trcd_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	unsigned long val;
	(void)dev; (void)attr;
	if (kstrtoul(buf, 0, &val) || val < 14 || val > 32)
		return -EINVAL;
	policy_decode_trcd = (u8)val;
	return count;
}
static DEVICE_ATTR_RW(decode_trcd);

/* decode_vswing_mv */
static ssize_t decode_vswing_mv_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "%u\n", policy_decode_vswing_mv);
}
static ssize_t decode_vswing_mv_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	unsigned long val;
	(void)dev; (void)attr;
	if (kstrtoul(buf, 0, &val) || val < 200 || val > 400)
		return -EINVAL;
	policy_decode_vswing_mv = (u16)val;
	return count;
}
static DEVICE_ATTR_RW(decode_vswing_mv);

/* decode_dfe_tap1 */
static ssize_t decode_dfe_tap1_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "%u\n", policy_decode_dfe_tap1);
}
static ssize_t decode_dfe_tap1_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	unsigned long val;
	(void)dev; (void)attr;
	if (kstrtoul(buf, 0, &val) || val > 255)
		return -EINVAL;
	policy_decode_dfe_tap1 = (u8)val;
	return count;
}
static DEVICE_ATTR_RW(decode_dfe_tap1);

/* prefill_vswing_mv */
static ssize_t prefill_vswing_mv_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "%u\n", policy_prefill_vswing_mv);
}
static ssize_t prefill_vswing_mv_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	unsigned long val;
	(void)dev; (void)attr;
	if (kstrtoul(buf, 0, &val) || val < 200 || val > 400)
		return -EINVAL;
	policy_prefill_vswing_mv = (u16)val;
	return count;
}
static DEVICE_ATTR_RW(prefill_vswing_mv);

/* prefill_ctle_gain_db */
static ssize_t prefill_ctle_gain_db_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "%d\n", policy_prefill_ctle_gain_db);
}
static ssize_t prefill_ctle_gain_db_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	long val;
	(void)dev; (void)attr;
	if (kstrtol(buf, 0, &val) || val < -12 || val > 12)
		return -EINVAL;
	policy_prefill_ctle_gain_db = (s8)val;
	return count;
}
static DEVICE_ATTR_RW(prefill_ctle_gain_db);

/* agentic_priority */
static ssize_t agentic_priority_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "%u\n", policy_agentic_priority);
}
static ssize_t agentic_priority_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	unsigned long val;
	(void)dev; (void)attr;
	if (kstrtoul(buf, 0, &val) || val > 7)
		return -EINVAL;
	policy_agentic_priority = (u8)val;
	return count;
}
static DEVICE_ATTR_RW(agentic_priority);

/* idle_pll_reduction */
static ssize_t idle_pll_reduction_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "%u\n", policy_idle_pll_reduction);
}
static ssize_t idle_pll_reduction_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned long val;
	(void)dev; (void)attr;
	if (kstrtoul(buf, 0, &val) || val > 1)
		return -EINVAL;
	policy_idle_pll_reduction = (u8)val;
	return count;
}
static DEVICE_ATTR_RW(idle_pll_reduction);

/* idle_vswing_mv */
static ssize_t idle_vswing_mv_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "%u\n", policy_idle_vswing_mv);
}
static ssize_t idle_vswing_mv_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	unsigned long val;
	(void)dev; (void)attr;
	if (kstrtoul(buf, 0, &val) || val < 200 || val > 400)
		return -EINVAL;
	policy_idle_vswing_mv = (u16)val;
	return count;
}
static DEVICE_ATTR_RW(idle_vswing_mv);

/* ── Threshold show/store functions ────────────────────────────── */

/* prefill_wr_bw */
static ssize_t prefill_wr_bw_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "%u\n", thresh_prefill_wr_bw);
}
static ssize_t prefill_wr_bw_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	unsigned long val;
	(void)dev; (void)attr;
	if (kstrtoul(buf, 0, &val) || val < 1 || val > 1024)
		return -EINVAL;
	thresh_prefill_wr_bw = (u32)val;
	return count;
}
static DEVICE_ATTR_RW(prefill_wr_bw);

/* decode_wr_ceil */
static ssize_t decode_wr_ceil_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "%u\n", thresh_decode_wr_ceil);
}
static ssize_t decode_wr_ceil_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	unsigned long val;
	(void)dev; (void)attr;
	if (kstrtoul(buf, 0, &val) || val > 64)
		return -EINVAL;
	thresh_decode_wr_ceil = (u32)val;
	return count;
}
static DEVICE_ATTR_RW(decode_wr_ceil);

/* decode_llc_floor */
static ssize_t decode_llc_floor_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "%u\n", thresh_decode_llc_floor);
}
static ssize_t decode_llc_floor_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	unsigned long val;
	(void)dev; (void)attr;
	if (kstrtoul(buf, 0, &val) || val < 1 || val > 1000000)
		return -EINVAL;
	thresh_decode_llc_floor = (u32)val;
	return count;
}
static DEVICE_ATTR_RW(decode_llc_floor);

/* agentic_var_pct */
static ssize_t agentic_var_pct_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "%u\n", thresh_agentic_var_pct);
}
static ssize_t agentic_var_pct_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	unsigned long val;
	(void)dev; (void)attr;
	if (kstrtoul(buf, 0, &val) || val < 1 || val > 100)
		return -EINVAL;
	thresh_agentic_var_pct = (u32)val;
	return count;
}
static DEVICE_ATTR_RW(agentic_var_pct);

/* idle_cmd_floor */
static ssize_t idle_cmd_floor_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "%u\n", thresh_idle_cmd_floor);
}
static ssize_t idle_cmd_floor_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	unsigned long val;
	(void)dev; (void)attr;
	if (kstrtoul(buf, 0, &val) || val > 1000000)
		return -EINVAL;
	thresh_idle_cmd_floor = (u32)val;
	return count;
}
static DEVICE_ATTR_RW(idle_cmd_floor);

/* ── Status (read-only) attributes ─────────────────────────────── */

static ssize_t current_phase_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "0x%02x\n", atomic_read(&current_phase_id));
}
static DEVICE_ATTR_RO(current_phase);

static ssize_t ecc_correctable_rate_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "%llu\n", ecc_state.correctable_rate);
}
static DEVICE_ATTR_RO(ecc_correctable_rate);

static ssize_t ecc_uncorrectable_count_show(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "%llu\n", ecc_state.uncorrectable_count);
}
static DEVICE_ATTR_RO(ecc_uncorrectable_count);

static ssize_t read_retry_count_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "%llu\n", ecc_state.read_retry_count);
}
static DEVICE_ATTR_RO(read_retry_count);

static ssize_t p99_latency_ns_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	(void)dev; (void)attr;
	return sysfs_emit(buf, "%llu\n", ecc_state.p99_latency_ns);
}
static DEVICE_ATTR_RO(p99_latency_ns);

static ssize_t active_channel_sysfs_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	(void)dev; (void)attr;
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
static DEVICE_ATTR(active_channel, 0444, active_channel_sysfs_show, NULL);

/* ── Attribute groups ──────────────────────────────────────────── */

static struct attribute *mem_hint_policy_attrs[] = {
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

static const struct attribute_group mem_hint_policy_group = {
	.name  = "policy",
	.attrs = mem_hint_policy_attrs,
};

static struct attribute *mem_hint_threshold_attrs[] = {
	&dev_attr_prefill_wr_bw.attr,
	&dev_attr_decode_wr_ceil.attr,
	&dev_attr_decode_llc_floor.attr,
	&dev_attr_agentic_var_pct.attr,
	&dev_attr_idle_cmd_floor.attr,
	NULL,
};

static const struct attribute_group mem_hint_threshold_group = {
	.name  = "thresholds",
	.attrs = mem_hint_threshold_attrs,
};

static struct attribute *mem_hint_status_attrs[] = {
	&dev_attr_current_phase.attr,
	&dev_attr_ecc_correctable_rate.attr,
	&dev_attr_ecc_uncorrectable_count.attr,
	&dev_attr_read_retry_count.attr,
	&dev_attr_p99_latency_ns.attr,
	&dev_attr_active_channel.attr,
	NULL,
};

static const struct attribute_group mem_hint_status_group = {
	.name  = "status",
	.attrs = mem_hint_status_attrs,
};

/* ── Init / exit ───────────────────────────────────────────────── */

int mem_hint_sysfs_init(struct device *dev)
{
	int ret;

	ret = sysfs_create_group(&dev->kobj, &mem_hint_policy_group);
	if (ret)
		return ret;

	ret = sysfs_create_group(&dev->kobj, &mem_hint_threshold_group);
	if (ret) {
		sysfs_remove_group(&dev->kobj, &mem_hint_policy_group);
		return ret;
	}

	ret = sysfs_create_group(&dev->kobj, &mem_hint_status_group);
	if (ret) {
		sysfs_remove_group(&dev->kobj, &mem_hint_threshold_group);
		sysfs_remove_group(&dev->kobj, &mem_hint_policy_group);
		return ret;
	}

	return 0;
}

void mem_hint_sysfs_exit(struct device *dev)
{
	sysfs_remove_group(&dev->kobj, &mem_hint_status_group);
	sysfs_remove_group(&dev->kobj, &mem_hint_threshold_group);
	sysfs_remove_group(&dev->kobj, &mem_hint_policy_group);
}
