# mem-hint Python Package

Patent Pending: Indian Patent Application No. 202641053160.

This package exposes a thin Python interface for the illustrative `/dev/mem_hint` Linux kernel driver. It includes:

- `MemHintClient` for direct device and sysfs interaction
- `MemHintScheduler` for thread-safe runtime hooks
- Example integrations for vLLM, TensorRT-LLM, and PyTorch-style training loops

## Install

```bash
pip install -e .
```

## Quick example

```python
from mem_hint import MemHintClient

with MemHintClient() as client:
    client.prefill(bw_gbps=400, priority=7)
    client.decode(latency_ns=90, priority=7)
    print(client.get_current_phase())
```
