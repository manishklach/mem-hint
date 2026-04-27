# Phase Classification

Patent Pending: Indian Patent Application No. 202641053160.

Phase classification is the bridge between memory telemetry and semantic policy.
The explicit-hint path is always preferred when the runtime can declare phase
boundaries directly, but the PMU path exists so the same control plane can
operate with zero application changes. The classifier is intentionally simple in
the reference implementation because the point is to demonstrate the contract
and the threshold logic, not to overfit a single microarchitecture.

## Phase Table

| Phase        | ID     | Classification criteria                                          | Best-mode PHY configuration                                                                               |
| ------------ | ------ | ---------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------- |
| Prefill      | `0x01` | `write_bw > 10 GB/s` and `write_bw > read_bw`                    | `tRCD=22`, `tCL=22`, `tRP=22`, `tRAS=52`, `vswing=300`, `DFE Tap1=0x10`, `CTLE=2`                         |
| Decode       | `0x02` | `read_bw > write_bw * 2`, `write_bw < 1 GB/s`, `llc_miss > 5000` | `tRCD=18`, `tCL=18`, `tRP=18`, `tRAS=42`, `vswing=280`, `DFE Tap1=0x14`, `CTLE=3`                         |
| Agentic      | `0x03` | `bw_variance_pct > 50`                                           | `tRCD=20`, `tCL=20`, `tRP=20`, `tRAS=48`, `vswing=290`, `DFE Tap1=0x12`, `CTLE=2`                         |
| Idle         | `0x04` | `dram_cmd_rate < 1000/s`                                         | `tRCD=24`, `tCL=24`, `tRP=24`, `tRAS=56`, `vswing=240`, `DFE Tap1=0x08`, `CTLE=0`, reduced refresh policy |
| ForwardPass  | `0x05` | Explicit training hook at `forward()` start                      | Same profile as Prefill                                                                                   |
| BackwardPass | `0x06` | Explicit training hook at `loss.backward()` start                | `tRCD=18`, `tCL=18`, `tRP=18`, `tRAS=38`, `vswing=280`, `DFE Tap1=0x14`, `CTLE=3`                         |

These values correspond to the best-mode reference configurations discussed in
the patent’s phase-to-policy table. The implementation then scales away from
nominal according to priority before the safety limiter clamps the final result.

## PMU Counter Mapping

The reference classifier uses a narrow set of signals because the goal is not to
be clever, it is to be explainable:

- `UNC_M_CAS_COUNT.WR` from the IMC uncore represents sustained write pressure
and is the strongest Prefill cue.
- `UNC_M_CAS_COUNT.RD` from the IMC uncore represents sustained read pressure
and is the primary Decode cue when it dominates writes.
- `MEM_LOAD_RETIRED.L3_MISS` from the core PMU distinguishes light read traffic
from true KV-cache miss pressure.
- `OFFCORE_REQUESTS` or an equivalent derived stream helps compute burst
variance for Agentic mode.
- `UNC_M_CMD_RATE` from the IMC uncore provides the best simple signal for Idle
detection.

## Why 100 Microseconds

The polling interval is set to 100 microseconds because it is a compromise
between responsiveness and software overhead. Shorter windows become expensive
and noisy on general-purpose CPUs. Longer windows miss fast transitions,
especially around decode entry and queue drain. A 100-microsecond cadence is
short enough to catch semantic shifts before they become visible as severe tail
latency, while still being practical for a kernel timer and a small amount of
classification logic.

## Sliding-Window Trend Model

The simple classifier in the repository uses threshold checks and hysteresis,
but the design anticipates a sliding-window trend model layered on top:

1. Retain the last ten PMU samples.
2. Compute short-term directionality for reads, writes, misses, and variance.
3. Detect whether the next 3–5 samples are converging toward a known phase
signature.
4. Trigger a pre-adjustment before the target phase is fully dominant.

This trend model is the conceptual bridge between raw thresholding and
predictive adaptation. It is what makes the later 500-microsecond window
meaningful.

## The 500-Microsecond Predictive Pre-Adjustment Window

The 500-microsecond pre-adjustment window is computed as five 100-microsecond
classifier periods. That is long enough to smooth noise and short enough to
remain attached to a single phase transition rather than an average of unrelated
traffic. If the trend model indicates a likely transition, policy can start
relaxing or tightening selected knobs during that five-sample horizon instead of
waiting for the new phase to be fully visible in the controller.

## Phase Transition Matrix

| From \ To | Prefill                                       | Decode                                                 | Agentic                                          | Idle                                         | Forward                                     | Backward                  |
| --------- | --------------------------------------------- | ------------------------------------------------------ | ------------------------------------------------ | -------------------------------------------- | ------------------------------------------- | ------------------------- |
| Prefill   | stay if write-heavy prompt continues          | prompt completes, read pressure dominates              | tool or branch burst interrupts prompt flow      | queue drains unexpectedly                    | training launch substitutes explicit signal | not expected directly     |
| Decode    | speculative batch refill or new prompt starts | stay if read-heavy loop continues                      | tool call or planning burst begins               | request completes and command rate collapses | training workload replaces serving loop     | not expected directly     |
| Agentic   | tool output triggers fresh prompt ingestion   | tool return re-enters decode loop                      | stay if variance remains high                    | external waits drain activity                | explicit training handoff                   | explicit training handoff |
| Idle      | new request queue enters prefill              | rare, only if decode-like activity appears immediately | orchestration burst wakes system                 | stay                                         | explicit training start                     | not expected directly     |
| Forward   | next minibatch forward stays forward          | not expected in training path                          | control-plane burst may transiently look agentic | pause or scheduler stall                     | stay                                        | `loss.backward()` begins  |
| Backward  | not expected directly                         | not expected directly                                  | optimizer / orchestration burst                  | training stall or barrier wait               | next minibatch begins                       | stay                      |

## Hysteresis

Hysteresis matters because PMU data is noisy. Without it, a borderline workload
could oscillate between Decode and Agentic or between low-activity Decode and
Idle every polling interval. The reference logic handles this in the simplest
possible way: if none of the classification rules fire confidently, return the
previous phase. That single rule gives the PMU path temporal continuity and
stops thresholds from behaving like edge detectors.

## See Also

- [Architecture Overview](architecture.md)
- [Deployment Modes](deployment-modes.md)
- [Hardware Channels](hardware-channels.md)
- [CXL Fabric Embodiment](cxl-embodiment.md)
