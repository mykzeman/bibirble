# System requirements benchmark: PR #12 (Story Mode)

- **PR:** #12 "Add Story Mode", `claude/story-mode-impl` → `claude/upbeat-archimedes-kguhel`
- **Head measured:** `bc02691`. **Baseline:** `0ede19f` (the PR's parent commit)
- **Date:** 2026-11-19
- **Host:** Ubuntu 24.04.5 LTS (x86_64), 4 vCPU, 16 GB RAM, g++ 13.3.0, CMake 3.28.3,
  wxWidgets 3.2.4 (GTK3, `libwxgtk3.2-dev`), `nlohmann-json3-dev`. GUI runs used Xvfb at 1280x800x24.

## Requirements check (the README's "Requirements" section, run locally)

| Requirement (README) | Result |
|---|---|
| `libwxgtk3.2-dev` | Installed (3.2.4). Configure and link succeed |
| `nlohmann-json3-dev` | Installed. Configure succeeds |
| `cmake build-essential` (CMake 3.16+, C++17) | CMake 3.28 and g++ 13 work |
| Release build (`cmake -DCMAKE_BUILD_TYPE=Release`) | Builds with **0 warnings, 0 errors** |
| `ctest --test-dir build` (new `story_tests`) | **1/1 passed** (0.16 s) |
| `python tools/check_story_map.py` | **No errors found** |
| App launches (Linux, Xvfb) | Starts, loads data and stays up; no stderr errors |

The PR adds **no new third-party dependencies**. The new `story_tests` target needs only
nlohmann/json, not wxWidgets.

## Build cost (Release, `-j1`, clean build)

| Metric | Baseline | PR #12 | Change |
|---|---|---|---|
| `cmake --build` default (all targets), wall time | 25.6 s | 59.6 s | **+34.0 s (+133%)** |
| App target only (`--target Bibirble`), wall time | 25.6 s | 41.5 s | +15.9 s (+62%) |
| Peak compiler memory (largest single process) | 440 MB | 446 MB | +6 MB |
| Build directory on disk | 21 MB | 42 MB | +21 MB (mostly the `story_tests` binary) |

**Note:** `story_tests` is part of the default `all` target, so a plain `./build.sh` /
`cmake --build build` now compiles the test binary too. That takes about 18 s of the extra
time and adds a 9.6 MB binary. If release builds should skip it, put it behind an option
such as `BIBIRBLE_BUILD_TESTS` or mark it `EXCLUDE_FROM_ALL`.

## Binary footprint

| Metric | Baseline | PR #12 | Change |
|---|---|---|---|
| `Bibirble` file size | 9.83 MB | 10.28 MB | +446 KB (+4.5%) |
| `.text` (includes the embedded JSON blobs) | 9,585,832 B | 9,958,828 B | +373 KB |
| Embedded `story_map.json` | — | 40.7 KB | new |
| Shared libraries linked (`ldd` count) | — | 85 | wx core and base, unchanged |

## Runtime (Linux/Xvfb, 3 runs each, start screen idle)

| Metric | Baseline | PR #12 | Change |
|---|---|---|---|
| Resident memory after loading (median) | 171.3 MB | 172.2 MB | **+0.9 MB (+0.5%)** |
| Time until memory levels off (load time) | 1.71 s | 1.71 s | no measurable change |
| CPU time while starting (user + sys) | ~0.46 s | ~0.45 s | within noise |
| Threads | 5 | 5 | same |
| `story_tests` run | — | 0.16 s, 50 MB peak | new |

The first head run took 2.67 s to load, but the 3 repeat runs were all 1.71 s, so that
first number looks like an outlier.

## Recommended minimum system requirements (unchanged by this PR)

- **OS:** Windows 10+, or Linux with GTK3 and wxWidgets 3.2. macOS still needs
  `bible_sections.json` next to the app, and now `story_map.json` too (no embedded copy yet)
- **RAM:** at least 256 MB free for the game (it uses about 172 MB). Building from source
  needs at least 1 GB per parallel compile job (about 450 MB peak per job)
- **Disk:** about 11 MB for the binary. A source build needs about 45 MB, plus the 9 MB data file
- **CPU:** any x86_64 CPU. A clean single-core build takes about 1 minute

## Scope and limits

- The Story Mode screens were not clicked through automatically. Runtime numbers are for
  start-up and the idle start screen. Story Mode logic is covered by `story_tests`.
- Windows (`build.ps1`/MinGW) and macOS builds were not run.
