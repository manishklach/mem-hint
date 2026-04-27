#!/usr/bin/env python3
"""
demo_sequence.py — Dry-run demo of /dev/mem_hint phase signaling.

Patent Pending: Indian Patent Application No. 202641053160
Inventor: Manish KL

Runs entirely in dry_run mode — no kernel module needed.
Safe to run on any machine (Linux, macOS, Windows).

Usage:
    python demo_sequence.py
"""

import time
import struct
import logging

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s  %(message)s",
    datefmt="%H:%M:%S",
)
log = logging.getLogger("demo")

from mem_hint import MemHintClient  # noqa: E402
from mem_hint.phases import PHASES, PHASE_PREFILL, PHASE_DECODE  # noqa: E402
from mem_hint.phases import PHASE_AGENTIC, PHASE_IDLE  # noqa: E402

HINT_FORMAT = "<BHHBBB"

PHASE_NAMES = {
    0x01: "PREFILL",
    0x02: "DECODE",
    0x03: "AGENTIC",
    0x04: "IDLE",
    0x05: "FORWARD",
    0x06: "BACKWARD",
}

EXPLANATIONS = {
    0x01: "High-bandwidth prompt ingestion / KV cache fill. "
          "Memory controller targets max sustained BW (tRCD=22).",
    0x02: "Low-latency token generation. Read-heavy KV access. "
          "Controller tightens timing (tRCD=18, tCL=18) for min P99.",
    0x03: "Bursty tool-use / API calls / planning loops. "
          "Controller uses balanced timing (tRCD=20) with burst tolerance.",
    0x04: "Idle between requests. Near-zero command rate. "
          "Controller relaxes timing, enables PLL reduction for power savings.",
    0x05: "Training forward pass. Same profile as prefill — "
          "max sustained bandwidth.",
    0x06: "Training backward pass. Write-heavy gradient traffic. "
          "Controller optimizes for write bandwidth (tRAS=38).",
}


def format_hint(phase_id, lat_ns, bw_gbps, security, priority):
    """Pack and display the 8-byte hint structure."""
    payload = struct.pack(
        HINT_FORMAT,
        phase_id & 0xFF,
        lat_ns & 0xFFFF,
        bw_gbps & 0xFFFF,
        security & 0xFF,
        min(priority, 7) & 0xFF,
        0,
    )
    hex_str = " ".join(f"{b:02x}" for b in payload)
    return hex_str, len(payload)


def demo_step(client, phase_id, lat_ns, bw_gbps, priority,
              description):
    """Execute one demo step and log it."""
    name = PHASE_NAMES.get(phase_id, "UNKNOWN")
    hex_str, size = format_hint(phase_id, lat_ns, bw_gbps, 0, priority)

    log.info("=" * 64)
    log.info("PHASE: %s (0x%02x)  priority=%d", name, phase_id, priority)
    log.info("  latency_target = %d ns", lat_ns)
    log.info("  bw_target      = %d GB/s", bw_gbps)
    log.info("  hint payload   = [%s] (%d bytes)", hex_str, size)
    log.info("  %s", description)
    log.info("")

    # Dispatch through the client (dry-run: logged, not written)
    client.send(phase_id, lat_ns, bw_gbps, priority=priority)
    time.sleep(0.5)


def main():
    log.info("=" * 64)
    log.info("/dev/mem_hint — Phase Sequence Demo (dry_run)")
    log.info("Patent Pending: IN 202641053160")
    log.info("=" * 64)
    log.info("")

    phase_info = PHASES[PHASE_DECODE]
    log.info("Phase table check: DECODE lat=%d bw=%d",
             phase_info.latency_target_ns, phase_info.bw_target_gbps)
    log.info("")

    with MemHintClient(dry_run=True) as client:
        # Step 1: Prefill
        demo_step(client, PHASE_PREFILL, 200, 400, 7,
                  EXPLANATIONS[PHASE_PREFILL])

        # Step 2: Decode
        demo_step(client, PHASE_DECODE, 90, 150, 7,
                  EXPLANATIONS[PHASE_DECODE])

        # Step 3: Agentic (tool call during inference)
        demo_step(client, PHASE_AGENTIC, 120, 200, 5,
                  EXPLANATIONS[PHASE_AGENTIC])

        # Step 4: Back to Decode (after tool returns)
        demo_step(client, PHASE_DECODE, 90, 150, 7,
                  EXPLANATIONS[PHASE_DECODE])

        # Step 5: Idle (request completed)
        demo_step(client, PHASE_IDLE, 0, 0, 3,
                  EXPLANATIONS[PHASE_IDLE])

    log.info("=" * 64)
    log.info("Demo complete. All hints were dry-run (no hardware writes).")
    log.info("In production, each hint would reach the memory controller")
    log.info("via MSR/MMIO/CXL DVSEC within the 500us pre-adjustment window.")
    log.info("=" * 64)


if __name__ == "__main__":
    main()
