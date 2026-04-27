# Hardware Channels

Patent Pending: Indian Patent Application No. 202641053160.

The hardware channel layer is where a validated semantic hint becomes a privileged control event. The architectural point is not that every platform must use the same transport. The point is that the phase hint ABI should stay stable while the transport can vary according to controller placement, firmware ownership, and fabric topology.

## When To Use MSR, MMIO, Or CXL DVSEC

Use the MSR path when the controller is tightly coupled to the CPU or SoC and the privileged register model is a natural fit. This is the simplest embodiment and maps cleanly to bring-up environments where a CPU-local control path is easiest to validate.

Use the MMIO path when the memory controller or PHY is firmware-oriented and already exposes configuration windows through mapped registers. This is appropriate for discrete controllers, simulation models, and system integrations where the control plane already thinks in terms of mapped device registers.

Use the CXL DVSEC path when the target memory region sits behind a fabric and policy needs to be expressed per device or per HDM region. This is the path that lets the same semantic hint ABI scale into attached or pooled memory systems.

## Platform Detection And Channel Selection

At module load, the driver can select a channel through the `active_channel` module parameter. A production implementation would also inspect platform capabilities and default to the best-supported path. In the reference implementation, the parameter makes the choice explicit and reviewable:

```bash
sudo insmod mem_hint.ko active_channel=0   # MSR
sudo insmod mem_hint.ko active_channel=1   # MMIO
sudo insmod mem_hint.ko active_channel=2   # CXL DVSEC
```

The driver intentionally suppresses illustrative hardware writes unless explicitly enabled, because the repository models the contract and does not claim real hardware register ownership.

## encode_hint() Bit Layout

```text
Bit 63         48 47      40 39    24 23     8 7      0
┌──────────────┬──────────┬────────┬────────┬────────┐
│   reserved   │ priority │security│   bw   │latency │ phase│
└──────────────┴──────────┴────────┴────────┴────────┘
```

This is the essential compression step. The software-facing structure carries semantic fields. The hardware-facing path receives a single packed word that can be moved through whatever privileged conduit the platform supports.

## CXL DVSEC Register Layout Overview

The reference repository does not pretend to own a real CXL DVSEC layout, but the architectural shape is clear:

1. A DVSEC capability identifies a vendor-specific control region.
2. That region contains one or more policy registers scoped to a region, device, or link.
3. The encoded hint is written into that region together with optional status or acknowledgment handling.
4. The target memory region applies or forwards the policy internally.

That is enough to explain the embodiment without falsely claiming a shipping vendor map.

## See Also

- [Architecture Overview](architecture.md)
- [CXL Fabric Embodiment](cxl-embodiment.md)
- [Hardware Safety Limiter](safety-limiter.md)
- [Phase Classification](phase-classification.md)
