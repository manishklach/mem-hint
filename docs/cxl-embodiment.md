# CXL Fabric Embodiment

Patent Pending: Indian Patent Application No. 202641053160.

Claims 15 and 16 are easiest to understand as the “memory is no longer local” extension of the architecture. In plain language, the invention is not limited to tuning a single DDR channel next to a CPU. It also applies when semantic workload intent must travel across a coherent fabric and influence multiple memory regions with different latency, bandwidth, and electrical characteristics.

## Per-HDM-Region Independent PHY Adaptation

Once memory is regionized, one policy no longer fits all. A host might have local DDR5 as Region 0, a CXL Type-2 attached device as Region 1, and a high-bandwidth local HBM pool as Region 2. Decode-sensitive traffic may want aggressive local timing, while spill or cold-state traffic can tolerate a relaxed remote region. The embodiment therefore assumes region-local adaptation rather than a single global knob.

## Congestion Response Policy

The congestion response logic in a CXL embodiment is not simply “slow everything down.” A sensible policy is more nuanced:

1. Observe link utilization, queue occupancy, retries, and region pressure.
2. If fabric congestion rises, relax the remote region that is contributing least to user-visible latency.
3. Preserve tighter local DDR or HBM behavior for the decode-critical path if telemetry still supports it.
4. If congestion remains high, shift additional traffic classes toward a safer or less latency-sensitive region.
5. Feed the result back into the next policy decision rather than treating CXL as an opaque side channel.

That logic is what turns the fabric into part of the policy loop instead of a passive transport.

## NUMA IPI Hint Propagation

Claims 38 and 39 matter when the system spans multiple sockets or region owners. A phase transition on one node may need to influence memory behavior on another. The clean way to do that is to propagate a compact hint through a NUMA-aware signaling path, for example with an IPI or mailbox-like event that tells a peer node to re-evaluate its local region policy.

The point is not that every implementation must literally use IPIs. The point is that the semantic hint can be propagated as a control event rather than reconstructed from fabric symptoms alone.

## Multi-GPU Training Synchronization

Claims 31 through 33 become relevant when host memory policy and accelerator synchronization interact. In a multi-GPU training job, backward propagation is often accompanied by optimizer traffic, checkpointing, and collective communication. A fabric-aware embodiment can coordinate those phases by letting the host memory control plane understand that a bandwidth-heavy or write-heavy synchronization epoch is underway and adjust region policy accordingly.

```text
┌─────────────────────────────────────┐
│           Host CPU / SoC            │
│      Memory Policy Engine           │
│         CXL Root Complex            │
└──────────────┬──────────────────────┘
               │ CXL 2.0/3.0
      ┌────────▼─────────┐
      │   CXL Fabric     │
      │     Switch       │
      │ congestion telemetry
      └──┬─────────┬─────┘
┌────────▼───┐  ┌──▼────────────┐  ┌─▼──────┐
│ DDR5 DIMMs │  │ CXL Type-2    │  │  HBM3  │
│ Region 0   │  │ Region 1      │  │Region 2│
│ tRCD=18    │  │ tRCD=22       │  │ stack  │
│ (tight)    │  │ (relaxed)     │  │  PHY   │
└────────────┘  └───────────────┘  └────────┘
```

The topology diagram above is intentionally simple, but it captures the essential idea: different memory regions can respond differently to the same high-level hint.

## See Also

- [Architecture Overview](architecture.md)
- [Hardware Channels](hardware-channels.md)
- [Phase Classification](phase-classification.md)
- [Deployment Modes](deployment-modes.md)
