/* SPDX-License-Identifier: GPL-2.0-only
 * mem_hint.h - Workload Hint Interface for AI Memory Systems
 * Patent Pending: Indian Patent Application No. 202641053160
 * Inventor: Manish KL, filed 26 April 2026
 * Reference implementation - MSR addresses are illustrative.
 */

#ifndef MEM_HINT_H
#define MEM_HINT_H

#include <linux/types.h>

struct mem_workload_hint {
	u8 phase_id;
	u16 latency_target_ns;
	u16 bw_target_gbps;
	u8 security_level;
	u8 priority;
	u8 reserved[2];
} __packed;

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

#endif /* MEM_HINT_H */
