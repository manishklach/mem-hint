# Hardware Safety Limiter

Patent Pending: Indian Patent Application No. 202641053160.

The safety limiter is the part of the architecture that turns a clever systems
idea into a responsible one. The moment software can request more aggressive
memory behavior, the memory subsystem becomes a potential attack surface. That
risk is not theoretical. If timing and signaling margins are exposed without a
hard guardrail, a malicious or simply careless workload could ask for settings
that increase error rate, destabilize refresh behavior, or deliberately exploit
marginal cells and links.

The Rowhammer analogy is useful because it reminds us that software-visible
activity and physical memory behavior are often more tightly coupled than
architects expect. Rowhammer was not “a DRAM problem that software happened to
trigger.” It was a cross-layer problem in which benign-seeming software access
patterns created electrical consequences in memory. A workload-aware timing
interface needs to assume the same class of coupling from day one.

## Hardware Versus Firmware

This distinction is the key architectural point. The firmware or kernel-visible
model may contain a function called `safety_clamp()`, but that is not the
ultimate source of truth. In a production system, the true limiter must live in
immutable hardware logic. Software can propose. Hardware decides. The repository
models the limiter in software because the goal is to make the interface
reviewable and buildable, but the prose and code comments are intentionally
explicit that this is a model of the enforcement plane, not the enforcement
plane itself.

## JEDEC JESD79-5 Compliance In Practice

JESD79-5 matters because it defines the legal and characterized operating
envelopes for DDR5 timing and signaling. In practice, compliance means that
timing parameters such as `tRCD`, `tCL`, and `tRP` cannot be moved arbitrarily
just because a phase table says they should. It also means that voltage swing
and related electrical tuning live inside a bounded operating region derived
from the memory technology, controller implementation, board layout, and
validation data.

## SPD EEPROM Bounds

The immediate source of those bounds is the SPD EEPROM and associated platform
characterization. SPD provides the baseline timing envelope and module-specific
characteristics that firmware reads during boot and initialization. A shipping
safety limiter would consume those bounds early in platform bring-up and convert
them into hardware-enforced clamp ranges. The reference implementation hardcodes
representative DDR5 defaults because generic CI runners obviously do not expose
real SPD state.

## ECC Feedback Integration

The limiter does more than clamp static min and max values. It also listens to
ECC and retry telemetry. If the correctable error rate rises above the
configured threshold, the limiter relaxes aggressive parameters by stepping
timing back toward a safer region. That is essential because a static safe range
is only part of the story. A platform can remain nominally “within bounds” and
still show that the selected operating point is unwise under current thermal,
aging, or traffic conditions.

## Security-Hardened Mode

When `security_level > 0`, the limiter deliberately holds selected timing values
at or above a more conservative nominal floor. This is the software-visible
manifestation of a security-aware operating mode. The repository treats that
mode as a stronger safety posture for sensitive contexts. It does not try to
imply that user space can simply assert “secure” and receive special handling.
In a real deployment, that bit would only be meaningful when paired with a trust
signal.

## TEE Context Validation

That trust signal is where TEE context validation comes in. The patent’s
security-oriented claims contemplate a system in which sensitive contexts can
request a hardened policy only when they are backed by attestation, trusted
runtime state, or equivalent evidence. The reference driver leaves this as an
explicit stub because there is no honest way to fake a TEE integration in a
generic reference repository. The correct behavior is to acknowledge the
requirement and keep the hook visible.

## Claim 9 And Claim 10 In Plain Language

Claim 9, in plain language, is the statement that workload-aware policy must
still be subordinate to a hardware-enforced safe operating range. Claim 10
extends that logic by making the limiter authoritative regardless of software
privilege. These claims matter because they prevent the interface from
collapsing into “kernel code that can overclock DRAM.” The invention is a
disciplined control plane, not a bypass.

## Why “Cannot Be Bypassed At Any Privilege Level” Matters

That phrase is meaningful because it shifts the trust boundary out of the OS. If
ring 0, a hypervisor, or firmware can bypass the limiter, then the limiter is
not a safety mechanism. It is only a convention. The architecture described here
treats non-bypassability as a defining requirement: the kernel may participate
in the model, but it must not be the final authority.

## See Also

- [Architecture Overview](architecture.md)
- [Hardware Channels](hardware-channels.md)
- [CXL Fabric Embodiment](cxl-embodiment.md)
- [Deployment Modes](deployment-modes.md)
