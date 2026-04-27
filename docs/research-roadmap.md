# Research Roadmap

Patent Pending: Indian Patent Application No. 202641053160.

This document outlines the phased research and development plan for `/dev/mem_hint`, from the current simulated reference implementation through to hardware-validated prototypes and upstream integration.

## Phase 1: Simulated Reference Implementation ✅

**Status:** Complete (current state of repository)

Deliverables:
- Linux kernel module with 4 source files (mem_hint.c, mem_hint_pmu.c, mem_hint_sysfs.c, mem_hint_safety.c)
- 6-phase workload classification (Prefill, Decode, Agentic, Idle, ForwardPass, BackwardPass)
- Safety limiter with 5-vector selftest and JEDEC/ECC/security enforcement
- Python client library with dry_run mode for CI and development
- Scheduler hook layer for vLLM, TensorRT-LLM, and PyTorch integration patterns
- sysfs interface for runtime policy and threshold tuning
- 3 hardware dispatch channels (MSR, MMIO, CXL DVSEC) — all illustrative/stubbed
- Documentation suite and GitHub Pages technical article
- GitHub Actions CI with kernel build and Python test validation

Key limitation: All hardware register addresses are illustrative. No hardware validation has been performed.

## Phase 2: PMU-Backed Classifier

**Status:** Planned

Goal: Replace the simulated PMU sample collection stub with real hardware performance counter subscriptions using `perf_event_create_kernel_counter()`.

Target counters:
- `UNC_M_CAS_COUNT.WR` — IMC uncore write CAS command count (→ write_bw_gbps)
- `UNC_M_CAS_COUNT.RD` — IMC uncore read CAS command count (→ read_bw_gbps)
- `MEM_LOAD_RETIRED.L3_MISS` — Core PMU L3 miss count (→ llc_miss_rate)
- `OFFCORE_REQUESTS.ALL_DATA_RD` — Offcore data read requests (→ bw_variance)
- `UNC_M_CMD_RATE` — IMC uncore total command rate (→ idle detection)

Validation plan:
- Test on Intel Sapphire Rapids or Emerald Rapids server with DDR5
- Measure classification accuracy against known workload traces (MLPerf inference, training)
- Compare predicted phase transitions against actual workload state from runtime hooks

Dependencies: Access to server-class hardware with uncore PMU support.

## Phase 3: Firmware Mailbox Prototype

**Status:** Research

Goal: Demonstrate the MMIO dispatch path by writing to a firmware mailbox register on a real memory controller or PHY simulation model.

Approach options:
1. **Rambus PHY simulation model** — If access to Rambus DFE/CTLE register simulation is available, map the `iowrite32()` path to a real register window.
2. **Custom firmware on FPGA** — Implement a minimal memory controller state machine on an FPGA development board (e.g., Xilinx Alveo) that accepts hint writes and adjusts simulated timing parameters.
3. **Platform firmware collaboration** — Partner with a DRAM vendor or memory controller IP provider to integrate the hint protocol into their firmware development environment.

Key question: Which vendor firmware environment provides the most realistic validation path without requiring NDA-restricted register documentation?

## Phase 4: CXL HDM-Region Policy Model

**Status:** Research

Goal: Demonstrate per-region timing adaptation in a fabric-attached memory topology using CXL 2.0/3.0 DVSEC extensions.

Deliverables:
- DVSEC capability layout for a vendor-specific control region
- Per-HDM-region policy register definitions
- Congestion response policy that adjusts remote region timing based on fabric telemetry
- NUMA IPI hint propagation for cross-socket phase coordination

Validation plan:
- Test on CXL-capable platform (e.g., Intel Sapphire Rapids with CXL Type-2 device)
- Or simulate using QEMU CXL emulation with custom DVSEC extensions

Dependencies: CXL hardware or emulation environment.

## Phase 5: Runtime Integrations

**Status:** Planned

Goal: Upstream production-quality phase hooks into major AI serving and training frameworks.

Target integrations:
- **vLLM** — Hook into the scheduler's prefill/decode state machine. The `on_prefill_start()` and `on_decode_start()` hooks map directly to vLLM's request lifecycle.
- **TensorRT-LLM** — Hook into the C++ execution context's phase transitions. Requires a C-level integration rather than Python hooks.
- **PyTorch FSDP** — Hook into the forward/backward pass boundaries via `register_full_backward_hook()`. The `on_forward_pass()` and `on_backward_pass()` hooks align with FSDP's shard-level execution.
- **Megatron-LM** — Hook into pipeline parallelism stage transitions for fine-grained phase control across pipeline stages.

Acceptance criteria:
- Integration must not add measurable latency to the critical path (< 1µs overhead per phase transition)
- Integration must degrade gracefully when the kernel module is not loaded (dry_run fallback)
- Integration must be configurable via environment variable (e.g., `MEM_HINT_ENABLED=1`)

## Phase 6: Hardware / FPGA Simulator

**Status:** Future research

Goal: Build a hardware simulation environment that demonstrates the full control-plane stack from userspace hint to PHY timing adjustment, with observable timing changes.

Approach:
1. Implement a minimal DDR5 PHY timing model on FPGA (focus on tRCD/tCL/tRP response to hint writes)
2. Connect via PCIe BAR to the MMIO dispatch path
3. Instrument timing measurements to validate that hint writes produce the expected PHY configuration changes within the 500µs pre-adjustment window
4. Publish measurement data as validation evidence for the patent's claimed performance improvements

This phase would provide the first hardware-measured evidence of the architecture's effectiveness.

## Timeline

| Phase | Target | Status |
|-------|--------|--------|
| Phase 1 | Q2 2026 | ✅ Complete |
| Phase 2 | Q3 2026 | 🔲 Planned |
| Phase 3 | Q4 2026 | 🔲 Research |
| Phase 4 | Q1 2027 | 🔲 Research |
| Phase 5 | Q2-Q3 2027 | 🔲 Planned |
| Phase 6 | Q4 2027+ | 🔲 Future |

## See Also

- [Architecture Overview](architecture.md)
- [Deployment Modes](deployment-modes.md)
- [Hardware Channels](hardware-channels.md)
- [CXL Fabric Embodiment](cxl-embodiment.md)
