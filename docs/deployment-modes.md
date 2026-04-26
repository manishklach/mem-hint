# Deployment Modes

Patent Pending: Indian Patent Application No. 202641053160.

The reference implementation supports three deployment modes so teams can adopt the interface at different levels of integration.

| Mode | Accuracy | Effort | Best for |
| --- | --- | --- | --- |
| Explicit runtime hints | Highest | Medium | Runtime owners who know exact phase boundaries |
| PMU auto-classification | Medium | Low | Operators who cannot patch the runtime yet |
| sysfs tuning only | Indirect | Low | Lab validation and threshold exploration |

## 1. Explicit Runtime Hints

Step by step:

1. Build and load the kernel module.
2. Install the userspace Python package or C helper library.
3. Add hooks at runtime boundaries such as `generate()`, token decode steps, `forward()`, and `loss.backward()`.
4. Emit the corresponding phase ID through `/dev/mem_hint`.

This mode should be preferred because explicit intent outranks inferred intent. If a runtime knows it is entering decode, that signal is more authoritative than a PMU heuristic.

## 2. PMU Auto-classification

Step by step:

1. Load the module with PMU polling enabled.
2. Observe `status/current_phase` while the workload runs.
3. Tune thresholds if the default classifier is too eager or too conservative.

This mode infers phase from measured behavior. It is especially useful when integrating with third-party binaries or when bootstrapping policy on a platform where runtime hooks do not exist yet.

## 3. sysfs Tuning Workflow

Step by step:

1. Read current defaults.
2. Adjust threshold or policy values.
3. Re-run the workload.
4. Inspect status counters and latency.

Example shell commands:

```bash
cat /sys/bus/platform/drivers/mem_hint/status/current_phase
cat /sys/bus/platform/drivers/mem_hint/status/p99_latency_ns
echo 12 > /sys/bus/platform/drivers/mem_hint/thresholds/prefill_write_bw_gbps
echo 290 > /sys/bus/platform/drivers/mem_hint/policy/decode_vswing_mv
echo 5 > /sys/bus/platform/drivers/mem_hint/policy/agentic_priority
```

## Interaction Rules

Explicit hints take priority over PMU-derived hints. The auto-classifier exists as a fallback or as a background suggestion mechanism, but it should not override a recent, privileged runtime write that clearly asserts current phase.

## Verification

The fastest verification path is:

```bash
cat /sys/bus/platform/drivers/mem_hint/status/current_phase
cat /sys/bus/platform/drivers/mem_hint/status/active_channel
```

If phase changes are occurring, `last_transition_ms` should reset close to zero when a transition happens and then increase over time.
