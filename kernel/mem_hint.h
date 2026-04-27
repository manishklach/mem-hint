/* SPDX-License-Identifier: GPL-2.0-only
 * mem_hint.h - Workload Hint Interface for AI Memory Systems
 * Patent Pending: Indian Patent Application No. 202641053160
 * Inventor: Manish KL, filed 26 April 2026
 * Reference implementation - MSR addresses are illustrative.
 */

#ifndef MEM_HINT_H
#define MEM_HINT_H

#ifdef __KERNEL__
#include <linux/atomic.h>
#include <linux/device.h>
#include <linux/ktime.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#else
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint64_t u64;
typedef int8_t s8;
#endif

#ifdef __KERNEL__
struct mem_workload_hint {
	u8 phase_id;
	u16 latency_target_ns;
	u16 bw_target_gbps;
	u8 security_level;
	u8 priority;
	u8 reserved[2];
} __packed;
#else
struct mem_workload_hint {
	u8 phase_id;
	u16 latency_target_ns;
	u16 bw_target_gbps;
	u8 security_level;
	u8 priority;
	u8 reserved[2];
} __attribute__((packed));
#endif

#define PHASE_PREFILL	0x01
#define PHASE_DECODE	0x02
#define PHASE_AGENTIC	0x03
#define PHASE_IDLE	0x04
#define PHASE_FORWARD	0x05
#define PHASE_BACKWARD	0x06

enum mem_hint_channel {
	CH_MSR = 0,
	CH_MMIO = 1,
	CH_CXL_DVSEC = 2,
};

#define MEM_HINT_MSR	0xC0010F00

struct phy_config {
	u8 tRCD;
	u8 tCL;
	u8 tRP;
	u8 tRAS;
	u16 vswing_mv;
	u8 dfe_tap[4];
	s8 ctle_gain_db;
	u8 refresh_mode;
};

struct jedec_limits {
	u8 min_tRCD;
	u8 max_tRCD;
	u8 min_tCL;
	u8 max_tCL;
	u8 min_tRP;
	u8 max_tRP;
	u16 min_vswing_mv;
	u16 max_vswing_mv;
};

struct ecc_telemetry {
	u64 correctable_rate;
	u64 uncorrectable_count;
	u64 read_retry_count;
	u64 p99_latency_ns;
};

#ifdef __KERNEL__
struct pmu_sample {
	u32 write_bw_gbps;
	u32 read_bw_gbps;
	u32 llc_miss_rate;
	u32 bw_variance_pct;
	u32 dram_cmd_rate;
};

extern atomic_t current_phase_id;
extern enum mem_hint_channel active_channel;
extern struct jedec_limits platform_limits;
extern struct ecc_telemetry ecc_state;
extern ktime_t last_transition_time;
extern u8 mem_hint_security_level;
extern struct platform_driver mem_hint_platform_driver;
extern struct platform_device *mem_hint_pdev;

extern u32 decode_trcd;
extern u32 decode_vswing_mv;
extern u32 decode_dfe_tap1;
extern u32 prefill_vswing_mv;
extern s32 prefill_ctle_gain_db;
extern u32 agentic_priority;
extern u32 idle_pll_reduction;
extern u32 idle_vswing_mv;
extern unsigned int prefill_wr_thresh;
extern unsigned int decode_wr_ceil;
extern unsigned int decode_llc_floor;
extern unsigned int agentic_var_thresh;
extern unsigned int idle_cmd_thresh;

int mem_hint_apply(const struct mem_workload_hint *h);
int mem_hint_pmu_init(void);
void mem_hint_pmu_exit(void);
int mem_hint_sysfs_init(void);
void mem_hint_sysfs_exit(void);
void mem_hint_sysfs_notify_phase_change(void);
int mem_hint_should_touch_hw(void);
struct phy_config safety_clamp(struct phy_config proposed,
			       struct jedec_limits limits,
			       struct ecc_telemetry ecc);
int safety_limiter_selftest(void);
#endif

#endif /* MEM_HINT_H */
