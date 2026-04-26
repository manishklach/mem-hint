# Status

Patent Pending: Indian Patent Application No. 202641053160.

This repository is a reference implementation. The goal is to make the interface contract reviewable and buildable without overstating real hardware support.

| Component | Status | Notes |
| --- | --- | --- |
| `/dev/mem_hint` character device | Implemented | End-to-end write path exists in the kernel module |
| Shared userspace headers | Implemented | C-facing interface mirrors the 8-byte hint contract |
| Python client | Implemented | Direct device and sysfs access available |
| Python scheduler hooks | Implemented | Reference integrations for inference and training stacks |
| PMU classifier | Simulated | Telemetry collection is a stub, not real PMU wiring |
| sysfs policy/status surface | Implemented | Useful for demos, testing, and API review |
| MSR hardware path | Illustrative | Disabled by default; not real hardware programming |
| MMIO hardware path | Illustrative | No real controller mapping or register contract |
| CXL DVSEC path | Illustrative | Logging stub only |
| Safety limiter | Software model | Models a hardware concept but does not replace hardware enforcement |
| Real silicon support | Not implemented | No vendor-specific enablement or production validation |

## Candid Notes

- The kernel module builds and the interface is coherent.
- The repository is intentionally conservative about claiming hardware behavior.
- The right way to read this project is as a reference control-plane proposal with working software scaffolding, not as a device enablement package.
