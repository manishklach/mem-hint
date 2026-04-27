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
        id=PHASE_PREFILL, name="Prefill",
        latency_target_ns=200, bw_target_gbps=400,
        description="High bandwidth prompt ingestion / KV cache fill",
        memory_req="Max sustained bandwidth"
    ),
    PHASE_DECODE: Phase(
        id=PHASE_DECODE, name="Decode",
        latency_target_ns=90, bw_target_gbps=150,
        description="Low-latency token generation, read-heavy KV access",
        memory_req="Min P99 latency"
    ),
    PHASE_AGENTIC: Phase(
        id=PHASE_AGENTIC, name="Agentic",
        latency_target_ns=120, bw_target_gbps=200,
        description="Bursty tool-use, external API calls, planning loops",
        memory_req="Burst tolerance"
    ),
    PHASE_IDLE: Phase(
        id=PHASE_IDLE, name="Idle",
        latency_target_ns=0, bw_target_gbps=0,
        description="Between inference requests, near-zero command rate",
        memory_req="Minimum power"
    ),
    PHASE_FORWARD: Phase(
        id=PHASE_FORWARD, name="ForwardPass",
        latency_target_ns=200, bw_target_gbps=400,
        description="Training forward pass, same profile as prefill",
        memory_req="Max sustained bandwidth"
    ),
    PHASE_BACKWARD: Phase(
        id=PHASE_BACKWARD, name="BackwardPass",
        latency_target_ns=90, bw_target_gbps=300,
        description="Training backward pass, gradient write heavy",
        memory_req="High write bandwidth (tWR reduced)"
    ),
}
