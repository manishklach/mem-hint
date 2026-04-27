# Hardware Channels

Patent Pending: Indian Patent Application No. 202641053160.

The hardware channel layer is where a validated semantic hint becomes a privileged control event. The architectural point is not that every platform must use the same transport. The point is that the phase hint ABI should stay stable while the transport can vary according to controller placement, firmware ownership, and fabric topology. This separation is one of the central structural decisions in the architecture: it lets the same compact 8-byte hint packet travel through CPU-local registers, memory-mapped device windows, or fabric-level protocol extensions without changing the semantic contract that user space and the kernel driver agree on.

## Channel Selection: When to Use Which

Use the MSR path when the controller is tightly coupled to the CPU or SoC and the privileged register model is a natural fit. This is the simplest embodiment and maps cleanly to bring-up environments where a CPU-local control path is easiest to validate. It is also the most natural fit for ARM Neoverse platforms that expose implementation-defined system registers with analogous semantics to x86 model-specific registers.

Use the MMIO path when the memory controller or PHY is firmware-oriented and already exposes configuration windows through mapped registers. This is appropriate for discrete controllers, simulation models, and system integrations where the control plane already thinks in terms of mapped device registers. A Rambus PHY simulation model, for instance, would expose a configuration window that the driver can map and program through iowrite32 calls.

Use the CXL DVSEC path when the target memory region sits behind a fabric and policy needs to be expressed per device or per HDM region. This is the path that lets the same semantic hint ABI scale into attached or pooled memory systems without requiring topology-specific changes to the hint structure itself. The CXL path also opens up multi-region adaptation, where different fabric-attached memory regions can be steered independently based on congestion telemetry and workload semantics.

## MSR Path (Claim 4)

The MSR embodiment is the most direct and lowest-latency option. On x86, `wrmsrl(MEM_HINT_MSR, encoded_val)` moves the packed hint into a model-specific register in a single privileged instruction. This is appropriate for CPU-local control planes where a register write is the natural dispatch mechanism. On ARM Neoverse, an equivalent system register write achieves the same effect through the platform-specific register interface. The key architectural constraint is that MSR writes are CPU-local: they affect the memory controller closest to the issuing core, which is ideal for NUMA-local memory but requires additional propagation for cross-socket awareness.

## MMIO Path (Claim 5)

The MMIO embodiment fits discrete controllers or firmware-owned register blocks. The driver maps a controller-specific BAR region using `ioremap()` and writes the encoded hint via `iowrite32()`. This path is slightly higher latency than MSR due to PCIe or fabric traversal, but it provides direct access to the controller's configuration space regardless of whether the controller is CPU-integrated or discrete. In the reference implementation, the MMIO base must be explicitly mapped before use. If `mc_mmio_base` is NULL, the driver logs a debug message rather than attempting an unmapped write.

## CXL DVSEC Path (Claim 6)

The CXL DVSEC embodiment matters because modern memory systems are no longer strictly local. Claim 6 generalizes the hint transport into a fabric-aware path that can target HDM regions or device-specific policy registers without changing the semantic ABI. A DVSEC capability in the CXL configuration space identifies a vendor-specific control region. That region contains one or more policy registers scoped to a region, device, or link. The encoded hint is written into that region together with optional status or acknowledgment handling. The target memory region applies or forwards the policy internally. In the reference implementation, this path is a stub that logs the encoded value, because the repository does not claim ownership of a shipping vendor DVSEC layout.

## encode_hint() Bit Layout

The essential compression step packs the software-facing structure into a single 64-bit word:

```text
Bit  63        56 55      48 47    40 39      24 23       8 7     0
┌──────────┬──────────┬────────┬──────────┬──────────┬───────┐
│ reserved │ priority │security│    bw    │ latency  │phase  │
└──────────┴──────────┴────────┴──────────┴──────────┴───────┘
```

The phase_id occupies the lowest byte for quick extraction in hardware decode logic. Latency and bandwidth targets occupy 16-bit fields that are wide enough for practical value ranges. Security level and priority each consume one byte. The reserved field at the top allows future extensions without changing the lower-field layout. This packing means the entire semantic content of a workload hint can travel through any privileged conduit, whether MSR, MMIO, or CXL DVSEC, as a single atomic value that hardware can latch in one cycle.

## Channel Selection at Module Load

At module load, the driver selects a channel through the `active_channel` module parameter. A production implementation would also inspect platform capabilities and default to the best-supported path. In the reference implementation, the parameter makes the choice explicit and reviewable:

```bash
sudo insmod mem_hint.ko active_channel=0   # MSR
sudo insmod mem_hint.ko active_channel=1   # MMIO
sudo insmod mem_hint.ko active_channel=2   # CXL DVSEC
```

The driver intentionally uses illustrative register addresses and logs conduit activity rather than blindly writing to undefined hardware state. This is a reference implementation, not a claim of real hardware register ownership.

## See Also

- [Architecture Overview](architecture.md)
- [CXL Fabric Embodiment](cxl-embodiment.md)
- [Hardware Safety Limiter](safety-limiter.md)
- [Phase Classification](phase-classification.md)
- [Deployment Modes](deployment-modes.md)
