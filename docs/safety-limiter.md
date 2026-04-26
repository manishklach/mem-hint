# Safety Limiter

Patent Pending: Indian Patent Application No. 202641053160.

The safety limiter exists because workload-aware memory control is powerful enough to become dangerous if left unconstrained. The easiest analogy is Rowhammer-era thinking: if software can push memory timing or signaling in pursuit of performance without an immutable guardrail, it can create a reliability and security attack surface. Thermal abuse, signal-integrity margin erosion, and error-amplifying latency cuts all live in the same category.

The patent's claims 9 and 10 distinguish policy logic from enforcement logic. Policy may be software-visible and tunable. Enforcement is not. In a real system, the clamp would be immutable ASIC combinational logic in the memory-controller or PHY path. This repository models that hardware-visible behavior in software because the goal is to demonstrate interface semantics, not to claim that a kernel module should be trusted as the final safety boundary.

## Hardware vs. Firmware

- Hardware safety logic is the non-bypassable authority.
- Firmware or driver software proposes an operating point.
- The clamp function applies JEDEC and telemetry bounds before anything reaches hardware.
- Software cannot override the real hardware implementation.

That distinction matters because "cannot be bypassed at any privilege level" means even ring 0 code, SMM, or a hypervisor cannot force an out-of-bounds operating point if the hardware limiter is correctly implemented. The kernel module in this repository is only the firmware-visible model of that logic.

## JEDEC Compliance

The reference bounds mirror DDR5-6400 style limits:

- `tRCD`: `14..32`
- `tCL`: `14..36`
- `tRP`: `14..32`
- `vswing_mv`: `200..400`

In shipping hardware, these bounds would be read from SPD EEPROM at boot and then merged with board-specific SI characterization. The module hardcodes them as defaults because the repository is meant to be buildable on generic Linux CI runners.

## ECC Feedback in the Clamp

The clamp integrates telemetry through `ecc.correctable_rate`. When the correctable error rate exceeds `1000000` per `10^8` accesses, the limiter relaxes `tRCD` and `tCL` by one step before allowing the request to proceed. That means the controller can back away from an aggressive setting without waiting for software to notice instability. This is the feedback-loop piece of the patent's reliability story.

## Security-hardened Mode

When `security_level > 0`, the reference model raises `tRCD` and `tCL` to at least `22`, which is treated as a nominal-safe floor for the illustrative platform. The idea is that sensitive or attested contexts can request a more conservative operating point even if performance policy would have gone lower.

Claims 17 and 18 are reflected as a TEE validation stub in the driver: a real implementation would verify that the hint originated from an attested runtime or trusted execution environment before honoring elevated `security_level` semantics. The repository logs a warning because no TEE integration is present here.
