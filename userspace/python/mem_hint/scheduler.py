"""
MemHintScheduler — high-level scheduler wrapper for AI runtimes.
Patent Pending: Indian Patent Application No. 202641053160
"""

import logging
import threading

from mem_hint.client import MemHintClient

logger = logging.getLogger(__name__)


class MemHintScheduler:
    """
    Thread-safe scheduler that wraps MemHintClient.
    Drop-in integration point for vLLM, TensorRT-LLM,
    PyTorch FSDP, and custom agentic runtimes.

    Usage:
        scheduler = MemHintScheduler()
        scheduler.on_prefill_start(batch_size=32)
        # ... run prefill ...
        scheduler.on_decode_start(request_id="req-001")
        # ... generate tokens ...
        scheduler.on_idle()
    """

    def __init__(self,
                 device: str = "/dev/mem_hint",
                 dry_run: bool = False,
                 default_priority: int = 7):
        self._client = MemHintClient(device=device, dry_run=dry_run)
        self._lock = threading.Lock()
        self._default_priority = default_priority
        self._current_phase = 0x04

    def _send(self, method: str, **kwargs) -> None:
        with self._lock:
            getattr(self._client, method)(**kwargs)

    def on_prefill_start(self, batch_size: int = 1) -> None:
        """Call at start of each prefill phase."""
        logger.debug("Phase: PREFILL (batch=%d)", batch_size)
        priority = min(7, self._default_priority)
        self._send("prefill", bw_gbps=400, priority=priority)
        self._current_phase = 0x01

    def on_decode_start(self, request_id: str = "") -> None:
        """Call when switching from prefill to decode."""
        logger.debug("Phase: DECODE (req=%s)", request_id)
        self._send("decode", latency_ns=90,
                   priority=self._default_priority)
        self._current_phase = 0x02

    def on_tool_call(self, tool_name: str = "") -> None:
        """Call when an agentic tool invocation begins."""
        logger.debug("Phase: AGENTIC (tool=%s)", tool_name)
        self._send("agentic", priority=5)
        self._current_phase = 0x03

    def on_idle(self) -> None:
        """Call when no requests are queued."""
        logger.debug("Phase: IDLE")
        self._send("idle", priority=3)
        self._current_phase = 0x04

    def on_forward_pass(self, batch_size: int = 1) -> None:
        """Call at start of training forward pass."""
        logger.debug("Phase: FORWARD_PASS (batch=%d)", batch_size)
        self._send("forward_pass", bw_gbps=400)
        self._current_phase = 0x05

    def on_backward_pass(self, step: int = 0) -> None:
        """Call at start of training backward pass / loss.backward()."""
        logger.debug("Phase: BACKWARD_PASS (step=%d)", step)
        self._send("backward_pass")
        self._current_phase = 0x06

    @property
    def current_phase(self) -> int:
        return self._current_phase

    def get_p99_latency(self) -> int:
        return self._client.get_p99_latency()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.on_idle()
        self._client.close()
