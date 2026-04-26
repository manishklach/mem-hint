# Patent Pending: Indian Patent Application No. 202641053160
# Inventor: Manish KL, filed 26 April 2026

import threading

from .client import MemHintClient


class MemHintScheduler:
    """Thread-safe convenience scheduler for inference and training hooks."""

    def __init__(self, device="/dev/mem_hint"):
        self._client = MemHintClient(device=device)
        self._lock = threading.Lock()

    def __enter__(self):
        self._client.__enter__()
        return self

    def __exit__(self, exc_type, exc, tb):
        return self._client.__exit__(exc_type, exc, tb)

    def on_prefill_start(self, batch_size: int):
        bw = 400 + max(batch_size - 1, 0) * 10
        with self._lock:
            self._client.prefill(bw_gbps=bw, priority=7)

    def on_decode_start(self, request_id: str):
        priority = 7 if request_id else 5
        with self._lock:
            self._client.decode(latency_ns=90, priority=priority)

    def on_tool_call(self, tool_name: str):
        priority = 6 if tool_name else 5
        with self._lock:
            self._client.agentic(priority=priority)

    def on_idle(self):
        with self._lock:
            self._client.idle(priority=3)

    def on_forward_pass(self, batch_size: int):
        bw = 400 + max(batch_size - 1, 0) * 8
        with self._lock:
            self._client.forward_pass(bw_gbps=bw)

    def on_backward_pass(self, step: int):
        _ = step
        with self._lock:
            self._client.backward_pass()
