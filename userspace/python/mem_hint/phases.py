# Patent Pending: Indian Patent Application No. 202641053160
# Inventor: Manish KL, filed 26 April 2026

from dataclasses import dataclass


PHASE_PREFILL = 0x01
PHASE_DECODE = 0x02
PHASE_AGENTIC = 0x03
PHASE_IDLE = 0x04
PHASE_FORWARD = 0x05
PHASE_BACKWARD = 0x06


@dataclass(frozen=True)
class Phase:
    id: int
    name: str
    latency_target_ns: int
    bw_target_gbps: int
    description: str


PHASES = {
    PHASE_PREFILL: Phase(
        id=PHASE_PREFILL,
        name="prefill",
        latency_target_ns=0,
        bw_target_gbps=400,
        description="Prompt ingestion and KV/cache population.",
    ),
    PHASE_DECODE: Phase(
        id=PHASE_DECODE,
        name="decode",
        latency_target_ns=90,
        bw_target_gbps=150,
        description="Read-heavy token generation with low-latency pressure.",
    ),
    PHASE_AGENTIC: Phase(
        id=PHASE_AGENTIC,
        name="agentic",
        latency_target_ns=0,
        bw_target_gbps=0,
        description="Bursty tool and control-plane oriented activity.",
    ),
    PHASE_IDLE: Phase(
        id=PHASE_IDLE,
        name="idle",
        latency_target_ns=0,
        bw_target_gbps=0,
        description="Low-activity, power-aware relaxed mode.",
    ),
    PHASE_FORWARD: Phase(
        id=PHASE_FORWARD,
        name="forward",
        latency_target_ns=0,
        bw_target_gbps=400,
        description="Training forward pass with prefill-like demand.",
    ),
    PHASE_BACKWARD: Phase(
        id=PHASE_BACKWARD,
        name="backward",
        latency_target_ns=0,
        bw_target_gbps=350,
        description="Training backward pass optimized for write throughput.",
    ),
}
