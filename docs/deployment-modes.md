# Deployment Modes

Patent Pending: Indian Patent Application No. 202641053160.

The repository supports three deployment modes because AI infrastructure teams
adopt control planes in stages. Some own the runtime and can add explicit hooks
immediately. Others need a zero-application-change path to validate the idea
first. Still others only want live operator controls while they study how a
future runtime integration should behave.

## Comparison

| Mode                    | Accuracy | App changes | Best for                                                                       |
| ----------------------- | -------- | ----------- | ------------------------------------------------------------------------------ |
| Explicit runtime hints  | Highest  | Yes         | Teams that own serving or training code and can mark phase boundaries directly |
| PMU auto-classification | Medium   | No          | Operators validating the idea on unmodified runtimes                           |
| sysfs policy tuning     | Indirect | No          | Lab testing, operator experimentation, threshold tuning                        |

## Mode 1: Explicit Runtime Hints

This is the intended end state. The runtime knows when prefill begins, when
decode takes over, when an agent starts a tool call, and when a training
framework enters backward propagation. Those events map directly to the hint
ABI.

The practical steps are straightforward. First, build and load the kernel
module. Second, install the userspace package or link the C helper library.
Third, add a hook at each important phase boundary. Finally, observe the status
path to confirm that phase transitions appear when expected.

Performance expectations are best in this mode because the signal comes from
semantic ground truth rather than inference. If the runtime knows it entered
decode, the controller does not need to guess from read/write ratios or miss
counters.

## Mode 2: PMU Auto-Classification

This mode exists for situations where application code cannot be changed yet.
The kernel samples PMU data every 100 microseconds, applies the phase
classifier, and synthesizes the same workload hint structure the explicit path
would have carried.

The steps are to load the module, confirm the PMU polling path is active, run
the workload unchanged, and then monitor the status tree to see whether phase
transitions align with expected behavior. If thresholds are too eager or too
conservative, tune them through sysfs and rerun the workload.

Performance expectations here are lower than explicit hints because the
classifier operates from symptoms. It is still valuable because it demonstrates
the policy engine and safety loop without requiring application integration
work.

## Mode 3: sysfs Policy Tuning

sysfs tuning is the operator control plane. It is not a replacement for the
other two modes. Instead, it lets the operator adjust thresholds and per-phase
defaults while explicit hints or PMU classification continue feeding the system.

The tuning workflow usually looks like this:

```bash
cat /sys/bus/platform/drivers/mem_hint/status/current_phase
cat /sys/bus/platform/drivers/mem_hint/status/p99_latency_ns
echo 12 > /sys/bus/platform/drivers/mem_hint/thresholds/prefill_write_bw_gbps
echo 1 > /sys/bus/platform/drivers/mem_hint/thresholds/decode_write_bw_ceiling
echo 290 > /sys/bus/platform/drivers/mem_hint/policy/decode_vswing_mv
echo 0 > /sys/bus/platform/drivers/mem_hint/policy/idle_pll_reduction
```

Performance expectations in this mode depend entirely on what is driving phase
selection. sysfs on its own is a tuning surface, not a phase source.

## How To Verify The Driver Is Working

Verification is intentionally simple:

1. Read `status/current_phase`.
2. Read `status/active_channel`.
3. Check `last_transition_ms` while a workload is active.
4. Observe whether `p99_latency_ns` and ECC counters are populated or at least
   readable.

If explicit hints are wired correctly, `current_phase` should move in lockstep
with runtime transitions. If PMU mode is active, phase changes should still
appear, but they will lag slightly behind semantic boundaries.

## How Explicit And PMU Interact

The priority order is explicit hint first, PMU inference second, operator tuning
third. In other words:

1. A privileged explicit hint is authoritative for the current phase.
2. PMU classification provides a fallback or background signal when explicit
   hints are absent.
3. sysfs changes alter thresholds and policy parameters, but they do not
   themselves claim the system entered a phase.

This ordering preserves semantic intent when it is available and still gives
operators and unmodified systems a useful path when it is not.

## See Also

- [Architecture Overview](architecture.md)
- [Phase Classification](phase-classification.md)
- [Hardware Channels](hardware-channels.md)
- [Hardware Safety Limiter](safety-limiter.md)
