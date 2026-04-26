# CXL Embodiment

Patent Pending: Indian Patent Application No. 202641053160.

Claims 15 and 16 are captured here as the fabric-attached memory embodiment of `/dev/mem_hint`. Instead of a single local DDR channel receiving the control signal, the hint can be interpreted per CXL HDM region so that different memory pools react differently to the same workload phase.

## Per-region Adaptation

Each HDM region can maintain an independent policy state:

- Local DDR5 near the CPU may tighten latency for decode.
- CXL-attached pooled memory may relax signaling or scheduling to preserve margin.
- A scratch or spill region may prioritize bandwidth during forward or backward phases.

That separation matters because the memory system is no longer monolithic. The fabric introduces its own latency, congestion, and arbitration behavior.

## Congestion Telemetry Integration

The CXL embodiment extends the feedback loop with fabric counters:

- link utilization
- retry/NACK pressure
- HDM region occupancy
- switch queue depth

The high-level congestion response policy is:

- Relax CXL-side aggressiveness when fabric congestion rises.
- Tighten local DDR5 for latency-sensitive decode paths if local headroom still exists.
- Keep the global behavior NUMA-aware instead of assuming all bytes are equal.

## NUMA-aware Policy and IPI Propagation

Claims 38 and 39 map naturally onto multi-socket or region-aware systems. A runtime on one NUMA node may emit a hint that needs to be propagated to a peer node or to a fabric manager. A production implementation could:

1. Tag the hint with node or region affinity.
2. Broadcast a lightweight IPI or mailbox event to sibling sockets.
3. Recompute local-vs-remote adaptation independently.

## Multi-GPU Training and NVLink

Claims 31 through 33 are relevant to multi-GPU training stacks. Backward phases often flood host memory and communication paths with gradient-related traffic. A practical policy might:

- tighten local DDR5 write-oriented timing during backward pass,
- relax CXL-pooled spill regions to protect margin,
- and coordinate with NVLink or NVSwitch schedule windows so host memory adaptation does not fight collective traffic.

```text
          +------------------+       +------------------+
          | GPU 0 / NVLink   |-------| GPU 1 / NVLink   |
          +--------+---------+       +--------+---------+
                   |                          |
                   +------------+-------------+
                                |
                        +-------v--------+
                        | CPU / LKM      |
                        | /dev/mem_hint  |
                        +-------+--------+
                                |
                +---------------+----------------+
                |                                |
         +------v------+                  +------v------+
         | Local DDR5  |                  | CXL Switch  |
         | low-latency |                  | congestion  |
         +------+------+\                 +------+------+
                |       \                        |
                |        \                +------v------+
                |         \---------------| CXL Memory  |
                |                          | HDM Region  |
                |                          +-------------+
```
