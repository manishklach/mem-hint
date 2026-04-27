# Demo Output

This document shows the expected output from `userspace/python/examples/demo_sequence.py`, which demonstrates the `/dev/mem_hint` phase signaling interface in dry-run mode.

## Running the Demo

```bash
cd userspace/python
pip install -e . --quiet
python examples/demo_sequence.py
```

No kernel module or root privileges are needed. The demo runs entirely in `dry_run=True` mode.

## Expected Output

```
07:00:00  ================================================================
07:00:00  /dev/mem_hint — Phase Sequence Demo (dry_run)
07:00:00  Patent Pending: IN 202641053160
07:00:00  ================================================================
07:00:00
07:00:00  Phase table check: DECODE lat=90 bw=150
07:00:00
07:00:00  ================================================================
07:00:00  PHASE: PREFILL (0x01)  priority=7
07:00:00    latency_target = 200 ns
07:00:00    bw_target      = 400 GB/s
07:00:00    hint payload   = [01 c8 00 90 01 00 07 00] (8 bytes)
07:00:00    High-bandwidth prompt ingestion / KV cache fill. Memory
07:00:00    controller targets max sustained BW (tRCD=22).
07:00:00
07:00:01  ================================================================
07:00:01  PHASE: DECODE (0x02)  priority=7
07:00:01    latency_target = 90 ns
07:00:01    bw_target      = 150 GB/s
07:00:01    hint payload   = [02 5a 00 96 00 00 07 00] (8 bytes)
07:00:01    Low-latency token generation. Read-heavy KV access. Controller
07:00:01    tightens timing (tRCD=18, tCL=18) for min P99.
07:00:01
07:00:01  ================================================================
07:00:01  PHASE: AGENTIC (0x03)  priority=5
07:00:01    latency_target = 120 ns
07:00:01    bw_target      = 200 GB/s
07:00:01    hint payload   = [03 78 00 c8 00 00 05 00] (8 bytes)
07:00:01    Bursty tool-use / API calls / planning loops. Controller uses
07:00:01    balanced timing (tRCD=20) with burst tolerance.
07:00:01
07:00:02  ================================================================
07:00:02  PHASE: DECODE (0x02)  priority=7
07:00:02    latency_target = 90 ns
07:00:02    bw_target      = 150 GB/s
07:00:02    hint payload   = [02 5a 00 96 00 00 07 00] (8 bytes)
07:00:02    Low-latency token generation. Read-heavy KV access. Controller
07:00:02    tightens timing (tRCD=18, tCL=18) for min P99.
07:00:02
07:00:02  ================================================================
07:00:02  PHASE: IDLE (0x04)  priority=3
07:00:02    latency_target = 0 ns
07:00:02    bw_target      = 0 GB/s
07:00:02    hint payload   = [04 00 00 00 00 00 03 00] (8 bytes)
07:00:02    Idle between requests. Near-zero command rate. Controller relaxes
07:00:02    timing, enables PLL reduction for power savings.
07:00:02
07:00:03  ================================================================
07:00:03  Demo complete. All hints were dry-run (no hardware writes).
07:00:03  In production, each hint would reach the memory controller
07:00:03  via MSR/MMIO/CXL DVSEC within the 500us pre-adjustment window.
07:00:03  ================================================================
```

## Phase Explanation

| Phase | ID | What Happens | Memory Controller Response |
|-------|----|--------------|----|
| **PREFILL** | 0x01 | Prompt tokens ingested, KV cache filled | Max sustained BW mode: tRCD=22, vswing=300mV |
| **DECODE** | 0x02 | Autoregressive token generation | Low-latency mode: tRCD=18, tCL=18, vswing=280mV |
| **AGENTIC** | 0x03 | Tool calls, planning, bursty access | Balanced mode: tRCD=20, burst tolerance enabled |
| **IDLE** | 0x04 | No active requests | Power-save: tRCD=24, PLL reduction, vswing=240mV |
| **FORWARD** | 0x05 | Training forward pass | Same as Prefill — max sustained BW |
| **BACKWARD** | 0x06 | Training backward pass (gradients) | Write-optimized: tRAS=38, high write throughput |

## The 8-Byte Hint Structure

Each hint is an 8-byte packed struct sent via `write()` to `/dev/mem_hint`:

```
Offset  Size  Field               Example (DECODE)
0       1     phase_id            0x02
1       2     latency_target_ns   90 (0x005a LE)
3       2     bw_target_gbps      150 (0x0096 LE)
5       1     security_level      0x00
6       1     priority            0x07 (max)
7       1     reserved            0x00
```

## What Happens in Production

In a real deployment with the kernel module loaded:

1. The `write()` syscall copies the 8-byte struct from userspace
2. The kernel validates the phase ID (0x01–0x06) and clamps priority to [0,7]
3. `phase_to_phy_config()` looks up the optimal PHY timing for the phase
4. `safety_clamp()` enforces JEDEC SPD bounds, ECC feedback, and security floor
5. The encoded hint is dispatched via the configured hardware channel (MSR/MMIO/CXL)
6. The memory controller adjusts timing within the 500µs pre-adjustment window
7. sysfs status attributes are updated for observability

The demo shows steps 1-3 in dry-run mode. Steps 4-7 require the kernel module and compatible hardware.
