/* SPDX-License-Identifier: GPL-2.0-only
 * mem_hint_safety.c - Safety limiter model for AI Memory Systems
 * Patent Pending: Indian Patent Application No. 202641053160
 * Inventor: Manish KL, filed 26 April 2026
 * Reference implementation - hardware clamp logic is modeled in software.
 */

#include <linux/kernel.h>

/*
 * In real hardware this logic is immutable ASIC combinational logic, not
 * software. This file is the firmware-visible model of that hardware safety
 * limiter so the reference repository can demonstrate the contract between
 * policy software and enforced timing bounds. Software cannot override the
 * actual hardware implementation in a real product. The model corresponds to
 * the patent's claims 9 and 10, where workload-aware control is subordinate to
 * non-bypassable safety enforcement and JEDEC-compliant bounds checking.
 */
struct phy_config safety_clamp(struct phy_config proposed,
			       struct jedec_limits limits,
			       struct ecc_telemetry ecc)
{
	struct phy_config clamped;

	clamped = proposed;
	clamped.tRCD = clamp_t(u8, clamped.tRCD, limits.min_tRCD, limits.max_tRCD);
	clamped.tCL = clamp_t(u8, clamped.tCL, limits.min_tCL, limits.max_tCL);
	clamped.tRP = clamp_t(u8, clamped.tRP, limits.min_tRP, limits.max_tRP);
	clamped.vswing_mv = clamp_t(u16, clamped.vswing_mv,
				    limits.min_vswing_mv, limits.max_vswing_mv);

	if (ecc.correctable_rate > 1000000ULL) {
		clamped.tRCD = min_t(u8, clamped.tRCD + 1, limits.max_tRCD);
		clamped.tCL = min_t(u8, clamped.tCL + 1, limits.max_tCL);
	}

	if (mem_hint_security_level > 0) {
		clamped.tRCD = max_t(u8, clamped.tRCD, 22);
		clamped.tCL = max_t(u8, clamped.tCL, 22);
	}

	return clamped;
}

int safety_limiter_selftest(void)
{
	struct phy_config test = {
		.tRCD = 10,
		.tCL = 10,
		.tRP = 40,
		.tRAS = 52,
		.vswing_mv = 800,
		.dfe_tap = { 0, 0, 0, 0 },
		.ctle_gain_db = 0,
		.refresh_mode = 0,
	};
	struct ecc_telemetry test_ecc = {
		.correctable_rate = 2000000ULL,
	};
	struct phy_config result;

	result = safety_clamp(test, platform_limits, test_ecc);
	if (result.tRCD < platform_limits.min_tRCD ||
	    result.tCL < platform_limits.min_tCL ||
	    result.tRP > platform_limits.max_tRP ||
	    result.vswing_mv > platform_limits.max_vswing_mv) {
		pr_err("mem_hint: safety limiter self-test failed\n");
		return -EINVAL;
	}

	pr_info("mem_hint: safety limiter self-test passed\n");
	return 0;
}
