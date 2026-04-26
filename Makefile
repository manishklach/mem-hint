# Patent Pending: Indian Patent Application No. 202641053160

PYTHON ?= python3

.PHONY: kernel userspace python-check validate clean

kernel:
	$(MAKE) -C kernel

userspace:
	$(MAKE) -C userspace/lib
	$(MAKE) -C userspace/examples

python-check:
	$(PYTHON) -m py_compile userspace/python/mem_hint/*.py userspace/python/mem_hint_cli.py
	@command -v flake8 >/dev/null 2>&1 && flake8 userspace/python || \
		echo "warning: flake8 not installed; skipping lint"

validate:
	bash scripts/validate_repo.sh

clean:
	$(MAKE) -C kernel clean || true
	$(MAKE) -C userspace/lib clean || true
	$(MAKE) -C userspace/examples clean || true
	@find userspace/python -name "__pycache__" -type d -prune -exec rm -rf {} +
	@find userspace/python -name "*.pyc" -delete
