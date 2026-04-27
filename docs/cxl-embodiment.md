# CXL Fabric Embodiment

Patent Pending: Indian Patent Application No. 202641053160.

Claims 15 and 16 are easiest to understand as the "memory is no longer local"
extension of the architecture. In plain language, the invention is not limited
to tuning a single DDR channel next to a CPU. It also applies when semantic
workload intent must travel across a coherent fabric and influence multiple
memory regions with different latency, bandwidth, and electrical
characteristics. The CXL embodiment is architecturally significant because it
demonstrates that the hint ABI can scale beyond a single controller and a single
memory technology into a heterogeneous, fabric-attached topology where each
region has independent policy requirements.

## Per-HDM-Region Independent PHY Adaptation

Once memory is regionized, one policy no longer fits all. A host might have
local DDR5 as Region 0, a CXL Type-2 attached device as Region 1, and a
high-bandwidth local HBM pool as Region 2. Decode-sensitive traffic may want
aggressive local timing, while spill or cold-state traffic can tolerate a
relaxed remote region. The embodiment therefore assumes region-local adaptation
rather than a single global knob. Each region independently receives the encoded
hint and applies or ignores it according to its local policy engine and safety
constraints. This is what makes the CXL path more than just another transport:
it implies a distributed policy model where hint propagation and local
enforcement are separate concerns.

## Congestion Response Policy

The congestion response logic in a CXL embodiment is not simply "slow everything
down." A sensible policy is more nuanced and takes into account the relative
importance of each memory region to the current workload phase:

1. Observe link utilization, queue occupancy, retries, and region pressure
across the fabric.
2. If fabric congestion rises above 80% utilization on a given link, relax the
remote CXL region that is contributing least to user-visible latency. This means
widening timing margins and reducing signaling aggressiveness on the congested
link.
3. Simultaneously, preserve tighter local DDR5 or HBM timing behavior for the
decode-critical path, because local memory is not contributing to fabric
congestion and remains the primary latency-sensitive resource.
4. If congestion remains high despite relaxing one region, shift additional
traffic classes toward a safer or less latency-sensitive region. This may
involve moving cold KV cache pages to the relaxed CXL region while keeping hot
decode state local.
5. Feed the congestion response result back into the next policy decision rather
than treating CXL as an opaque side channel. This closes the loop and ensures
that the policy engine learns from fabric conditions over time rather than
reacting purely from local telemetry.

That logic is what turns the fabric into part of the policy loop instead of a
passive transport. The congestion response interacts with the safety limiter at
each region independently: even if the policy engine decides to tighten one
region and relax another, both regions independently enforce their JEDEC bounds
and ECC-driven constraints.

## NUMA IPI Hint Propagation

Claims 38 and 39 matter when the system spans multiple sockets or region owners.
A phase transition on one node may need to influence memory behavior on another.
The clean way to do that is to propagate a compact hint through a NUMA-aware
signaling path, for example with an IPI (inter-processor interrupt) or
mailbox-like event that tells a peer node to re-evaluate its local region
policy. In practice this means that when the primary node transitions from
decode to prefill, it can notify the remote node to begin relaxing its
CXL-attached memory timing in anticipation of bandwidth-heavy traffic arriving
through the fabric.

The point is not that every implementation must literally use IPIs. The point is
that the semantic hint can be propagated as a control event rather than
reconstructed from fabric symptoms alone. Reconstructing phase intent from
link-level telemetry would be too slow and too noisy to be useful. Direct
propagation preserves the semantic fidelity of the original phase classification
while adding only a small fixed-latency overhead for the IPI or mailbox
delivery.

## Multi-GPU Training Synchronisation

Claims 31 through 33 become relevant when host memory policy and accelerator
synchronization interact. In a multi-GPU training job, backward propagation is
often accompanied by optimizer traffic, checkpointing, and collective
communication over NVLink or PCIe. A fabric-aware embodiment can coordinate
those phases by letting the host memory control plane understand that a
bandwidth-heavy or write-heavy synchronization epoch is underway and adjust
region policy accordingly.

For example, during an all-reduce operation across 8 GPUs, the host memory may
be handling gradient accumulation buffers and checkpoint staging. The backward
phase hint tells the host controller to optimize for write bandwidth. When the
all-reduce completes and the optimizer step begins, the hint can transition to a
balanced profile that prioritizes neither reads nor writes but minimizes queue
congestion for the mixed traffic pattern that optimizer and checkpoint
operations create.

The GPU-side IPC stub mentioned in the training hook example is the integration
point where host-side phase hints could be mirrored to GPU-side firmware through
shared-memory doorbells or similar mechanisms. This is left as a stub in the
reference implementation because honest GPU firmware integration requires
vendor-specific knowledge that a reference repository cannot claim.

## CXL Topology Diagram

```text
┌─────────────────────────────────────┐
│         Host CPU / SoC              │
│      Memory Policy Engine           │
│         CXL Root Complex            │
└──────────────┬──────────────────────┘
               │ CXL 2.0/3.0
      ┌────────▼──────────┐
      │   CXL Fabric      │ ◄── congestion telemetry
      │     Switch         │
      └──┬──────────┬─────┘
┌────────▼───┐  ┌───▼────────────┐  ┌──▼─────────┐
│ DDR5 DIMMs │  │  CXL Type-2    │  │    HBM3    │
│  Region 0  │  │   Region 1     │  │  Region 2  │
│  tRCD=18   │  │   tRCD=22      │  │   stack    │
│  (tight)   │  │   (relaxed)    │  │    PHY     │
└────────────┘  └────────────────┘  └────────────┘
```

The topology diagram above illustrates the essential idea: different memory
regions can respond differently to the same high-level hint. Region 0 runs tight
timing because it is the local, low-latency path for decode-critical data.
Region 1 runs relaxed timing because it is behind the fabric and serves as a
capacity tier. Region 2 is an HBM stack with its own PHY characteristics. All
three regions receive the same encoded hint, but their local policy engines and
safety limiters produce different PHY configurations appropriate to their
technology and link characteristics.

## See Also

- [Architecture Overview](architecture.md)
- [Hardware Channels](hardware-channels.md)
- [Phase Classification](phase-classification.md)
- [Deployment Modes](deployment-modes.md)
- [Hardware Safety Limiter](safety-limiter.md)
