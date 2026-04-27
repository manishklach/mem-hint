/* SPDX-License-Identifier: GPL-2.0-only
 * mem_hint_safety.c — /dev/mem_hint workload-aware memory hint interface
 * Patent Pending: Indian Patent Application No. 202641053160
 * Inventor: Manish KL, filed 26 April 2026
 * Reference implementation — illustrative MSR addresses, not
 * production silicon register definitions.
 */

/*
 * In production hardware, this logic is implemented as
 * immutable combinational logic within the memory controller
 * ASIC, anchored to the platform hardware root of trust.
 * This file models the firmware-visible behaviour of that
 * hardware logic. No software path — including this kernel
 * module — can override the hardware implementation.
 * Relates to patent claims 9 and 10.
 */

#include <linux/kernel.h>
#include <linux/module.h>

#include "mem_hint.h"

/* ── Safety clamp ──────────────────────────────────────────────── */

struct phy_config safety_clamp(
	struct phy_config   p,
	struct jedec_limits l,
	struct ecc_telemetry ecc,
	u8 security_level)
{
	/* Hard clamp all timing fields to JEDEC SPD bounds */
	p.tRCD = clamp_t(u8, p.tRCD, l.min_tRCD, l.max_tRCD);
	p.tCL  = clamp_t(u8, p.tCL,  l.min_tCL,  l.max_tCL);
	p.tRP  = clamp_t(u8, p.tRP,  l.min_tRP,  l.max_tRP);
	p.vswing_mv = clamp_t(u16, p.vswing_mv,
			      l.min_vswing_mv, l.max_vswing_mv);

	/* ECC feedback: relax 1 clk if correctable rate > 10^-6
	   (correctable_rate stored as errors per 10^8 accesses,
	    so threshold = 100) */
	if (ecc.correctable_rate > 100ULL) {
		p.tRCD = min_t(u8, p.tRCD + 1, l.max_tRCD);
		p.tCL  = min_t(u8, p.tCL  + 1, l.max_tCL);
		pr_debug("mem_hint: safety: ECC relax tRCD=%u tCL=%u\n",
			 p.tRCD, p.tCL);
	}

	/* Security hardening: hold at or above JEDEC nominal
	   (claim 17: security_level > 0 → timing >= nominal) */
	if (security_level > 0) {
		p.tRCD = max_t(u8, p.tRCD, 22);
		p.tCL  = max_t(u8, p.tCL,  22);
		p.tRP  = max_t(u8, p.tRP,  22);
		pr_debug("mem_hint: safety: security floor applied\n");
	}

	return p;
}

/* ── Self-test ─────────────────────────────────────────────────── */

void safety_limiter_selftest(void)
{
	struct phy_config test, result;
	struct ecc_telemetry test_ecc;
	int pass = 1;

	/* Test 1: tRCD=8 (below min=14) → clamped to 14 */
	test = (struct phy_config){
		.tRCD = 8, .tCL = 22, .tRP = 22, .tRAS = 52,
		.vswing_mv = 300, .dfe_tap = {0}, .ctle_gain_db = 0,
		.refresh_mode = 0
	};
	test_ecc = (struct ecc_telemetry){0};
	result = safety_clamp(test, platform_limits, test_ecc, 0);
	if (result.tRCD != 14) {
		pr_err("mem_hint: selftest FAIL: tRCD below min: got %u, want 14\n",
		       result.tRCD);
		pass = 0;
	} else {
		pr_info("mem_hint: selftest PASS: tRCD below min clamped to 14\n");
	}

	/* Test 2: tRCD=40 (above max=32) → clamped to 32 */
	test.tRCD = 40;
	result = safety_clamp(test, platform_limits, test_ecc, 0);
	if (result.tRCD != 32) {
		pr_err("mem_hint: selftest FAIL: tRCD above max: got %u, want 32\n",
		       result.tRCD);
		pass = 0;
	} else {
		pr_info("mem_hint: selftest PASS: tRCD above max clamped to 32\n");
	}

	/* Test 3: vswing=100 (below min=200) → clamped to 200 */
	test.tRCD = 22;
	test.vswing_mv = 100;
	result = safety_clamp(test, platform_limits, test_ecc, 0);
	if (result.vswing_mv != 200) {
		pr_err("mem_hint: selftest FAIL: vswing below min: got %u, want 200\n",
		       result.vswing_mv);
		pass = 0;
	} else {
		pr_info("mem_hint: selftest PASS: vswing below min clamped to 200\n");
	}

	/* Test 4: security_level=1, tRCD=16 → raised to 22 */
	test.vswing_mv = 300;
	test.tRCD = 16;
	result = safety_clamp(test, platform_limits, test_ecc, 1);
	if (result.tRCD != 22) {
		pr_err("mem_hint: selftest FAIL: security floor: got %u, want 22\n",
		       result.tRCD);
		pass = 0;
	} else {
		pr_info("mem_hint: selftest PASS: security floor raised tRCD to 22\n");
	}

	/* Test 5: ecc.correctable_rate=200, tRCD=18 → relaxed to 19 */
	test.tRCD = 18;
	test_ecc.correctable_rate = 200;
	result = safety_clamp(test, platform_limits, test_ecc, 0);
	if (result.tRCD != 19) {
		pr_err("mem_hint: selftest FAIL: ECC relax: got %u, want 19\n",
		       result.tRCD);
		pass = 0;
	} else {
		pr_info("mem_hint: selftest PASS: ECC relax tRCD 18→19\n");
	}

	if (pass)
		pr_info("mem_hint: safety limiter selftest complete — all PASS\n");
	else
		pr_warn("mem_hint: safety limiter selftest complete — some FAIL\n");
}
