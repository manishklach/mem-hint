## Description

Brief description of what this PR does.

## Type of Change

- [ ] Bug fix (non-breaking change that fixes an issue)
- [ ] New feature (non-breaking change that adds functionality)
- [ ] Documentation update
- [ ] Kernel module change (**requires careful review**)
- [ ] CI/build system change

## Checklist

- [ ] I have read [CONTRIBUTING.md](CONTRIBUTING.md)
- [ ] The patent notice (Indian Patent Application No. 202641053160) remains intact in all kernel source files
- [ ] No overclaims about real hardware support — all hardware paths are clearly marked as illustrative unless hardware validation has been performed
- [ ] I have not added personal paths, secrets, or machine-specific configuration
- [ ] Python changes pass `flake8` and the dry-run test suite
- [ ] Kernel changes compile with `make -C kernel` against recent kernel headers

## Kernel Change Review (if applicable)

If this PR modifies files in `kernel/`:
- [ ] safety_clamp() behavior is preserved or strengthened
- [ ] The selftest vectors still pass
- [ ] `MEM_HINT_REAL_HW` gating is not removed
- [ ] No new `wrmsrl()`/`iowrite32()` calls without proper gating

## Testing

Describe how you tested your changes:
- [ ] `python examples/demo_sequence.py` runs successfully
- [ ] GitHub Actions CI passes
- [ ] (If kernel change) Module compiles against target kernel headers
