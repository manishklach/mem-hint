"""
MemHintClient — Python interface to /dev/mem_hint.
Patent Pending: Indian Patent Application No. 202641053160
"""

import logging
import os
import struct
from typing import Optional

from mem_hint.phases import (PHASE_AGENTIC, PHASE_BACKWARD, PHASE_DECODE,
                             PHASE_FORWARD, PHASE_IDLE, PHASE_PREFILL)

logger = logging.getLogger(__name__)

HINT_FORMAT = "<BHHBB2s"
HINT_SIZE = struct.calcsize(HINT_FORMAT)
assert HINT_SIZE == 9, f"Hint struct must be 9 bytes, got {HINT_SIZE}"

SYSFS_BASE = "/sys/bus/platform/drivers/mem_hint"


class MemHintClient:
    """
    Client for the /dev/mem_hint kernel interface.

    Usage:
        with MemHintClient() as c:
            c.prefill(bw_gbps=400)
            # ... run prefill ...
            c.decode(latency_ns=90)

    dry_run=True: log hints without opening /dev/mem_hint.
    Useful for testing on systems without the kernel module loaded.
    """

    def __init__(self,
                 device: str = "/dev/mem_hint",
                 dry_run: bool = False):
        self.device = device
        self.dry_run = dry_run
        self._fd: Optional[int] = None
        if not dry_run:
            self._open()

    def _open(self) -> None:
        if self._fd is not None:
            return
        if not os.path.exists(self.device):
            logger.warning(
                "%s not found — is mem_hint.ko loaded? "
                "Use dry_run=True for testing.", self.device
            )
            self.dry_run = True
            return
        self._fd = os.open(self.device, os.O_WRONLY)

    def close(self) -> None:
        if self._fd is not None:
            os.close(self._fd)
            self._fd = None

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    def send(self,
             phase_id: int,
             latency_ns: int = 0,
             bw_gbps: int = 0,
             security: int = 0,
             priority: int = 7) -> None:
        """Send a raw workload hint to the kernel driver."""
        phase_id = int(phase_id) & 0xFF
        latency = int(latency_ns) & 0xFFFF
        bw = int(bw_gbps) & 0xFFFF
        security = int(security) & 0xFF
        priority = min(int(priority), 7) & 0xFF

        payload = struct.pack(HINT_FORMAT,
                              phase_id, latency, bw,
                              security, priority, b"\x00\x00")
        assert len(payload) == HINT_SIZE

        if self.dry_run:
            logger.debug(
                "dry_run: phase=0x%02x lat=%dns bw=%dGBs prio=%d",
                phase_id, latency_ns, bw_gbps, priority
            )
            return

        if self._fd is None:
            self._open()
        if self._fd is not None:
            os.write(self._fd, payload)

    def prefill(self, bw_gbps: int = 400, priority: int = 7) -> None:
        self.send(PHASE_PREFILL, latency_ns=200,
                  bw_gbps=bw_gbps, priority=priority)

    def decode(self, latency_ns: int = 90, priority: int = 7) -> None:
        self.send(PHASE_DECODE, latency_ns=latency_ns,
                  bw_gbps=150, priority=priority)

    def agentic(self, priority: int = 5) -> None:
        self.send(PHASE_AGENTIC, latency_ns=120,
                  bw_gbps=200, priority=priority)

    def idle(self, priority: int = 3) -> None:
        self.send(PHASE_IDLE, latency_ns=0,
                  bw_gbps=0, priority=priority)

    def forward_pass(self, bw_gbps: int = 400) -> None:
        self.send(PHASE_FORWARD, latency_ns=200,
                  bw_gbps=bw_gbps, priority=7)

    def backward_pass(self) -> None:
        self.send(PHASE_BACKWARD, latency_ns=90,
                  bw_gbps=300, priority=7)

    def read_status(self, attr: str) -> str:
        path = os.path.join(SYSFS_BASE, "status", attr)
        try:
            with open(path) as f:
                return f.read().strip()
        except (FileNotFoundError, PermissionError) as e:
            logger.debug("sysfs read %s: %s", path, e)
            return ""

    def set_policy(self, attr: str, value: int) -> None:
        path = os.path.join(SYSFS_BASE, "policy", attr)
        try:
            with open(path, "w") as f:
                f.write(str(value))
        except (FileNotFoundError, PermissionError) as e:
            logger.warning("sysfs write %s=%d: %s", attr, value, e)

    def get_current_phase(self) -> int:
        v = self.read_status("current_phase")
        try:
            return int(v, 0)
        except (ValueError, TypeError):
            return 0

    def get_p99_latency(self) -> int:
        v = self.read_status("p99_latency_ns")
        try:
            return int(v)
        except (ValueError, TypeError):
            return 0
