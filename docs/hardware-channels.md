# Hardware Channels

Patent Pending: Indian Patent Application No. 202641053160.

The repository models three hardware delivery paths for encoded workload hints: `MSR`, `MMIO`, and `CXL DVSEC`. Each path carries the same 64-bit contract but targets a different class of platform embodiment.

## When to Use Each

- `MSR`: use for local CPU-memory-controller prototypes or early silicon where privileged software can access a model-specific register path.
- `MMIO`: use when the memory controller exposes firmware-managed control registers rather than an MSR interface.
- `CXL DVSEC`: use when the target memory region sits behind a CXL fabric and policy must be carried over a device-specific extended capability structure.

## Platform Detection and Selection

Channel selection is controlled by the module parameter `active_channel`:

```bash
sudo insmod mem_hint.ko active_channel=0  # MSR
sudo insmod mem_hint.ko active_channel=1  # MMIO
sudo insmod mem_hint.ko active_channel=2  # CXL DVSEC
```

In a production implementation, module init would probe CPU vendor IDs, memory-controller capabilities, ACPI/PCIe enumerations, and CXL DVSEC support before selecting a default. The reference driver defaults to `MSR`.

## `encode_hint()` Layout

```text
 63                    56 55          48 47          40 39          24 23          8 7          0
+------------------------+--------------+--------------+--------------+--------------+-------------+
| reserved[0]            | priority     | security     | bw_target    | latency_ns   | phase_id    |
+------------------------+--------------+--------------+--------------+--------------+-------------+
```

The repository keeps the encoded command narrow enough to fit a single 64-bit write while preserving all fields of the eight-byte workload struct.

## CXL DVSEC Structure Overview

The CXL path is intentionally a stub, but the modeled structure is:

1. Locate the device-specific extended capability for the target memory device.
2. Identify a vendor-defined register block for workload-hint ingestion.
3. Write the encoded hint into the region or port-scoped control register.
4. Optionally expose per-region acknowledgment and fault counters.

That is consistent with the patent's external-memory embodiment while remaining clearly non-normative about any real vendor register map.
