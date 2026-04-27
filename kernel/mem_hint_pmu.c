/* SPDX-License-Identifier: GPL-2.0-only
 * mem_hint_pmu.c — /dev/mem_hint workload-aware memory hint interface
 * Patent Pending: Indian Patent Application No. 202641053160
 * Inventor: Manish KL, filed 26 April 2026
 * Reference implementation — illustrative MSR addresses, not
 * production silicon register definitions.
 */

#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/version.h>

#include "mem_hint.h"

/* ── Module parameters (PMU classifier thresholds) ─────────────── */

static unsigned int prefill_wr_thresh    = 10;
static unsigned int decode_wr_ceil       = 1;
static unsigned int decode_llc_floor     = 5000;
static unsigned int agentic_var_thresh   = 50;
static unsigned int idle_cmd_thresh      = 1000;

module_param(prefill_wr_thresh,  uint, 0644);
module_param(decode_wr_ceil,     uint, 0644);
module_param(decode_llc_floor,   uint, 0644);
module_param(agentic_var_thresh, uint, 0644);
module_param(idle_cmd_thresh,    uint, 0644);

/* ── State ─────────────────────────────────────────────────────── */

static struct hrtimer pmu_timer;
static u8 pmu_last_phase = PHASE_IDLE;

/* ── Simulated PMU sample collection ───────────────────────────── */

/*
 * In a production implementation, this function would use
 * perf_event_create_kernel_counter() to subscribe to the following
 * hardware performance events:
 *
 *   UNC_M_CAS_COUNT.WR        → write_bw_gbps
 *     IMC uncore write CAS command count, scaled to GB/s by
 *     multiplying by cache-line size and dividing by sample interval.
 *
 *   UNC_M_CAS_COUNT.RD        → read_bw_gbps
 *     IMC uncore read CAS command count, scaled similarly.
 *
 *   MEM_LOAD_RETIRED.L3_MISS  → llc_miss_rate
 *     Core PMU event counting retired loads that missed the LLC.
 *     Provides the latency-pressure signal for decode classification.
 *
 *   OFFCORE_REQUESTS.ALL_DATA_RD → bw_variance (computed)
 *     Used over a sliding window to compute burst variance for
 *     agentic classification.
 *
 *   UNC_M_CMD_RATE             → dram_cmd_rate
 *     IMC uncore total command rate for idle detection.
 *
 * The reference stub returns static values for CI testability.
 */
static struct pmu_sample collect_pmu_sample(void)
{
	static struct pmu_sample s = {
		.write_bw_gbps   = 5,
		.read_bw_gbps    = 8,
		.llc_miss_rate   = 6000,
		.bw_variance_pct = 20,
		.dram_cmd_rate   = 50000,
	};

	return s;
}

/* ── Phase classifier ──────────────────────────────────────────── */

static u8 classify_phase(const struct pmu_sample *s, u8 prev)
{
	/* Rule 1: write-dominant → Prefill */
	if (s->write_bw_gbps > prefill_wr_thresh &&
	    s->write_bw_gbps > s->read_bw_gbps)
		return PHASE_PREFILL;

	/* Rule 2: read-dominant, low writes, high LLC misses → Decode */
	if (s->read_bw_gbps > s->write_bw_gbps * 2 &&
	    s->write_bw_gbps < decode_wr_ceil &&
	    s->llc_miss_rate > decode_llc_floor)
		return PHASE_DECODE;

	/* Rule 3: high variance → Agentic */
	if (s->bw_variance_pct > agentic_var_thresh)
		return PHASE_AGENTIC;

	/* Rule 4: very low command rate → Idle */
	if (s->dram_cmd_rate < idle_cmd_thresh)
		return PHASE_IDLE;

	/* Rule 5: hysteresis — hold previous phase */
	return prev;
}

/* ── Build hint from classified phase ──────────────────────────── */

static struct mem_workload_hint phase_to_hint(u8 phase_id)
{
	struct mem_workload_hint h = {0};

	h.phase_id = phase_id;
	h.priority = 5;

	switch (phase_id) {
	case PHASE_PREFILL:
	case PHASE_FORWARD:
		h.latency_target_ns = 200;
		h.bw_target_gbps    = 400;
		break;
	case PHASE_DECODE:
		h.latency_target_ns = 90;
		h.bw_target_gbps    = 150;
		break;
	case PHASE_AGENTIC:
		h.latency_target_ns = 120;
		h.bw_target_gbps    = 200;
		break;
	case PHASE_BACKWARD:
		h.latency_target_ns = 90;
		h.bw_target_gbps    = 300;
		break;
	case PHASE_IDLE:
	default:
		h.latency_target_ns = 0;
		h.bw_target_gbps    = 0;
		h.priority           = 3;
		break;
	}

	return h;
}

/* ── Timer callback ────────────────────────────────────────────── */

static enum hrtimer_restart pmu_timer_callback(struct hrtimer *t)
{
	struct pmu_sample s = collect_pmu_sample();
	u8 phase = classify_phase(&s, pmu_last_phase);

	if (phase != pmu_last_phase) {
		struct mem_workload_hint h = phase_to_hint(phase);
		mem_hint_apply(&h);
		pmu_last_phase = phase;
	}

	hrtimer_forward_now(t, ms_to_ktime(100));
	return HRTIMER_RESTART;
}

/* ── Init / exit ───────────────────────────────────────────────── */

int mem_hint_pmu_init(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 17, 0)
	hrtimer_setup(&pmu_timer, pmu_timer_callback,
		      CLOCK_MONOTONIC, HRTIMER_MODE_REL);
#else
	hrtimer_init(&pmu_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pmu_timer.function = pmu_timer_callback;
#endif
	hrtimer_start(&pmu_timer, ms_to_ktime(100), HRTIMER_MODE_REL);
	pr_info("mem_hint: PMU classifier started (100ms polling)\n");
	return 0;
}

void mem_hint_pmu_exit(void)
{
	hrtimer_cancel(&pmu_timer);
}
