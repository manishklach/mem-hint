/* SPDX-License-Identifier: GPL-2.0-only
 * mem_hint.c - Workload Hint Interface for AI Memory Systems
 * Patent Pending: Indian Patent Application No. 202641053160
 * Inventor: Manish KL, filed 26 April 2026
 * Reference implementation - MSR addresses are illustrative.
 */

#include <linux/atomic.h>
#include <linux/capability.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <asm/msr.h>

#include "mem_hint.h"

#define MEM_HINT_MAJOR		240
#define MEM_HINT_DEVICE_NAME	"mem_hint"
#define MEM_HINT_MMIO_OFFSET	0x100

static atomic_t current_phase_id = ATOMIC_INIT(PHASE_IDLE);
static enum mem_hint_channel active_channel = CH_MSR;
static int active_channel_param = CH_MSR;
static struct jedec_limits platform_limits = {
	.min_tRCD = 14,
	.max_tRCD = 32,
	.min_tCL = 14,
	.max_tCL = 36,
	.min_tRP = 14,
	.max_tRP = 32,
	.min_vswing_mv = 200,
	.max_vswing_mv = 400,
};
static struct ecc_telemetry ecc_state;
static struct class *mem_hint_class;
static struct device *mem_hint_dev;
static struct platform_device *mem_hint_platform_dev;
static void __iomem *mc_mmio_base;
static ktime_t last_transition_time;
static u8 mem_hint_security_level;
static bool illustrative_hw_writes;

extern const struct attribute_group *mem_hint_driver_groups[];
extern u32 decode_trcd;
extern u32 decode_vswing_mv;
extern u32 decode_dfe_tap1;
extern u32 prefill_vswing_mv;
extern s32 prefill_ctle_gain_db;
extern u32 agentic_priority;
extern u32 idle_pll_reduction;
extern u32 idle_vswing_mv;

module_param_named(active_channel, active_channel_param, int, 0644);
MODULE_PARM_DESC(active_channel,
		 "Hardware channel: 0=MSR(default) 1=MMIO 2=CXL_DVSEC");
module_param_named(illustrative_hw_writes, illustrative_hw_writes, bool, 0644);
MODULE_PARM_DESC(illustrative_hw_writes,
		 "Allow illustrative privileged writes for reference testing");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Manish KL");
MODULE_DESCRIPTION("AI workload hint interface - patent IN 202641053160");
MODULE_VERSION("0.1.0");

static struct platform_driver mem_hint_platform_driver;

static u64 encode_hint(const struct mem_workload_hint *h)
{
	u64 val;

	val = 0;
	val |= (u64)h->phase_id;
	val |= ((u64)h->latency_target_ns & 0xffffULL) << 8;
	val |= ((u64)h->bw_target_gbps & 0xffffULL) << 24;
	val |= ((u64)h->security_level & 0xffULL) << 40;
	val |= ((u64)h->priority & 0xffULL) << 48;
	val |= ((u64)h->reserved[0] & 0xffULL) << 56;

	return val;
}

static u8 scale_timing(u8 nominal, u8 proposed, u8 priority)
{
	int delta;
	int scaled;

	if (priority > 7)
		priority = 7;

	delta = (int)nominal - (int)proposed;
	scaled = (int)nominal - ((delta * (int)priority) / 7);

	if (scaled < 0)
		scaled = 0;
	if (scaled > 0xff)
		scaled = 0xff;

	return (u8)scaled;
}

int mem_hint_should_touch_hw(void)
{
	return illustrative_hw_writes ? 1 : 0;
}

static struct phy_config phase_to_phy_config(const struct mem_workload_hint *h)
{
	struct phy_config cfg = {
		.tRCD = 22,
		.tCL = 22,
		.tRP = 22,
		.tRAS = 52,
		.vswing_mv = 300,
		.dfe_tap = { 0x10, 0x00, 0x00, 0x00 },
		.ctle_gain_db = 2,
		.refresh_mode = 0,
	};
	u8 priority;
	u8 nominal_trcd;
	u8 nominal_tcl;
	u8 nominal_trp;
	u8 nominal_tras;

	nominal_trcd = 22;
	nominal_tcl = 22;
	nominal_trp = 22;
	nominal_tras = 52;
	priority = h->priority > 7 ? 7 : h->priority;

	switch (h->phase_id) {
	case PHASE_PREFILL:
	case PHASE_FORWARD:
		cfg.vswing_mv = prefill_vswing_mv;
		cfg.dfe_tap[0] = 0x10;
		cfg.ctle_gain_db = prefill_ctle_gain_db;
		break;
	case PHASE_DECODE:
		cfg.tRCD = decode_trcd;
		cfg.tCL = 18;
		cfg.tRP = 18;
		cfg.tRAS = 42;
		cfg.vswing_mv = decode_vswing_mv;
		cfg.dfe_tap[0] = decode_dfe_tap1;
		cfg.ctle_gain_db = 3;
		break;
	case PHASE_AGENTIC:
		cfg.tRCD = 20;
		cfg.tCL = 20;
		cfg.tRP = 20;
		cfg.tRAS = 48;
		cfg.vswing_mv = 290;
		cfg.dfe_tap[0] = 0x12;
		cfg.ctle_gain_db = 2;
		if (priority < agentic_priority)
			priority = agentic_priority;
		break;
	case PHASE_IDLE:
		cfg.tRCD = 24;
		cfg.tCL = 24;
		cfg.tRP = 24;
		cfg.tRAS = 56;
		cfg.vswing_mv = idle_vswing_mv;
		cfg.dfe_tap[0] = 0x08;
		cfg.ctle_gain_db = 0;
		cfg.refresh_mode = idle_pll_reduction;
		break;
	case PHASE_BACKWARD:
		cfg.tRCD = decode_trcd;
		cfg.tCL = 18;
		cfg.tRP = 18;
		cfg.tRAS = 38;
		cfg.vswing_mv = decode_vswing_mv;
		cfg.dfe_tap[0] = decode_dfe_tap1;
		cfg.ctle_gain_db = 3;
		break;
	default:
		break;
	}

	cfg.tRCD = scale_timing(nominal_trcd, cfg.tRCD, priority);
	cfg.tCL = scale_timing(nominal_tcl, cfg.tCL, priority);
	cfg.tRP = scale_timing(nominal_trp, cfg.tRP, priority);
	cfg.tRAS = scale_timing(nominal_tras, cfg.tRAS, priority);

	return cfg;
}

int mem_hint_apply(const struct mem_workload_hint *h)
{
	struct phy_config proposed;
	struct phy_config clamped;
	u64 val;
	int prev_phase;

	if (!h)
		return -EINVAL;

	proposed = phase_to_phy_config(h);
	mem_hint_security_level = h->security_level;
	clamped = safety_clamp(proposed, platform_limits, ecc_state);
	val = encode_hint(h);

	switch (active_channel) {
	case CH_MSR:
		/*
		 * This repository models the interface contract only. The MSR
		 * address is illustrative and disabled by default so loading
		 * the module on a development system does not touch undefined
		 * hardware state.
		 */
		if (mem_hint_should_touch_hw())
			wrmsrl(MEM_HINT_MSR, val);
		else
			pr_debug("mem_hint: illustrative MSR write suppressed (0x%llx)\n",
				 val);
		break;
	case CH_MMIO:
		/*
		 * MMIO handling is also illustrative. A production driver would
		 * discover and map a documented controller window first.
		 */
		if (mem_hint_should_touch_hw() && mc_mmio_base)
			iowrite64(val, mc_mmio_base + MEM_HINT_MMIO_OFFSET);
		else
			pr_debug("mem_hint: illustrative MMIO write suppressed or unmapped\n");
		break;
	case CH_CXL_DVSEC:
		pr_debug("mem_hint: CXL DVSEC path: illustrative stub write 0x%llx\n",
			 val);
		break;
	default:
		return -EINVAL;
	}

	prev_phase = atomic_xchg(&current_phase_id, h->phase_id);
	if (prev_phase != h->phase_id)
		last_transition_time = ktime_get();

	pr_debug("mem_hint: applied phase=%u trcd=%u tcl=%u trp=%u tras=%u vswing=%u\n",
		 h->phase_id, clamped.tRCD, clamped.tCL, clamped.tRP,
		 clamped.tRAS, clamped.vswing_mv);
	mem_hint_sysfs_notify_phase_change();

	return 0;
}

static int mem_hint_open(struct inode *inode, struct file *file)
{
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	return 0;
}

static ssize_t mem_hint_write(struct file *file, const char __user *buf,
			      size_t len, loff_t *ppos)
{
	struct mem_workload_hint hint;
	int ret;

	if (len < sizeof(hint))
		return -EINVAL;

	if (copy_from_user(&hint, buf, sizeof(hint)))
		return -EFAULT;

	if (hint.phase_id < PHASE_PREFILL || hint.phase_id > PHASE_BACKWARD)
		return -EINVAL;

	if (hint.priority > 7)
		hint.priority = 7;

	if (hint.security_level > 0)
		pr_warn_ratelimited("mem_hint: security hint requested without TEE attestation stub\n");

	ret = mem_hint_apply(&hint);
	if (ret)
		return ret;

	return sizeof(hint);
}

static const struct file_operations mem_hint_fops = {
	.owner = THIS_MODULE,
	.open = mem_hint_open,
	.write = mem_hint_write,
};

static int mem_hint_platform_probe(struct platform_device *pdev)
{
	dev_set_drvdata(&pdev->dev, NULL);
	return 0;
}

static int mem_hint_platform_remove(struct platform_device *pdev)
{
	dev_set_drvdata(&pdev->dev, NULL);
	return 0;
}

static struct platform_driver mem_hint_platform_driver = {
	.probe = mem_hint_platform_probe,
	.remove = mem_hint_platform_remove,
	.driver = {
		.name = "mem_hint",
		.groups = mem_hint_driver_groups,
	},
};

static int __init mem_hint_init(void)
{
	int ret;

	last_transition_time = ktime_get();
	if (active_channel_param < CH_MSR || active_channel_param > CH_CXL_DVSEC)
		active_channel_param = CH_MSR;
	active_channel = active_channel_param;
	ret = register_chrdev(MEM_HINT_MAJOR, MEM_HINT_DEVICE_NAME,
			      &mem_hint_fops);
	if (ret < 0)
		return ret;

	mem_hint_class = class_create("mem_hint");
	if (IS_ERR(mem_hint_class)) {
		ret = PTR_ERR(mem_hint_class);
		goto err_chrdev;
	}

	mem_hint_dev = device_create(mem_hint_class, NULL,
				     MKDEV(MEM_HINT_MAJOR, 0), NULL,
				     MEM_HINT_DEVICE_NAME);
	if (IS_ERR(mem_hint_dev)) {
		ret = PTR_ERR(mem_hint_dev);
		goto err_class;
	}

	ret = platform_driver_register(&mem_hint_platform_driver);
	if (ret)
		goto err_device;

	mem_hint_platform_dev = platform_device_register_simple("mem_hint", -1,
								NULL, 0);
	if (IS_ERR(mem_hint_platform_dev)) {
		ret = PTR_ERR(mem_hint_platform_dev);
		goto err_platform_driver;
	}

	ret = safety_limiter_selftest();
	if (ret)
		goto err_platform_device;

	ret = mem_hint_sysfs_init();
	if (ret)
		goto err_platform_device;

	ret = mem_hint_pmu_init();
	if (ret)
		goto err_sysfs;

	pr_info("mem_hint: loaded, channel=%d, patent IN 202641053160 reference implementation\n",
		active_channel);
	if (!illustrative_hw_writes)
		pr_info("mem_hint: illustrative hardware writes are disabled by default\n");
	return 0;

err_sysfs:
	mem_hint_sysfs_exit();
err_platform_device:
	platform_device_unregister(mem_hint_platform_dev);
err_platform_driver:
	platform_driver_unregister(&mem_hint_platform_driver);
err_device:
	device_destroy(mem_hint_class, MKDEV(MEM_HINT_MAJOR, 0));
err_class:
	class_destroy(mem_hint_class);
err_chrdev:
	unregister_chrdev(MEM_HINT_MAJOR, MEM_HINT_DEVICE_NAME);
	return ret;
}

static void __exit mem_hint_exit(void)
{
	mem_hint_pmu_exit();
	mem_hint_sysfs_exit();
	platform_device_unregister(mem_hint_platform_dev);
	platform_driver_unregister(&mem_hint_platform_driver);
	device_destroy(mem_hint_class, MKDEV(MEM_HINT_MAJOR, 0));
	class_destroy(mem_hint_class);
	unregister_chrdev(MEM_HINT_MAJOR, MEM_HINT_DEVICE_NAME);
	pr_info("mem_hint: unloaded\n");
}

module_init(mem_hint_init);
module_exit(mem_hint_exit);

#include "mem_hint_pmu.c"
#include "mem_hint_sysfs.c"
#include "mem_hint_safety.c"
