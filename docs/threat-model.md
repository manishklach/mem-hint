# Threat Model

Patent Pending: Indian Patent Application No. 202641053160.

This document describes the security threat model for the `/dev/mem_hint`
interface and explains the layered defenses that prevent workload hints from
causing memory corruption, data loss, or system instability.

## 1. Malicious Userspace Hint Spam

**Threat:** A compromised or malicious userspace process sends a high-frequency
stream of contradictory phase hints to `/dev/mem_hint`, attempting to
destabilize memory timing or cause excessive PHY reconfiguration.

**Mitigations:**

- **CAP_SYS_ADMIN requirement.** The character device requires `CAP_SYS_ADMIN`
  to open. Unprivileged processes cannot send hints at all.
- **Safety clamp.** Every hint passes through `safety_clamp()` before reaching
  the hardware dispatch path. Even if a privileged process sends pathological
  timing requests, the clamp enforces JEDEC SPD bounds on all parameters (tRCD,
  tCL, tRP, vswing).
- **Rate limiting (roadmap).** A production implementation should add rate
  limiting on hint submission — for example, no more than 1000 hints per second
  per process. The reference implementation does not enforce this but the
  architecture supports it.

## 2. Privilege Boundary: CAP_SYS_ADMIN

**Design decision:** The `/dev/mem_hint` interface requires `CAP_SYS_ADMIN`
because it influences memory subsystem behavior. This places it in the same
trust category as other privileged memory management interfaces (e.g.,
`/proc/sys/vm/`, `/dev/mem`).

**Limitations:**

- `CAP_SYS_ADMIN` is a broad capability. A more granular capability (e.g., a
  custom `CAP_MEM_HINT`) would be preferable in a production kernel integration.
- Container environments that grant `CAP_SYS_ADMIN` should be aware that this
  also grants access to `/dev/mem_hint` if the module is loaded.
- A future iteration could use a more restrictive access control mechanism, such
  as a dedicated security module or cgroup-based filtering.

## 3. Unsafe PHY Tuning Risk

**Threat:** A workload hint requests timing parameters that push the memory
subsystem beyond safe operating margins, causing bit errors, data corruption, or
DIMM damage.

**Mitigations:**

- **JEDEC SPD bounds.** The `safety_clamp()` function enforces hard limits
  derived from JEDEC JESD79-5 specifications and the DIMM's SPD EEPROM data. No
  timing parameter can exceed the manufacturer-specified operating range.
- **ECC feedback integration.** If the correctable error rate exceeds 10⁻⁶
  (stored as errors per 10⁸ accesses with a threshold of 100), the clamp
  automatically relaxes timing by 1 clock cycle. This provides closed-loop
  protection against emerging reliability issues.
- **Security-hardened mode.** When `security_level > 0` (e.g., in TEE or
  confidential computing contexts), all timing parameters are held at or above
  JEDEC nominal values (tRCD ≥ 22, tCL ≥ 22, tRP ≥ 22). This prevents any
  aggressive tuning in security-sensitive workloads.

## 4. Why the Safety Clamp Exists

The safety clamp (patent claims 9 and 10) is the architectural answer to a
fundamental tension: software has better information about workload semantics,
but hardware has better information about physical constraints. The clamp allows
software to _propose_ timing configurations, but the hardware safety limiter has
final authority.

In the reference implementation, this is modeled in software
(`mem_hint_safety.c`). In production hardware, the clamp would be implemented as
immutable combinational logic within the memory controller ASIC, anchored to the
platform hardware root of trust. No software update — including a kernel module
replacement — could bypass the hardware implementation.

## 5. Why Real Hardware Needs an Immutable Limiter

Software-only safety is insufficient for memory timing control because:

1. **Kernel compromise.** If the kernel is compromised, a software-only limiter
   can be patched out. An ASIC-level limiter cannot be modified by any software
   agent, including the kernel.
2. **Firmware bugs.** Firmware updates that accidentally relax timing bounds
   would be caught by the hardware limiter before reaching the PHY.
3. **Supply chain.** A hardware limiter provides a root-of-trust anchor that is
   verifiable during manufacturing and not dependent on correct software
   deployment.
4. **Regulatory compliance.** Memory safety standards may require demonstrable
   hardware enforcement that cannot be circumvented by software, similar to how
   automotive safety systems require hardware watchdogs.

## 6. Side-Channel Concerns from Phase Signaling

**Threat:** The phase signaling interface leaks information about workload
behavior. An observer with access to sysfs status attributes (`current_phase`,
`p99_latency_ns`) or timing side-channels could infer what type of AI workload
is running (e.g., inference vs. training, prefill vs. decode).

**Assessment:**

- **sysfs access.** The status attributes are readable by any user with access
  to the platform device sysfs path. A production deployment should restrict
  sysfs permissions to root or a dedicated monitoring group.
- **Timing side-channels.** Changes in memory timing (tRCD, tCL) are observable
  through memory access latency measurements from any process on the same NUMA
  node. This is a fundamental property of shared memory hardware and is not
  unique to `/dev/mem_hint` — any memory configuration change (e.g., BIOS-level
  XMP profiles) creates the same observable effect.
- **Phase frequency.** The rate of phase transitions is observable and could
  reveal workload structure (e.g., prefill/decode alternation patterns in
  serving workloads). This is analogous to existing CPU frequency scaling
  side-channels.

**Mitigations:**

- Restrict sysfs permissions:
  `chmod 0440 /sys/bus/platform/drivers/mem_hint/status/*`
- Consider jittering phase transition timing in production implementations
- In confidential computing environments, use `security_level > 0` to prevent
  aggressive timing changes that would create larger observable differences

## 7. Mitigation Roadmap

| Priority | Mitigation                             | Status                      |
| -------- | -------------------------------------- | --------------------------- |
| P0       | JEDEC SPD hard clamp                   | ✅ Implemented              |
| P0       | CAP_SYS_ADMIN gate                     | ✅ Implemented              |
| P0       | ECC feedback integration               | ✅ Implemented              |
| P0       | Security-hardened mode                 | ✅ Implemented              |
| P1       | Hardware MSR write disabled by default | ✅ Implemented              |
| P1       | Selftest validation at module load     | ✅ Implemented              |
| P2       | Per-process hint rate limiting         | 🔲 Roadmap                  |
| P2       | Granular capability (CAP_MEM_HINT)     | 🔲 Roadmap                  |
| P2       | sysfs permission hardening             | 🔲 Roadmap                  |
| P3       | Phase transition jitter                | 🔲 Research                 |
| P3       | Hardware root-of-trust integration     | 🔲 Requires silicon partner |

## See Also

- [Architecture Overview](architecture.md)
- [Safety Limiter](safety-limiter.md)
- [Deployment Modes](deployment-modes.md)
- [SECURITY.md](../SECURITY.md)
