[![Build Status](https://github.com/manishklach/mem-hint/actions/workflows/build.yml/badge.svg)](https://github.com/manishklach/mem-hint/actions/workflows/build.yml)
![Patent Pending](https://img.shields.io/badge/Patent-Pending%20IN%20202641053160-orange)

# /dev/mem_hint — Workload-Aware Memory Hint Interface

A reference implementation of a software-defined, workload-aware memory signaling interface for AI computing systems.

Modern AI systems keep solving compute problems while quietly drowning in memory phase changes. Prefill wants throughput, decode wants latency, training swings between forward-read pressure and backward-write pressure, and none of that intent reaches the memory control plane in a structured way today.

`/dev/mem_hint` models a missing cross-layer contract: runtimes can signal workload phase, the kernel can classify phases automatically from PMU telemetry, policy can be tuned through sysfs, and an immutable safety model clamps all requests before they reach an illustrative hardware privilege channel. This repository is a clean reference implementation of that interface contract, not a claim of shipping silicon register definitions.

```text
+-----------------------------------------------------------------------------------+
| Layer 6: AI Runtime / Training Stack                                              |
| vLLM | TensorRT-LLM | PyTorch FSDP | Megatron | custom agents                     |
+-----------------------------------------------------------------------------------+
| Layer 5: /dev/mem_hint userspace contract                                         |
| C library | Python client | scheduler hooks | examples                            |
+-----------------------------------------------------------------------------------+
| Layer 4: Linux kernel module                                                      |
| char device write path | PMU auto-classifier | sysfs policy/status                |
+-----------------------------------------------------------------------------------+
| Layer 3: Privilege channel                                                        |
| MSR | MMIO | CXL DVSEC                                                            |
+-----------------------------------------------------------------------------------+
| Layer 2: Policy engine + safety limiter                                           |
| phase table | threshold tuning | ECC-aware clamp | security floor                 |
+-----------------------------------------------------------------------------------+
| Layer 1: PHY / memory devices                                                     |
| DDR5 timing knobs | signaling swing | equalization | CXL memory regions           |
+-----------------------------------------------------------------------------------+
```

## Quick Start

```bash
# Build and load the kernel module
cd kernel && make && sudo insmod mem_hint.ko

# Install Python library
pip install -e userspace/python/

# Send your first hint
python3 -c "from mem_hint import MemHintClient; c = MemHintClient(); c.__enter__(); c.decode(latency_ns=90, priority=7); c.__exit__(None, None, None)"
```

## Phase Reference

| Phase | ID | Intent |
| --- | --- | --- |
| Prefill | `0x01` | High bandwidth prompt ingestion / cache population |
| Decode | `0x02` | Low-latency token generation with read-heavy access |
| Agentic | `0x03` | Bursty tool-use or control-plane traffic with variance |
| Idle | `0x04` | Low activity, relaxed timings, power-aware behavior |
| Forward Pass | `0x05` | Training forward pass; same best-mode as prefill |
| Backward Pass | `0x06` | Training backward pass; write-heavy optimization |

## Deployment Modes

Explicit hints are the most direct path: the runtime writes phase intent into `/dev/mem_hint`, the kernel validates it, clamps it, and emits an encoded signal over the selected privilege channel. This mode is best when the software stack already knows phase boundaries.

PMU auto-classification lets the kernel infer phase transitions every `100 us` from IMC and core PMU telemetry. It is useful when modifying the runtime is hard, or when an operator wants a baseline policy without touching application code.

Sysfs policy tuning is the operator-facing control plane for field adaptation. Thresholds and per-phase policy defaults can be updated live while explicit hints and PMU classification continue using the same interface contract.

## sysfs Interface

Key paths:

- `/sys/bus/platform/drivers/mem_hint/policy/`
- `/sys/bus/platform/drivers/mem_hint/thresholds/`
- `/sys/bus/platform/drivers/mem_hint/status/current_phase`
- `/sys/bus/platform/drivers/mem_hint/status/p99_latency_ns`
- `/sys/bus/platform/drivers/mem_hint/status/active_channel`

## Patent Notice

This repository is associated with Indian Patent Application No. `202641053160`, filed by Manish KL on `26 April 2026`. It is published as a reference implementation for research, educational, and interoperability discussion purposes. See [PATENT_NOTICE.md](PATENT_NOTICE.md) for licensing and contact details.

## Contributing

Contribution guidelines live in [CONTRIBUTING.md](CONTRIBUTING.md).

## License

This repository uses a research-and-education-only custom license with an accompanying patent notice. See [LICENSE](LICENSE) and [PATENT_NOTICE.md](PATENT_NOTICE.md).
