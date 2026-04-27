/* SPDX-License-Identifier: GPL-2.0-only
 * mem_hint.c — /dev/mem_hint workload-aware memory hint interface
 * Patent Pending: Indian Patent Application No. 202641053160
 * Inventor: Manish KL, filed 26 April 2026
 * Reference implementation — illustrative MSR addresses, not
 * production silicon register definitions.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/atomic.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <asm/msr.h>
#include <linux/io.h>

#include "mem_hint.h"

/* ── Global state definitions ──────────────────────────────────── */

atomic_t            current_phase_id = ATOMIC_INIT(PHASE_IDLE);
struct ecc_telemetry ecc_state       = {0};
struct jedec_limits  platform_limits = {
	.min_tRCD = 14, .max_tRCD = 32,
	.min_tCL  = 14, .max_tCL  = 36,
	.min_tRP  = 14, .max_tRP  = 32,
	.min_vswing_mv = 200, .max_vswing_mv = 400,
};
enum mem_hint_channel active_channel = CH_MSR;
static struct class          *mem_hint_class;
static struct device         *mem_hint_dev;
static struct platform_device *mem_hint_pdev;
static void __iomem *mc_mmio_base __maybe_unused = NULL;

module_param(active_channel, int, 0644);
MODULE_PARM_DESC(active_channel,
	"HW channel: 0=MSR(default) 1=MMIO 2=CXL_DVSEC");

/* ── Hint encoding ─────────────────────────────────────────────── */

static u64 encode_hint(const struct mem_workload_hint *h)
{
	u64 val = 0;

	/* bits [7:0]   = phase_id       */
	val |= (u64)h->phase_id;
	/* bits [23:8]  = latency_target */
	val |= ((u64)h->latency_target_ns & 0xFFFFULL) << 8;
	/* bits [39:24] = bw_target      */
	val |= ((u64)h->bw_target_gbps   & 0xFFFFULL) << 24;
	/* bits [47:40] = security_level */
	val |= ((u64)h->security_level   & 0xFFULL)   << 40;
	/* bits [55:48] = priority       */
	val |= ((u64)h->priority         & 0xFFULL)   << 48;
	/* bits [63:56] = 0 (reserved)   */

	return val;
}

/* ── Phase → PHY config lookup ─────────────────────────────────── */

static struct phy_config phase_to_phy_config(u8 phase_id, u8 priority)
{
	/* Nominal / baseline config (Prefill/Forward default) */
	struct phy_config cfg = {
		.tRCD = 22, .tCL = 22, .tRP = 22, .tRAS = 52,
		.vswing_mv = 300,
		.dfe_tap = {0x10, 0, 0, 0},
		.ctle_gain_db = 2,
		.refresh_mode = 0,
	};
	/* Nominal values used for priority scaling */
	u8 nom_tRCD = 22, nom_tCL = 22, nom_tRP = 22, nom_tRAS = 52;
	int delta;

	if (priority > 7)
		priority = 7;

	switch (phase_id) {
	case PHASE_PREFILL:
	case PHASE_FORWARD:
		/* tRCD=22 tCL=22 tRP=22 tRAS=52 vswing=300 */
		cfg.tRCD = 22; cfg.tCL = 22; cfg.tRP = 22; cfg.tRAS = 52;
		cfg.vswing_mv = 300;
		cfg.dfe_tap[0] = 0x10;
		cfg.ctle_gain_db = 2;
		cfg.refresh_mode = 0;
		break;

	case PHASE_DECODE:
		cfg.tRCD = 18; cfg.tCL = 18; cfg.tRP = 18; cfg.tRAS = 42;
		cfg.vswing_mv = 280;
		cfg.dfe_tap[0] = 0x14;
		cfg.ctle_gain_db = 3;
		cfg.refresh_mode = 0;
		break;

	case PHASE_BACKWARD:
		/* tRAS=38 implements tWR reduction for gradient writes — claim 32 */
		cfg.tRCD = 18; cfg.tCL = 18; cfg.tRP = 18; cfg.tRAS = 38;
		cfg.vswing_mv = 280;
		cfg.dfe_tap[0] = 0x14;
		cfg.ctle_gain_db = 3;
		cfg.refresh_mode = 0;
		break;

	case PHASE_AGENTIC:
		cfg.tRCD = 20; cfg.tCL = 20; cfg.tRP = 20; cfg.tRAS = 48;
		cfg.vswing_mv = 290;
		cfg.dfe_tap[0] = 0x12;
		cfg.ctle_gain_db = 2;
		cfg.refresh_mode = 0;
		break;

	case PHASE_IDLE:
		cfg.tRCD = 24; cfg.tCL = 24; cfg.tRP = 24; cfg.tRAS = 56;
		cfg.vswing_mv = 240;
		cfg.dfe_tap[0] = 0x08;
		cfg.ctle_gain_db = 0;
		cfg.refresh_mode = 1;
		break;

	default:
		break;
	}

	/*
	 * Priority scaling: for each timing parameter,
	 *   delta = (param - nominal) * priority / 7
	 * So priority=0 yields nominal; priority=7 yields full deviation.
	 */
	delta = ((int)cfg.tRCD - (int)nom_tRCD) * (int)priority / 7;
	cfg.tRCD = (u8)((int)nom_tRCD + delta);

	delta = ((int)cfg.tCL - (int)nom_tCL) * (int)priority / 7;
	cfg.tCL = (u8)((int)nom_tCL + delta);

	delta = ((int)cfg.tRP - (int)nom_tRP) * (int)priority / 7;
	cfg.tRP = (u8)((int)nom_tRP + delta);

	delta = ((int)cfg.tRAS - (int)nom_tRAS) * (int)priority / 7;
	cfg.tRAS = (u8)((int)nom_tRAS + delta);

	return cfg;
}

/* ── Apply a workload hint ─────────────────────────────────────── */

void mem_hint_apply(const struct mem_workload_hint *h)
{
	struct phy_config config;
	struct phy_config clamped;
	u64 encoded_val;

	config = phase_to_phy_config(h->phase_id, h->priority);
	clamped = safety_clamp(config, platform_limits, ecc_state,
			       h->security_level);
	encoded_val = encode_hint(h);

	switch (active_channel) {
	case CH_MSR:
		wrmsrl(MEM_HINT_MSR, encoded_val);
		break;
	case CH_MMIO:
		if (mc_mmio_base)
			iowrite32((u32)encoded_val, mc_mmio_base);
		else
			pr_debug("mem_hint: MMIO base not mapped\n");
		break;
	case CH_CXL_DVSEC:
		pr_debug("mem_hint: CXL DVSEC stub: 0x%llx\n",
			 encoded_val);
		break;
	default:
		break;
	}

	atomic_set(&current_phase_id, h->phase_id);

	pr_debug("mem_hint: applied phase=%u tRCD=%u tCL=%u tRP=%u "
		 "tRAS=%u vswing=%u\n",
		 h->phase_id, clamped.tRCD, clamped.tCL,
		 clamped.tRP, clamped.tRAS, clamped.vswing_mv);
}

/* ── File operations ───────────────────────────────────────────── */

static int mem_hint_open(struct inode *inode, struct file *file)
{
	(void)inode;
	(void)file;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	return 0;
}

static ssize_t mem_hint_write(struct file *file, const char __user *buf,
			      size_t len, loff_t *ppos)
{
	struct mem_workload_hint hint;

	(void)file;
	(void)ppos;

	if (len < sizeof(struct mem_workload_hint))
		return -EINVAL;

	if (copy_from_user(&hint, buf, sizeof(hint)))
		return -EFAULT;

	if (hint.phase_id < 0x01 || hint.phase_id > 0x06)
		return -EINVAL;

	hint.priority = min_t(u8, hint.priority, 7);

	if (hint.security_level > 0)
		pr_debug("mem_hint: security_level=%u requested\n",
			 hint.security_level);

	mem_hint_apply(&hint);
	return sizeof(hint);
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open  = mem_hint_open,
	.write = mem_hint_write,
};

/* ── Module init / exit ────────────────────────────────────────── */

static int __init mem_hint_init(void)
{
	int ret;

	ret = register_chrdev(240, "mem_hint", &fops);
	if (ret < 0)
		return ret;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	mem_hint_class = class_create("mem_hint");
#else
	mem_hint_class = class_create(THIS_MODULE, "mem_hint");
#endif
	if (IS_ERR(mem_hint_class)) {
		ret = PTR_ERR(mem_hint_class);
		unregister_chrdev(240, "mem_hint");
		return ret;
	}

	mem_hint_dev = device_create(mem_hint_class, NULL,
				     MKDEV(240, 0), NULL, "mem_hint");
	if (IS_ERR(mem_hint_dev)) {
		ret = PTR_ERR(mem_hint_dev);
		class_destroy(mem_hint_class);
		unregister_chrdev(240, "mem_hint");
		return ret;
	}

	mem_hint_pdev = platform_device_register_simple(
		"mem_hint", -1, NULL, 0);
	if (IS_ERR(mem_hint_pdev)) {
		pr_warn("mem_hint: platform_device_register_simple failed\n");
		mem_hint_pdev = NULL;
	}

	mem_hint_pmu_init();

	if (mem_hint_pdev)
		mem_hint_sysfs_init(&mem_hint_pdev->dev);

	safety_limiter_selftest();

	pr_info("mem_hint: loaded, channel=%d\n", active_channel);

	return 0;
}

static void __exit mem_hint_exit(void)
{
	if (mem_hint_pdev)
		mem_hint_sysfs_exit(&mem_hint_pdev->dev);
	mem_hint_pmu_exit();
	if (mem_hint_pdev)
		platform_device_unregister(mem_hint_pdev);
	device_destroy(mem_hint_class, MKDEV(240, 0));
	class_destroy(mem_hint_class);
	unregister_chrdev(240, "mem_hint");
	pr_info("mem_hint: unloaded\n");
}

module_init(mem_hint_init);
module_exit(mem_hint_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Manish KL");
MODULE_DESCRIPTION("AI workload hint interface — patent IN 202641053160");
MODULE_VERSION("0.1.0");
