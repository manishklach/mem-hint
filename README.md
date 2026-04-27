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

## Why This Matters Now

AI runtimes are increasingly memory-phase-sensitive while system software still treats memory behavior as mostly static. Long-context inference, decode-heavy serving, tool-augmented agent loops, and distributed training all place materially different demands on memory latency, bandwidth, and signaling margin. A narrow interface for phase intent is becoming easier to justify even when the underlying hardware hooks are not standardized yet.

This repository is useful now because it makes that interface concrete. It gives kernel, runtime, firmware, and architecture teams something specific to review, test, critique, and refine instead of discussing the concept only at a whiteboard level.

## What Is Real Vs Illustrative

Real in this repository:

- the `/dev/mem_hint` character-device contract
- userspace C and Python client interfaces
- sysfs policy and status surface
- a compilable Linux kernel reference module structure
- validation and CI checks for repository hygiene

Illustrative in this repository:

- `MEM_HINT_MSR` and any privileged hardware register address
- MMIO controller mappings
- CXL DVSEC register programming
- PMU signal collection logic beyond a simulated classifier stub
- the safety limiter as hardware enforcement rather than a software model

This is a reference implementation, not real hardware programming. By default the kernel module suppresses illustrative privileged writes so a development machine does not touch undefined MSR or MMIO state.

## Current Implementation Status

- `/dev` interface: implemented
- Python client and scheduler: implemented
- Userspace examples: implemented
- PMU classifier: simulated
- MSR, MMIO, and CXL paths: illustrative and disabled by default
- Safety limiter: software model of a hardware concept
- Real silicon support: not implemented

See [docs/status.md](docs/status.md) for a more candid implementation matrix.

## How To Validate Locally

```bash
# Full repo validation
make validate

# Kernel-only build check
make kernel

# Userspace library and examples
make userspace

# Python syntax and optional lint
make python-check
```

The validation script checks required files, Python syntax, optional `flake8`, kernel buildability when local headers are available, and patent notice coverage in `kernel/*.c` and `kernel/*.h`.

## Roadmap Before Public Release

- add diagrams and screenshots for the sysfs and CLI flow
- verify GitHub Pages links and the blog-page integration
- expand the validation script with markdown/link checks
- add explicit runtime examples for more realistic serving code paths
- keep kernel behavior conservative and clearly illustrative
- review naming, comments, and licensing for public-facing clarity

## sysfs Interface

Key paths:

- `/sys/bus/platform/drivers/mem_hint/policy/`
- `/sys/bus/platform/drivers/mem_hint/thresholds/`
- `/sys/bus/platform/drivers/mem_hint/status/current_phase`
- `/sys/bus/platform/drivers/mem_hint/status/p99_latency_ns`
- `/sys/bus/platform/drivers/mem_hint/status/active_channel`

## GitHub Pages

The repository includes a static site under [`site/`](site/) and a dedicated `gh-pages` branch layout for publication. In GitHub repository Settings, set Pages to serve from the `gh-pages` branch and the root folder so the article is published at:

`https://manishklach.github.io/writings/dev-mem-hint-kernel-control-plane-ai-memory-systems.html`

## Patent Notice

This repository is associated with Indian Patent Application No. `202641053160`, filed by Manish KL on `26 April 2026`. It is published as a reference implementation for research, educational, and interoperability discussion purposes. See [PATENT_NOTICE.md](PATENT_NOTICE.md) for licensing and contact details.

## Contributing

Contribution guidelines live in [CONTRIBUTING.md](CONTRIBUTING.md).

## License

This repository uses a research-and-education-only custom license with an accompanying patent notice. See [LICENSE](LICENSE) and [PATENT_NOTICE.md](PATENT_NOTICE.md).
