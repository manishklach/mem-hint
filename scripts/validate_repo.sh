#!/usr/bin/env bash
# Patent Pending: Indian Patent Application No. 202641053160

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

echo "[validate] repo root: $repo_root"

required_files=(
  "README.md"
  "PATENT_NOTICE.md"
  "LICENSE"
  "CONTRIBUTING.md"
  ".github/workflows/build.yml"
  "kernel/Makefile"
  "kernel/mem_hint_main.c"
  "kernel/mem_hint.h"
  "kernel/mem_hint_pmu.c"
  "kernel/mem_hint_sysfs.c"
  "kernel/mem_hint_safety.c"
  "userspace/python/mem_hint/client.py"
  "userspace/python/mem_hint/phases.py"
  "userspace/python/mem_hint/scheduler.py"
  "userspace/python/mem_hint_cli.py"
  "docs/status.md"
  "docs/public-release-checklist.md"
)

echo "[validate] checking required files"
for path in "${required_files[@]}"; do
  if [[ ! -e "$path" ]]; then
    echo "error: missing required file: $path" >&2
    exit 1
  fi
done

echo "[validate] checking patent notice in kernel sources"
kernel_notice_files=(
  "kernel/mem_hint_main.c"
  "kernel/mem_hint.h"
  "kernel/mem_hint_pmu.c"
  "kernel/mem_hint_sysfs.c"
  "kernel/mem_hint_safety.c"
)
for path in "${kernel_notice_files[@]}"; do
  if ! grep -q "Patent Pending: Indian Patent Application No. 202641053160" "$path"; then
    echo "error: patent notice missing from $path" >&2
    exit 1
  fi
done

echo "[validate] python syntax"
mkdir -p userspace/python/__pycache__ userspace/python/mem_hint/__pycache__
python3 -m py_compile userspace/python/mem_hint/*.py userspace/python/mem_hint_cli.py

echo "[validate] flake8"
if command -v flake8 >/dev/null 2>&1; then
  flake8 userspace/python
else
  echo "warning: flake8 not installed; skipping lint"
fi

echo "[validate] kernel build"
kdir="${KDIR:-/lib/modules/$(uname -r)/build}"
if [[ -d "$kdir" ]]; then
  (
    cd kernel
    make KDIR="$kdir" clean >/dev/null
    make KDIR="$kdir"
  )
else
  echo "warning: kernel headers unavailable at $kdir; skipping kernel build"
fi

echo "[validate] complete"
