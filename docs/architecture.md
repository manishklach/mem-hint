# Architecture

Patent Pending: Indian Patent Application No. 202641053160.

`/dev/mem_hint` is a six-layer reference stack for communicating AI workload intent into the memory signaling plane. The architecture models the control-path described by the patent application: a user-space runtime or orchestration layer emits workload phase information, the Linux kernel module validates and classifies that intent, an illustrative privileged channel carries the encoded command, and an immutable safety model ensures that no workload request can violate platform bounds. This aligns with the core independent concept and with the safety-oriented framing of claims 9 and 10.

The six layers are:

1. User/runtime layer: vLLM, TensorRT-LLM, PyTorch FSDP, Megatron, or custom agent runtimes decide when a workload enters prefill, decode, agentic, idle, forward, or backward phases.
2. Device contract layer: userspace writes an 8-byte `mem_workload_hint` packet to `/dev/mem_hint`, or reads and tunes state through sysfs.
3. Linux kernel module layer: the LKM owns validation, PMU polling, encoding, notification, and policy lookups.
4. Privilege channel layer: encoded hints travel over one of three illustrative hardware paths, `MSR`, `MMIO`, or `CXL DVSEC`, as described in claims covering control-plane transport embodiments.
5. Policy engine and safety layer: workload policy selects an intended PHY mode while the safety limiter clamps every request against JEDEC and platform telemetry constraints.
6. PHY and memory-device layer: real hardware would adjust DDR5 signaling, timing, equalization, CXL-attached memory regions, or fabric policy.

There are three entry points into the system:

- Explicit hint path: a privileged process writes a `mem_workload_hint` struct directly to `/dev/mem_hint`. This is the most accurate path because the runtime already knows phase boundaries.
- PMU auto-classification path: the driver samples PMU telemetry every `100 us`, classifies the active phase, and emits a synthesized hint if the phase changed. This corresponds to the telemetry-driven control loop described in patent section 10.
- sysfs control path: operators and experiments tune thresholds and per-phase policy fields under `/sys/bus/platform/drivers/mem_hint/`, enabling live refinement without changing runtime code.

The feedback loop is intentionally closed. `ecc_correctable_rate`, `ecc_uncorrectable_count`, `read_retry_count`, and `p99_latency_ns` are exposed through status telemetry, and the safety limiter consumes ECC pressure to relax aggressive timing when error rate rises. That models the patent's claims around adaptive but bounded signaling control: workload intent can influence the platform, but measured reliability can override it.

```text
                         +--------------------------------------+
                         |      AI Runtime / Training Stack     |
                         | vLLM | TRT-LLM | FSDP | Megatron     |
                         +-------------------+------------------+
                                             |
                   explicit hint write       | phase-aware hooks
                                             v
+------------------------------+    +----------------------------+
| /dev/mem_hint char device    |    | sysfs policy + thresholds  |
| 8-byte workload hint packet  |    | live operator tuning       |
+---------------+--------------+    +--------------+-------------+
                |                                  |
                +----------------+-----------------+
                                 v
                    +------------------------------+
                    | Linux Kernel Module          |
                    | write path | PMU classifier  |
                    | status export | notifications|
                    +---------------+--------------+
                                    |
                                    v
                    +------------------------------+
                    | Policy Engine + Safety Clamp |
                    | phase lookup | JEDEC bounds  |
                    | ECC feedback | security floor|
                    +---------------+--------------+
                                    |
           +------------------------+------------------------+
           |                        |                        |
           v                        v                        v
    +-------------+          +-------------+          +-------------+
    | MSR Channel |          | MMIO Path   |          | CXL DVSEC   |
    | local socket|          | MC register |          | fabric ctrl |
    +------+------+          +------+------+          +------+------+
           |                        |                        |
           +------------------------+------------------------+
                                    v
                    +------------------------------+
                    | PHY / Memory Devices         |
                    | DDR5 timing | equalization   |
                    | CXL memory | region policy   |
                    +------------------------------+
```

The hardware channel abstraction intentionally supports three embodiments:

- `MSR`: best for local, tightly coupled CPU-memory controller coordination on development systems and early silicon.
- `MMIO`: appropriate when a firmware-owned memory controller exposes command registers rather than model-specific registers.
- `CXL DVSEC`: the fabric-oriented embodiment used when signaling must cross a coherent memory fabric, consistent with the claims around external memory devices and per-region adaptation.

The reference implementation is deliberately illustrative. `MEM_HINT_MSR` is not a real architectural register, MMIO mapping is stubbed, and the CXL path logs instead of touching real DVSEC registers. The important thing is the contract: phase intent is encoded, privilege is enforced, policy is applied, and safety remains authoritative.
