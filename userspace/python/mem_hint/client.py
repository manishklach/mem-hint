# Patent Pending: Indian Patent Application No. 202641053160
# Inventor: Manish KL, filed 26 April 2026

import os
import struct

from .phases import PHASE_AGENTIC
from .phases import PHASE_BACKWARD
from .phases import PHASE_DECODE
from .phases import PHASE_FORWARD
from .phases import PHASE_IDLE
from .phases import PHASE_PREFILL

STATUS_BASE = "/sys/bus/platform/drivers/mem_hint/status"
POLICY_BASE = "/sys/bus/platform/drivers/mem_hint/policy"


class MemHintClient:
    """Userspace client for the illustrative /dev/mem_hint driver."""

    def __init__(self, device="/dev/mem_hint"):
        self.device = device
        self.fd = None

    def __enter__(self):
        self.fd = os.open(self.device, os.O_WRONLY)
        return self

    def __exit__(self, exc_type, exc, tb):
        if self.fd is not None:
            os.close(self.fd)
            self.fd = None
        return False

    def _ensure_open(self):
        if self.fd is None:
            raise RuntimeError("MemHintClient must be opened with a context")

    def send(
        self,
        phase_id,
        latency_ns=0,
        bw_gbps=0,
        security=0,
        priority=7,
    ):
        self._ensure_open()
        priority = min(max(int(priority), 0), 7)
        payload = struct.pack(
            "<BHHBB2s",
            int(phase_id),
            int(latency_ns),
            int(bw_gbps),
            int(security),
            priority,
            b"\x00\x00",
        )
        os.write(self.fd, payload)

    def prefill(self, bw_gbps=400, priority=7):
        self.send(PHASE_PREFILL, bw_gbps=bw_gbps, priority=priority)

    def decode(self, latency_ns=90, priority=7):
        self.send(PHASE_DECODE, latency_ns=latency_ns, bw_gbps=150,
                  priority=priority)

    def agentic(self, priority=5):
        self.send(PHASE_AGENTIC, priority=priority)

    def idle(self, priority=3):
        self.send(PHASE_IDLE, priority=priority)

    def forward_pass(self, bw_gbps=400):
        self.send(PHASE_FORWARD, bw_gbps=bw_gbps, priority=7)

    def backward_pass(self):
        self.send(PHASE_BACKWARD, bw_gbps=350, priority=7)

    def read_status(self, attr: str) -> str:
        with open(f"{STATUS_BASE}/{attr}", "r", encoding="utf-8") as handle:
            return handle.read().strip()

    def set_policy(self, attr: str, value: int) -> None:
        with open(f"{POLICY_BASE}/{attr}", "w", encoding="utf-8") as handle:
            handle.write(f"{int(value)}\n")

    def get_current_phase(self) -> int:
        return int(self.read_status("current_phase"))

    def get_p99_latency(self) -> int:
        return int(self.read_status("p99_latency_ns"))
