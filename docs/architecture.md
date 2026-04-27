# Architecture Overview

Patent Pending: Indian Patent Application No. 202641053160.

The memory wall in AI systems is no longer just a bandwidth story. Large-model
serving spends one part of its life shoveling prompt state and another part
waiting on tail latency inside decode loops. Training jobs alternate between
forward passes that look like bandwidth-centric streaming and backward passes
that suddenly become write-heavy and congestion-sensitive. The software stack
knows exactly when those shifts happen. The hardware stack usually does not.

That mismatch is the architectural reason for `/dev/mem_hint`. Existing
controllers, PHYs, and attached fabrics already expose meaningful policy knobs,
but the path from runtime semantics to privileged hardware action is fractured.
The patent frames this as a cross-layer contract problem: the software has
intent, the hardware has capability, and the missing element is a
kernel-mediated interface that can validate, encode, route, and bound
memory-policy hints before they touch real timing or signaling state.

```text
+--------------------------------------------------------------------------------------------------+
| Layer 6: AI Runtime / Training Stack                                                             |
| vLLM | TensorRT-LLM | PyTorch FSDP | Megatron | custom agents                                    |
|                                 | explicit phase boundary (sub-microsecond software event)       |
+---------------------------------+----------------------------------------------------------------+
                                  | 8-byte semantic hint packet
                                  v
+--------------------------------------------------------------------------------------------------+
| Layer 5: /dev/mem_hint userspace contract                                                        |
| C library | Python client | scheduler hooks | examples                                            |
|                                 | write()/ioctl-like intent path (software latency)              |
+---------------------------------+----------------------------------------------------------------+
                                  v
+--------------------------------------------------------------------------------------------------+
| Layer 4: Linux kernel module                                                                     |
| char device write path | PMU auto-classifier | sysfs policy/status                               |
|                    | validation / copy_from_user / PMU sample at 100us cadence                   |
+--------------------+-----------------------------------------------------------------------------+
                     v
+--------------------------------------------------------------------------------------------------+
| Layer 3: Privilege channel                                                                       |
| MSR | MMIO | CXL DVSEC                                                                           |
|         | one privileged dispatch step toward local or fabric-attached memory control            |
+---------+----------------------------------------------------------------------------------------+
          v
+--------------------------------------------------------------------------------------------------+
| Layer 2: Policy engine + safety limiter                                                          |
| phase table | threshold tuning | ECC-aware clamp | security floor                                |
|      | predictive pre-adjustment (500us window) | bounded by Claims 9 and 10                    |
+------+-------------------------------------------------------------------------------------------+
       v
+--------------------------------------------------------------------------------------------------+
| Layer 1: PHY / memory devices                                                                    |
| DDR5 timing knobs | signaling swing | equalization | CXL memory regions                          |
|                           | realized as latency / bandwidth / retry behavior                     |
+--------------------------------------------------------------------------------------------------+
```

## Layer 1: PHY And Memory Devices

The lowest layer contains the actual electrical and timing domain. That includes
DDR5 and MRDIMM timings, equalization settings, refresh mode, voltage swing, and
the region-level behavior of attached CXL memory. The patent’s Claim 1 matters
here because it establishes the target of the interface: the point is not to
make software “aware” in the abstract, but to steer actual memory-system policy
in a phase-aware way.

## Layer 2: Policy Engine And Safety Limiter

The policy engine turns a phase hint into a proposed operating point. It is
where the lookup table lives, where phase-specific defaults are selected, and
where operator tuning enters through sysfs. Claims 9 and 10 then constrain what
this layer can do: even a perfectly valid software hint cannot overrule the hard
safety envelope. The feedback signals from ECC rate, read retry count, and
latency telemetry all land here before any hardware-facing decision is
finalized.

## Layer 3: Privilege Channel

The privilege channel is the conduit by which a validated policy decision
becomes a hardware event. Claim 4 covers an MSR-like embodiment, Claim 5 covers
an MMIO embodiment, and Claim 6 extends the concept into CXL DVSEC-style fabric
signaling. The architectural value of this separation is that the hint ABI stays
stable while the physical transport can vary by platform generation.

## Layer 4: Linux Kernel Module

The kernel is the broker and guardrail. It receives explicit hints, hosts the
PMU classifier, exposes sysfs tuning points, and ensures that only privileged
software can request phase transitions. It is also where interface stability is
enforced. The Linux module is not pretending to be the PHY or the controller
firmware; it is the control-plane host that mediates between software semantics
and privileged hardware paths.

## Layer 5: Userspace Contract

This layer exists to keep runtimes honest and simple. The userspace side does
not need to know DRAM internals. It only needs a stable representation of
semantic phases: prefill, decode, agentic, idle, forward, backward. The C
library, Python client, CLI, and scheduler wrappers all exist to keep the
contract narrow enough that runtimes can adopt it without learning
memory-controller details.

## Layer 6: AI Runtime And Training Stack

The runtime layer is where semantic knowledge exists first. A serving engine
knows when prefill starts. A scheduler knows when the queue empties. A training
framework knows exactly when a backward pass begins. That is why explicit hints
are the highest-quality input path. The PMU path exists for compatibility, but
the architecture is fundamentally built around preserving the semantic knowledge
software already has.

## Three Entry Points

### Explicit Hint Path

The explicit path is the cleanest expression of Claim 1. A privileged process
writes an 8-byte semantic hint into `/dev/mem_hint`, the kernel validates it,
encodes it, and passes it into the selected conduit. This mode is best when the
runtime already exposes precise phase boundaries and when the deployment owner
wants maximum accuracy.

### PMU Auto-Classification Path

The PMU path exists because not every runtime can be patched. The kernel samples
IMC and core PMU counters every 100 microseconds, classifies a likely phase, and
then feeds the result back through the same policy engine. This is
architecturally important because it means the rest of the stack does not care
where the phase originated. The classifier is a source, not a separate
subsystem.

### sysfs Policy Path

The sysfs path is for operators, validation, and field tuning. It does not
replace explicit hints or PMU classification. Instead, it lets an operator
adjust thresholds, policy floors, and observability endpoints while the same
underlying control plane continues to run. In practice this is the path that
turns the repository from a code artifact into a usable lab reference.

## Three Hardware Channels

### MSR

The MSR embodiment is appropriate for CPU-local control planes where a
privileged register write is the natural dispatch mechanism. This is the most
direct interpretation of Claim 4 and the easiest to reason about in early
prototypes.

### MMIO

The MMIO embodiment fits discrete controllers or firmware-owned register blocks.
It maps well to controller ASICs, simulation platforms, and PHY IP that already
expose configuration windows to platform firmware. This is the Claim 5 path.

### CXL DVSEC

The CXL DVSEC embodiment matters because modern memory systems are no longer
strictly local. Claim 6 generalizes the hint transport into a fabric-aware path
that can target HDM regions or device-specific policy registers without changing
the semantic ABI.

## Feedback Loop

The control loop proceeds in a strict order:

1. A phase hint arrives explicitly, or is inferred by PMU classification, or is
influenced by sysfs tuning.
2. The kernel validates the source, clamps the hint fields into the interface
contract, and encodes the semantic packet.
3. The policy engine maps the phase to a proposed timing, signaling, and
equalization profile.
4. The safety limiter clamps the proposal against JEDEC and platform-specific
limits.
5. The selected privilege channel dispatches the encoded hint toward the target
hardware path.
6. ECC, retry, and latency telemetry flow back into the status path and
influence the next control decision.

This feedback structure is what makes the repository more than a
character-device demo. It shows how Claims 1, 4, 5, 6, 9, and 10 fit together as
a coherent control-plane architecture.

## See Also

- [Phase Classification](phase-classification.md)
- [Hardware Safety Limiter](safety-limiter.md)
- [Deployment Modes](deployment-modes.md)
- [Hardware Channels](hardware-channels.md)
- [CXL Fabric Embodiment](cxl-embodiment.md)
