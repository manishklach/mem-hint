# Public Release Checklist

Patent Pending: Indian Patent Application No. 202641053160.

## Build and Validation

- [ ] `scripts/validate_repo.sh` passes all checks
- [ ] GitHub Actions CI passes (kernel build + Python lint + tests)
- [ ] `python examples/demo_sequence.py` runs in dry-run mode without errors
- [ ] `flake8 userspace/python/` exits cleanly

## Content and Accuracy

- [ ] README simulation disclaimers reviewed — no overclaims about real hardware
- [ ] "Projected Simulation Results" section clearly labeled as non-hardware-validated
- [ ] All register addresses clearly marked as illustrative
- [ ] Kernel build messaging is honest about illustrative hardware paths
- [ ] `MEM_HINT_REAL_HW` defaults to 0 (hardware writes disabled)

## Patent and Legal

- [ ] Patent notice text and application metadata (202641053160) are correct everywhere
- [ ] All kernel .c/.h files contain the patent notice header
- [ ] License and research-use terms are reviewed
- [ ] PATENT_NOTICE.md is up to date

## Security and Privacy

- [ ] No private information, machine-local paths (like `~`), or secrets in tracked files
- [ ] `__pycache__`, `.egg-info`, and other generated artifacts are not tracked
- [ ] SECURITY.md is present and accurate
- [ ] Contact email in SECURITY.md is correct

## Documentation

- [ ] All docs/ files exist and contain substantial prose (>50 lines each)
- [ ] docs/threat-model.md covers all identified risks
- [ ] docs/research-roadmap.md reflects current state accurately
- [ ] docs/demo-output.md shows realistic example output
- [ ] README links to docs, PATENT_NOTICE.md, and CONTRIBUTING.md all resolve

## GitHub Infrastructure

- [ ] GitHub Pages is enabled, serving from gh-pages branch
- [ ] Blog post canonical link resolves correctly
- [ ] Site landing page loads with no broken links
- [ ] Issue templates are present (.github/ISSUE_TEMPLATE/)
- [ ] Pull request template is present (.github/pull_request_template.md)
- [ ] v0.1.0 tag exists and points to correct commit
- [ ] Release is published on GitHub with accurate description

## Final Review

- [ ] Read through README as if seeing the repo for the first time
- [ ] Click every link in README and verify it resolves
- [ ] Run `demo_sequence.py` one more time and verify output matches docs/demo-output.md
- [ ] Share with a trusted reviewer for feedback before wider distribution
