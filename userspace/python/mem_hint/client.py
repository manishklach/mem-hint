"""
MemHintClient — Python client for /dev/mem_hint.
Patent Pending: Indian Patent Application No. 202641053160
Inventor: Manish KL
"""

import os
import struct
import logging
from typing import Optional

from mem_hint.phases import (
    PHASE_PREFILL, PHASE_DECODE, PHASE_AGENTIC,
    PHASE_IDLE, PHASE_FORWARD, PHASE_BACKWARD,
)

logger = logging.getLogger(__name__)

HINT_FORMAT = "<BHHBBB"
HINT_SIZE = struct.calcsize(HINT_FORMAT)
assert HINT_SIZE == 8, f"Hint struct must be 8 bytes, got {HINT_SIZE}"
SYSFS_BASE = "/sys/bus/platform/drivers/mem_hint"


class MemHintClient:
    """
    Client for the /dev/mem_hint kernel interface.

    Usage:
        with MemHintClient() as c:
            c.prefill(bw_gbps=400)
            c.decode(latency_ns=90)
            c.idle()

    dry_run=True: log hints without opening /dev/mem_hint.
    """

    def __init__(self, device="/dev/mem_hint", dry_run=False):
        self.device = device
        self.dry_run = dry_run
        self._fd: Optional[int] = None
        if not dry_run:
            self._open()

    def _open(self):
        if self._fd is not None:
            return
        if not os.path.exists(self.device):
            logger.warning(
                "%s not found. Use dry_run=True.", self.device)
            self.dry_run = True
            return
        self._fd = os.open(self.device, os.O_WRONLY)

    def close(self):
        if self._fd is not None:
            os.close(self._fd)
            self._fd = None

    def __enter__(self):
        return self

    def __exit__(self, *a):
        self.close()

    def send(self, phase_id, latency_ns=0, bw_gbps=0,
             security=0, priority=7):
        """Send a raw workload hint to the kernel driver."""
        payload = struct.pack(
            HINT_FORMAT,
            phase_id & 0xFF,
            latency_ns & 0xFFFF,
            bw_gbps & 0xFFFF,
            security & 0xFF,
            min(priority, 7) & 0xFF,
            0,
        )
        if self.dry_run:
            logger.debug(
                "dry_run: phase=0x%02x lat=%d bw=%d pri=%d",
                phase_id, latency_ns, bw_gbps, priority,
            )
            return
        if self._fd is None:
            self._open()
        if self._fd is not None:
            os.write(self._fd, payload)

    def prefill(self, bw_gbps=400, priority=7):
        self.send(PHASE_PREFILL, 200, bw_gbps, priority=priority)

    def decode(self, latency_ns=90, priority=7):
        self.send(PHASE_DECODE, latency_ns, 150, priority=priority)

    def agentic(self, priority=5):
        self.send(PHASE_AGENTIC, 120, 200, priority=priority)

    def idle(self, priority=3):
        self.send(PHASE_IDLE, 0, 0, priority=priority)

    def forward_pass(self, bw_gbps=400):
        self.send(PHASE_FORWARD, 200, bw_gbps, priority=7)

    def backward_pass(self):
        self.send(PHASE_BACKWARD, 90, 300, priority=7)

    def read_status(self, attr):
        path = os.path.join(SYSFS_BASE, "status", attr)
        try:
            with open(path) as f:
                return f.read().strip()
        except (FileNotFoundError, PermissionError):
            return ""

    def set_policy(self, attr, value):
        path = os.path.join(SYSFS_BASE, "policy", attr)
        try:
            with open(path, "w") as f:
                f.write(str(value))
        except (FileNotFoundError, PermissionError) as e:
            logger.warning("sysfs write %s: %s", attr, e)

    def get_current_phase(self):
        v = self.read_status("current_phase")
        try:
            return int(v, 0)
        except (ValueError, TypeError):
            return 0

    def get_p99_latency(self):
        v = self.read_status("p99_latency_ns")
        try:
            return int(v)
        except (ValueError, TypeError):
            return 0
