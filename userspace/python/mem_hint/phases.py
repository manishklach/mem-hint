from dataclasses import dataclass

PHASE_PREFILL = 0x01
PHASE_DECODE = 0x02
PHASE_AGENTIC = 0x03
PHASE_IDLE = 0x04
PHASE_FORWARD = 0x05
PHASE_BACKWARD = 0x06


@dataclass
class Phase:
    id: int
    name: str
    latency_target_ns: int
    bw_target_gbps: int
    description: str
    memory_req: str


PHASES = {
    PHASE_PREFILL: Phase(
        0x01, "Prefill", 200, 400,
        "High bandwidth prompt ingestion / KV cache fill",
        "Max sustained bandwidth",
    ),
    PHASE_DECODE: Phase(
        0x02, "Decode", 90, 150,
        "Low-latency token generation, read-heavy KV access",
        "Min P99 latency",
    ),
    PHASE_AGENTIC: Phase(
        0x03, "Agentic", 120, 200,
        "Bursty tool-use, external API calls, planning loops",
        "Burst tolerance",
    ),
    PHASE_IDLE: Phase(
        0x04, "Idle", 0, 0,
        "Between inference requests, near-zero command rate",
        "Minimum power",
    ),
    PHASE_FORWARD: Phase(
        0x05, "ForwardPass", 200, 400,
        "Training forward pass, same profile as prefill",
        "Max sustained bandwidth",
    ),
    PHASE_BACKWARD: Phase(
        0x06, "BackwardPass", 90, 300,
        "Training backward pass, gradient write heavy (tWR reduced)",
        "High write bandwidth",
    ),
}
