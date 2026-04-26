# Patent Pending: Indian Patent Application No. 202641053160

PYTHON ?= python3

.PHONY: kernel userspace python-check validate clean

kernel:
	cd kernel && $(MAKE)

userspace:
	$(MAKE) -C userspace/lib
	$(MAKE) -C userspace/examples

python-check:
	@mkdir -p userspace/python/__pycache__ userspace/python/mem_hint/__pycache__
	$(PYTHON) -m py_compile userspace/python/mem_hint/*.py userspace/python/mem_hint_cli.py
	@command -v flake8 >/dev/null 2>&1 && flake8 userspace/python || \
		echo "warning: flake8 not installed; skipping lint"

validate:
	bash scripts/validate_repo.sh

clean:
	cd kernel && $(MAKE) clean || true
	$(MAKE) -C userspace/lib clean || true
	$(MAKE) -C userspace/examples clean || true
	@rm -f Module.symvers modules.order .Module.symvers.cmd .modules.order.cmd
	@find userspace/python -name "__pycache__" -type d -prune -exec rm -rf {} +
	@find userspace/python -name "*.pyc" -delete
