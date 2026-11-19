# System requirements benchmark: PR #13 (PR #12 benchmark doc)

- **PR:** #13 "Add system requirements benchmark for PR #12 (Story Mode)", `claude/dazzling-curie-qwqnei` → `claude/story-mode-impl`
- **Head measured:** `952ab65`. **Baseline:** `bc02691` (the PR's base)
- **Date:** 2026-11-19
- **Host:** Ubuntu 24.04 (x86_64), 4 vCPU, 16 GB RAM, g++ 13.3.0, CMake 3.28.3,
  wxWidgets 3.2.4 (GTK3), nlohmann-json 3.11.3. GUI run used Xvfb at 1280x800x24.

## What the PR changes

One file: `docs/benchmarks/PR12_SYSTEM_REQUIREMENTS.md` (+73 lines). No source, build,
data or dependency changes.

## Requirements check (README "Requirements", Ubuntu/Linux, run locally)

| Requirement | Result |
|---|---|
| `apt install libwxgtk3.2-dev` | Installed 3.2.4. Configure and link succeed |
| `apt install nlohmann-json3-dev` | Installed 3.11.3. Configure succeeds |
| `apt install cmake build-essential` | CMake 3.28.3, g++ 13.3.0 work |
| Release build | **0 warnings, 0 errors** |
| `ctest` | **1/1 passed** (0.14 s) |
| `python tools/check_story_map.py` | **No errors found** |
| App launch (Xvfb) | Up after 6 s, no stderr output, RSS 167 MB |

## Build and footprint (Release, `-j1`, clean)

| Metric | Baseline | PR #13 | Change |
|---|---|---|---|
| Default build wall time | 53.2 s | 54.1 s | within noise |
| Peak compiler memory | 457 MB | 457 MB | none |
| `Bibirble` size | 10,276,712 B | 10,276,712 B | none |
| `Bibirble` SHA-256 | `e73c4b1b…eece` | `e73c4b1b…eece` | **byte-identical** |
| `story_tests` size | 9,647,472 B | 9,647,472 B | none |

## Conclusion

PR #13 has **no effect** on system requirements: the built binaries are byte-identical.
The minimum requirements in `PR12_SYSTEM_REQUIREMENTS.md` still apply. Windows and macOS
builds were not run.
