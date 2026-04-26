# Phase Classification

Patent Pending: Indian Patent Application No. 202641053160.

The PMU subsystem models the patent's telemetry-driven phase inference described in section 10.1 `[0031]` and section 10.2. It samples every `100 us` and maps memory behavior into one of six phase identifiers:

| Phase | ID | Classification or trigger |
| --- | --- | --- |
| Prefill | `0x01` | `write_bw > 10 GB/s` and `write_bw > read_bw` |
| Decode | `0x02` | `read_bw > write_bw * 2`, `write_bw < 1 GB/s`, `llc_miss > 5000` |
| Agentic | `0x03` | `bw_variance_pct > 50` |
| Idle | `0x04` | `dram_cmd_rate < 1000 cmds/s` |
| Forward Pass | `0x05` | Explicit runtime signal for training forward pass |
| Backward Pass | `0x06` | Explicit runtime signal for training backward pass |

If none of the PMU conditions match, the classifier returns the previous phase. That hysteresis prevents oscillation across short, noisy intervals and is essential to the predictive transition model.

## PMU Counter Mapping

| Field | Counter mapping | Notes |
| --- | --- | --- |
| `write_bw_gbps` | `UNC_M_CAS_COUNT.WR` | IMC uncore write CAS count converted to GB/s |
| `read_bw_gbps` | `UNC_M_CAS_COUNT.RD` | IMC uncore read CAS count converted to GB/s |
| `llc_miss_rate` | `MEM_LOAD_RETIRED.L3_MISS` | Core PMU read-path miss pressure |
| `bw_variance_pct` | Sliding 10-sample window | Computed variance of the last `1 ms` |
| `dram_cmd_rate` | `UNC_M_CMD_RATE` | Aggregate command issue rate from IMC |

## Why `100 us` Polling

`100 us` is short enough to detect inference and training phase shifts before they dominate end-to-end latency, but long enough that software collection overhead remains practical. It also creates a convenient ten-sample window per millisecond, which makes the variance-based `Agentic` heuristic stable without requiring a large ring buffer.

## Sliding-Window Trend Model

The reference driver only classifies current phase, but the design anticipates a predictive model over a sliding ten-sample window:

1. Record the last ten PMU samples.
2. Compute trend slope for read bandwidth, write bandwidth, miss pressure, and variance.
3. Detect rising decode signatures before the decode state is fully dominant.
4. Schedule a pre-adjustment if the next phase is likely within the prediction horizon.

## Why a `500 us` Predictive Pre-adjustment Window

`500 us` is a practical control window because it spans multiple `100 us` samples while still being well inside the burst envelope of inference and training transitions. It is large enough to amortize PHY adaptation latency yet small enough to stay specific to the imminent phase change. In other words, it gives the control plane time to act without turning the policy into a slow average.

## Phase Transition Matrix

| From | To | Primary trigger |
| --- | --- | --- |
| Idle | Prefill | Request queue becomes non-empty; write bandwidth rises above threshold |
| Prefill | Decode | Prompt ingestion ends; read pressure dominates with low writes |
| Decode | Agentic | Tool-use or branch-heavy orchestration causes variance spike |
| Agentic | Decode | Variance settles; read-heavy low-write token loop resumes |
| Decode | Idle | Queue drains and command rate collapses |
| Forward | Backward | `loss.backward()` or equivalent training boundary |
| Backward | Forward | Next microbatch forward launch |

## Best-mode PHY Configurations

These best-mode values match the reference patent table used by the driver:

| Phase | `tRCD` | `tCL` | `tRP` | `tRAS` | `vswing_mv` | `dfe_tap1` | `ctle_gain_db` |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Prefill | 22 | 22 | 22 | 52 | 300 | `0x10` | 2 |
| Decode | 18 | 18 | 18 | 42 | 280 | `0x14` | 3 |
| Agentic | 20 | 20 | 20 | 48 | 290 | `0x12` | 2 |
| Idle | 24 | 24 | 24 | 56 | 240 | `0x08` | 0 |
| Forward Pass | 22 | 22 | 22 | 52 | 300 | `0x10` | 2 |
| Backward Pass | 18 | 18 | 18 | 38 | 280 | `0x14` | 3 |
