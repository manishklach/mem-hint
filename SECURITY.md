# Security Policy

Patent Pending: Indian Patent Application No. 202641053160.

## Scope

This repository contains an **illustrative reference implementation** of `/dev/mem_hint`, a workload-aware memory signaling interface for AI computing systems. It is published for research, educational, and interoperability discussion purposes.

## Important Warnings

1. **Do not load kernel modules from this repository on production systems.** The kernel code is a reference implementation with illustrative register addresses. It has not been validated against real memory-controller hardware and could cause undefined behavior if loaded on systems with conflicting MSR or MMIO mappings.

2. **Hardware register writes are disabled by default.** The `wrmsrl()` call in the MSR dispatch path is gated behind `#if MEM_HINT_REAL_HW`, which defaults to `0`. This means no actual hardware registers are written unless a developer explicitly enables this flag and recompiles the module. The MMIO and CXL DVSEC paths are stubs that log rather than write.

3. **The safety limiter is a software model, not a hardware guarantee.** In production, the safety limiter described in patent claims 9 and 10 would be implemented as immutable combinational logic within the memory controller ASIC. The software implementation in `mem_hint_safety.c` models this behavior but cannot provide the same bypass-resistance as a hardware implementation.

4. **CAP_SYS_ADMIN is required.** The character device `/dev/mem_hint` requires `CAP_SYS_ADMIN` to open. This is a deliberate privilege boundary, but it is not sufficient on its own for production security — see `docs/threat-model.md` for discussion.

## Reporting Security Issues

If you discover a security vulnerability in this reference implementation, please report it responsibly by contacting:

**Email:** mlachwani@gmail.com

Please include:
- A description of the vulnerability
- Steps to reproduce
- Your assessment of the impact
- Any suggested fix

We will acknowledge receipt within 48 hours and provide a timeline for resolution.

## Out of Scope

The following are **not** security issues for this repository:
- The fact that illustrative MSR/MMIO addresses do not correspond to real hardware
- The fact that the safety limiter is implemented in software rather than hardware
- Performance characteristics that differ from the projected simulation numbers
- The kernel module failing to load on systems without matching kernel headers

These are documented design decisions, not vulnerabilities.
