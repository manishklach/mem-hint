/* SPDX-License-Identifier: GPL-2.0-only
 * mem_hint_pmu.c - PMU auto-classification for AI Memory Systems
 * Patent Pending: Indian Patent Application No. 202641053160
 * Inventor: Manish KL, filed 26 April 2026
 * Reference implementation - PMU counters are illustrative.
 */

#include <linux/hrtimer.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>
#include <linux/moduleparam.h>

struct pmu_sample {
	u32 write_bw_gbps;
	u32 read_bw_gbps;
	u32 llc_miss_rate;
	u32 bw_variance_pct;
	u32 dram_cmd_rate;
};

static unsigned int prefill_wr_thresh = 10;
static unsigned int decode_wr_ceil = 1;
static unsigned int decode_llc_floor = 5000;
static unsigned int agentic_var_thresh = 50;
static unsigned int idle_cmd_thresh = 1000;
static struct hrtimer pmu_poll_timer;

module_param(prefill_wr_thresh, uint, 0644);
module_param(decode_wr_ceil, uint, 0644);
module_param(decode_llc_floor, uint, 0644);
module_param(agentic_var_thresh, uint, 0644);
module_param(idle_cmd_thresh, uint, 0644);

/*
 * Real hardware would source these fields from platform PMU blocks:
 * write_bw_gbps -> UNC_M_CAS_COUNT.WR (IMC uncore)
 * read_bw_gbps  -> UNC_M_CAS_COUNT.RD (IMC uncore)
 * llc_miss_rate -> MEM_LOAD_RETIRED.L3_MISS (core PMU)
 * bw_variance   -> computed over a sliding 10-sample window
 * dram_cmd_rate -> UNC_M_CMD_RATE (IMC uncore)
 */
static struct pmu_sample collect_pmu_sample(void)
{
	struct pmu_sample sample;
	u64 tick;

	/*
	 * Real implementation would use perf_event_create_kernel_counter()
	 * against uncore IMC and core PMU events, then convert counter
	 * deltas into rates over the 100 us polling interval. This stub
	 * simulates changing demand so the reference control path can be
	 * exercised on ordinary development systems.
	 */
	tick = ktime_get_ns() / 100000ULL;
	sample.write_bw_gbps = (tick % 7 == 0) ? 14 : ((tick % 5 == 0) ? 0 : 1);
	sample.read_bw_gbps = (tick % 3 == 0) ? 6 : 3;
	sample.llc_miss_rate = (tick % 4 == 0) ? 7000 : 1200;
	sample.bw_variance_pct = (tick % 6 == 0) ? 65 : 10;
	sample.dram_cmd_rate = (tick % 11 == 0) ? 800 : 6000;

	return sample;
}

static u8 classify_phase(const struct pmu_sample *sample, u8 prev_phase)
{
	if (sample->write_bw_gbps > prefill_wr_thresh &&
	    sample->write_bw_gbps > sample->read_bw_gbps)
		return PHASE_PREFILL;

	if (sample->read_bw_gbps > (sample->write_bw_gbps * 2) &&
	    sample->write_bw_gbps < decode_wr_ceil &&
	    sample->llc_miss_rate > decode_llc_floor)
		return PHASE_DECODE;

	if (sample->bw_variance_pct > agentic_var_thresh)
		return PHASE_AGENTIC;

	if (sample->dram_cmd_rate < idle_cmd_thresh)
		return PHASE_IDLE;

	return prev_phase;
}

static enum hrtimer_restart mem_hint_pmu_timer(struct hrtimer *timer)
{
	struct pmu_sample sample;
	struct mem_workload_hint hint;
	u8 prev_phase;
	u8 next_phase;

	sample = collect_pmu_sample();
	prev_phase = (u8)atomic_read(&current_phase_id);
	next_phase = classify_phase(&sample, prev_phase);
	if (next_phase != prev_phase) {
		memset(&hint, 0, sizeof(hint));
		hint.phase_id = next_phase;
		hint.priority = 5;
		switch (next_phase) {
		case PHASE_PREFILL:
		case PHASE_FORWARD:
			hint.bw_target_gbps = 400;
			break;
		case PHASE_DECODE:
			hint.latency_target_ns = 90;
			hint.bw_target_gbps = 150;
			break;
		case PHASE_BACKWARD:
			hint.bw_target_gbps = 350;
			break;
		default:
			break;
		}
		mem_hint_apply(&hint);
	}

	hrtimer_forward_now(timer, ns_to_ktime(100000));
	return HRTIMER_RESTART;
}

int mem_hint_pmu_init(void)
{
	hrtimer_init(&pmu_poll_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_PINNED);
	pmu_poll_timer.function = mem_hint_pmu_timer;
	hrtimer_start(&pmu_poll_timer, ns_to_ktime(100000), HRTIMER_MODE_REL_PINNED);
	pr_info("mem_hint: PMU auto-classifier started at 100us cadence\n");
	return 0;
}

void mem_hint_pmu_exit(void)
{
	hrtimer_cancel(&pmu_poll_timer);
	pr_info("mem_hint: PMU auto-classifier stopped\n");
}
