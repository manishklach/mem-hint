#!/usr/bin/env bash
# validate_repo.sh — Repository validation for mem-hint
# Patent Pending: Indian Patent Application No. 202641053160

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

PASS=0
FAIL=0

check() {
  local desc="$1"
  shift
  if "$@" >/dev/null 2>&1; then
    echo "[PASS] $desc"
    PASS=$((PASS + 1))
  else
    echo "[FAIL] $desc"
    FAIL=$((FAIL + 1))
  fi
}

echo "============================================="
echo "  mem-hint repository validation"
echo "  $(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo "============================================="
echo ""

# ── Required files ─────────────────────────────────────────────
echo "--- Required Files ---"
required_files=(
  "README.md"
  "PATENT_NOTICE.md"
  "LICENSE"
  "CONTRIBUTING.md"
  "SECURITY.md"
  "CHANGELOG.md"
  ".github/workflows/build.yml"
  "kernel/Makefile"
  "kernel/mem_hint.c"
  "kernel/mem_hint.h"
  "kernel/mem_hint_pmu.c"
  "kernel/mem_hint_sysfs.c"
  "kernel/mem_hint_safety.c"
  "userspace/include/mem_hint.h"
  "userspace/lib/mem_hint.c"
  "userspace/lib/Makefile"
  "userspace/examples/basic_hint.c"
  "userspace/examples/phase_monitor.c"
  "userspace/python/setup.py"
  "userspace/python/mem_hint/__init__.py"
  "userspace/python/mem_hint/client.py"
  "userspace/python/mem_hint/phases.py"
  "userspace/python/mem_hint/scheduler.py"
  "userspace/python/examples/demo_sequence.py"
  "docs/architecture.md"
  "docs/phase-classification.md"
  "docs/safety-limiter.md"
  "docs/deployment-modes.md"
  "docs/hardware-channels.md"
  "docs/cxl-embodiment.md"
  "docs/threat-model.md"
  "docs/research-roadmap.md"
  "docs/demo-output.md"
  "docs/public-release-checklist.md"
  "site/index.html"
  "site/writings/dev-mem-hint-kernel-control-plane-ai-memory-systems.html"
)

for path in "${required_files[@]}"; do
  check "exists: $path" test -e "$path"
done

# ── Patent notice in kernel sources ────────────────────────────
echo ""
echo "--- Patent Notice ---"
kernel_sources=(
  "kernel/mem_hint.c"
  "kernel/mem_hint.h"
  "kernel/mem_hint_pmu.c"
  "kernel/mem_hint_sysfs.c"
  "kernel/mem_hint_safety.c"
)
for path in "${kernel_sources[@]}"; do
  check "patent notice in $path" \
    grep -q "Patent Pending: Indian Patent Application No. 202641053160" "$path"
done

# ── Python compilation ─────────────────────────────────────────
echo ""
echo "--- Python Compilation ---"
if command -v python3 >/dev/null 2>&1; then
  for pyfile in userspace/python/mem_hint/*.py; do
    check "compiles: $pyfile" python3 -m py_compile "$pyfile"
  done
  check "compiles: demo_sequence.py" \
    python3 -m py_compile userspace/python/examples/demo_sequence.py
else
  echo "[SKIP] python3 not found"
fi

# ── flake8 lint ────────────────────────────────────────────────
echo ""
echo "--- Flake8 Lint ---"
if command -v python3 >/dev/null 2>&1 && python3 -m flake8 --version >/dev/null 2>&1; then
  check "flake8 clean" python3 -m flake8 userspace/python/ \
    --max-line-length=100 \
    --exclude=__pycache__,.eggs,*.egg-info \
    --ignore=E501,W503
else
  echo "[SKIP] flake8 not available"
fi

# ── Shell scripts executable ───────────────────────────────────
echo ""
echo "--- Shell Script Checks ---"
for sh in scripts/*.sh; do
  if [ -e "$sh" ]; then
    check "executable: $sh" test -x "$sh"
  fi
done

# ── No secrets or local paths ──────────────────────────────────
echo ""
echo "--- Secret / Path Scan ---"
check "no /home/ paths in tracked files" \
  bash -c '! git grep -l "/home/" -- "*.py" "*.c" "*.h" "*.md" "*.yml" 2>/dev/null'
check "no /Users/ paths in tracked files" \
  bash -c '! git grep -l "/Users/" -- "*.py" "*.c" "*.h" "*.md" "*.yml" 2>/dev/null'
check "no AWS keys in tracked files" \
  bash -c '! git grep -l "AKIA" -- "*.py" "*.c" "*.h" "*.md" "*.yml" 2>/dev/null'

# ── README content checks ─────────────────────────────────────
echo ""
echo "--- README Content ---"
check "README links to PATENT_NOTICE.md" grep -q "PATENT_NOTICE.md" README.md
check "README has dry_run section" grep -q "dry_run" README.md
check "README has simulation disclaimer" grep -q "simulation" README.md
check "README has citing section" grep -q "Citing" README.md

# ── Kernel build (optional) ───────────────────────────────────
echo ""
echo "--- Kernel Build ---"
kdir="${KDIR:-/lib/modules/$(uname -r)/build}"
if [ -d "$kdir" ]; then
  check "kernel module builds" \
    bash -c "cd kernel && make KDIR='$kdir' clean >/dev/null 2>&1; make KDIR='$kdir' 2>&1"
else
  echo "[SKIP] kernel headers not found at $kdir"
fi

# ── Summary ────────────────────────────────────────────────────
echo ""
echo "============================================="
echo "  Results: $PASS passed, $FAIL failed"
echo "============================================="

if [ "$FAIL" -gt 0 ]; then
  exit 1
fi
