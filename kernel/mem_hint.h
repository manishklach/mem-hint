/* SPDX-License-Identifier: GPL-2.0-only
 * mem_hint.h — /dev/mem_hint workload-aware memory hint interface
 * Patent Pending: Indian Patent Application No. 202641053160
 * Inventor: Manish KL, filed 26 April 2026
 * Reference implementation — illustrative MSR addresses, not
 * production silicon register definitions.
 */

#ifndef MEM_HINT_H
#define MEM_HINT_H

#ifdef __KERNEL__
# include <linux/types.h>
#else
# include <stdint.h>
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint64_t u64;
typedef int8_t   s8;
#endif

/* Phase ID constants */
#define PHASE_PREFILL    0x01
#define PHASE_DECODE     0x02
#define PHASE_AGENTIC    0x03
#define PHASE_IDLE       0x04
#define PHASE_FORWARD    0x05
#define PHASE_BACKWARD   0x06

/* Illustrative MSR address for workload hint register */
#define MEM_HINT_MSR     0xC0010F00

/* Hardware channel selection */
enum mem_hint_channel {
	CH_MSR       = 0,
	CH_MMIO      = 1,
	CH_CXL_DVSEC = 2,
};

/* Packed hint struct — 8 bytes exactly */
#ifdef __KERNEL__
struct mem_workload_hint {
	u8  phase_id;
	u16 latency_target_ns;
	u16 bw_target_gbps;
	u8  security_level;
	u8  priority;
	u8  reserved[2];
} __packed;
#else
struct mem_workload_hint {
	uint8_t  phase_id;
	uint16_t latency_target_ns;
	uint16_t bw_target_gbps;
	uint8_t  security_level;
	uint8_t  priority;
	uint8_t  reserved[2];
} __attribute__((packed));
#endif

/* PHY timing and signaling configuration */
struct phy_config {
	u8  tRCD, tCL, tRP, tRAS;
	u16 vswing_mv;
	u8  dfe_tap[4];
	s8  ctle_gain_db;
	u8  refresh_mode;
};

/* JEDEC SPD-derived hard limits */
struct jedec_limits {
	u8  min_tRCD, max_tRCD;
	u8  min_tCL,  max_tCL;
	u8  min_tRP,  max_tRP;
	u16 min_vswing_mv, max_vswing_mv;
};

/* ECC and reliability telemetry */
struct ecc_telemetry {
	u64 correctable_rate;
	u64 uncorrectable_count;
	u64 read_retry_count;
	u64 p99_latency_ns;
};

/* PMU sample from uncore and core counters */
struct pmu_sample {
	u64 write_bw_gbps;
	u64 read_bw_gbps;
	u64 llc_miss_rate;
	u64 bw_variance_pct;
	u64 dram_cmd_rate;
};

/* Function declarations */
void mem_hint_apply(const struct mem_workload_hint *h);
int  mem_hint_pmu_init(void);
void mem_hint_pmu_exit(void);
int  mem_hint_sysfs_init(struct device *dev);
void mem_hint_sysfs_exit(struct device *dev);
struct phy_config safety_clamp(struct phy_config proposed,
			       struct jedec_limits limits,
			       struct ecc_telemetry ecc,
			       u8 security_level);
void safety_limiter_selftest(void);

/* Shared state — defined in mem_hint.c */
extern atomic_t           current_phase_id;
extern struct ecc_telemetry ecc_state;
extern struct jedec_limits  platform_limits;
extern enum mem_hint_channel active_channel;

#endif /* MEM_HINT_H */
