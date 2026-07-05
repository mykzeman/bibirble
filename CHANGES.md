Story Mode:

Summary:
- Added a separate "Story Mode" reachable from the start screen: 10 curated sections following the
  Bible's narrative arc, each broken into chapters of 10 verses played with the existing guess-the-reference
  mechanic (same GameState/GameRow/reveal-panel/keyboard UI, just fed a curated verse sequence instead of a
  single random/daily verse). Section 1 (Creation, Genesis 1-11) is fully authored with 6 chapters plus a
  bonus "prophecy" chapter that unlocks once every chapter in the section has been aced (won outright, no
  lost rounds); Sections 2-10 are data stubs (title/subtitle/description only) marked "Coming Soon".
- Content lives in `story_sections.json` (curated `{book, chapter, verse}` references, resolved against
  `bible_sections.json` at play time via a new `BibleData::getVerse()` lookup).
- Story progress (per-chapter completion + perfect-run flag, section-aced state, bonus completion) persists
  to `bibirble_story_progress.json` in the same per-user data directory as `PersistenceManager`.
- New screens: `StoryMapScreen` (section select), `StorySectionScreen` (chapter select + bonus tile),
  `StoryInterstitialScreen` (reusable chapter-intro/chapter-complete "story beat" screen), and
  `IllustrationPanel` (draws a real image from `assets/story/...` if present, else a placeholder gradient
  card) -- so the whole flow works end-to-end before any art assets exist.
- Bug fix found while authoring Section 1's bonus prophecy chapter: `tools/sort.py` silently dropped any
  verse rendered as poetic "line text" (e.g. Genesis 3:15) and any verse under 7 words (e.g. "Jesus wept.").
  Fixed the parser and regenerated `bible_sections.json` (23,621 -> 31,098 verses, zero duplicates).

Files added:
- `src/StoryData.h/cpp`, `src/StoryProgress.h/cpp`, `src/IllustrationPanel.h/cpp`,
  `src/StoryMapScreen.h/cpp`, `src/StorySectionScreen.h/cpp`, `src/StoryInterstitialScreen.h/cpp`,
  `story_sections.json`

Files changed:
- `src/BibleData.h/cpp` (added `getVerse()`), `src/GameState.h` (added `GameMode::Story`),
  `src/StartScreen.h/cpp` (added the Story Mode button), `src/BibirbleWindow.h/cpp` (screen switching
  generalized into `ShowOnly()`, Story Mode session state and handlers), `CMakeLists.txt`,
  `tools/sort.py`, `bible_sections.json`, `README.md`, `AGENTS.md`

Testing guidance:
1. Build cleanly, run, click "Story Mode" from the start screen.
2. Section 1 ("Creation") should be [Available]; Sections 2-10 should be [Locked]/[Coming Soon].
3. Open Section 1, start Chapter 1, and play through -- confirm the header shows "Verse N of 10", the
   Continue button appears after each round, and the chapter-complete screen shows the correct score.
4. Confirm the bonus "Whispers of the Promise" chapter is locked until Chapter 1-6 are all won without a
   single loss (a section is "aced" only when every chapter is both completed and perfect).
5. Exit mid-chapter via the "Exit" button and confirm progress on that chapter is not saved.
6. Confirm Daily/Random mode still work exactly as before (no Story Mode UI bleeds into classic play).

---

CHANGES for recent integration work

Summary:
- Implemented a startup `LoadingDialog` check and integrated it into `main.cpp`.
- Added a small flag and accessor to `LoadingDialog` to indicate whether the data file was found.

Files changed:
- src/loading_dialog.h
  - Added `bool FoundFile() const` and `m_foundFile` flag.
  - When the JSON file is not located, the dialog sets `m_foundFile = false`.

- src/main.cpp
  - Now shows `LoadingDialog` before creating `BibirbleWindow`.
  - Displays a warning if `bible_sections.json` is missing/unreadable.

Why:
- The project had a `loading_dialog` with logic implemented in the header, but `main.cpp` did not use it. The app could start silently with missing data; adding the dialog gives a clear user-facing check.

Build fix:
- `CMakeLists.txt` now attempts to use the `WXWIN` environment variable or a common `C:/wxWidgets` install path before calling `find_package`. The `find_package` call was also changed to explicitly request `COMPONENTS core base` to improve detection on Windows.

This helps CMake discover locally installed wxWidgets on many Windows setups. If CMake still fails, follow the troubleshooting steps below to point CMake at your wxWidgets installation.

Notes / Next steps:
- `loading_dialog` currently parses `bible_sections.json` only to extract random verse text for animation; it does not hand the full dataset to `BibleData`. `BibirbleWindow` still calls `m_data.loadData("bible_sections.json")` in its constructor.
- If you want the loader to populate `BibleData` directly (e.g., to avoid double-parsing), I can refactor `LoadingDialog` to return the parsed JSON or a path and pass data into `BibleData` before creating `BibirbleWindow`.

Testing guidance:
1. Ensure `bible_sections.json` is located in the same directory as the executable (or one of the tested candidate paths).
2. Build and run the app; the loading dialog should animate and then the main window appears.
3. If the JSON cannot be found, a warning dialog will appear and the main window will start with an empty dataset (the UI will show an error message in the reveal panel).

If CMake still cannot find wxWidgets:

- Check the `WXWIN` environment variable in PowerShell:
```powershell
echo $Env:WXWIN
```
- If it's empty, set it for the session (replace path with your wxWidgets root):
```powershell
$Env:WXWIN = 'C:\path\to\wxWidgets'
```
- Then re-run CMake from a fresh `build` folder, for example:
```powershell
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
```
- If you installed wxWidgets via `vcpkg`, run CMake with the vcpkg toolchain file:
```powershell
cmake -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake ..
```

If you'd like, I can try to detect your installed wxWidgets path in the repo environment or further adapt `CMakeLists.txt` to accept additional hints.

Build script update:
- Updated `build.ps1` to automatically use the vcpkg toolchain file. It defaults to `C:\vcpkg` but accepts a `-VcpkgPath` parameter if your vcpkg is elsewhere. The script now:
  - Ensures a `build` directory exists.
  - Removes `CMakeCache.txt` and `CMakeFiles` from the build directory to avoid stale cache issues.
  - Calls CMake with `-DCMAKE_TOOLCHAIN_FILE` pointing at vcpkg and the Visual Studio 18 2026 generator by default.

Usage example (PowerShell):
```powershell
.\build.ps1 -BuildType Release -VcpkgPath 'C:\vcpkg' -Generator 'Visual Studio 18 2026' -Arch x64
```

Contact me if you want the loader to fully populate `BibleData` to avoid redundant parsing and improve startup performance.

Recent fixes (loader and virtual keyboard):

- Loader search paths extended and animation slowed: `loading_dialog.h` now tries additional relative locations such as `Debug/` and `build/` and uses longer delays so the progress gauge is visible during startup.
- Exposed digit controls in `GameRow` via `GetDigitCtrls()` to allow the main window to bind focus events.
- Virtual keyboard now routes input into the last-focused digit control and restores focus after button presses. Implemented in `BibirbleWindow.cpp` (`HandleKeyPress` and focus bindings).

Files modified for these fixes:
- `src/loading_dialog.h`
- `src/GameRow.h`
- `src/GameRow.cpp`
- `src/BibirbleWindow.cpp`

Next step: run a build and smoke-test the UI to confirm focus and loader behavior.

---

Feature parity pass with Bibirble-web (Daily/Random modes, Hard Mode, Settings, persistence, share grid):

Summary:
- Removed the dead `menu_wireframe.h/cpp` stub (never wired into the app) and built real Start/Settings
  screens in its place.
- Ported the web version's daily-hash/seeded-RNG algorithm bit-for-bit (`src/SeededRandom.h/cpp`), verified
  against the actual JS via node for a range of inputs including today's UTC date.
- Fixed three real bugs while porting rather than replicating them:
  - Nehemiah was miscategorized under "Prophets Minor" instead of "Historical" in both
    `BibleData::getBookArea()` and `tools/sort.py`'s AREAS dict; regenerated `bible_sections.json` so the
    shipped data's area field agrees (confirmed the diff is scoped to exactly the 330 Nehemiah entries).
  - `getRevealedText()` used to re-find each chunk's words by value via `std::find`, so a repeated word in a
    verse would only ever reveal its first occurrence. Reworked `sliceList` into `sliceIndices`, which tracks
    contiguous index ranges directly instead of searching by value.
  - The loading screen could hang forever: `wx_callafter_compat.h` (used because this wxWidgets 3.2 packaging
    has no standalone `<wx/callafter.h>`) queued its callback onto `wxTheApp`, whose pending-event queue is
    only drained by the *main* event loop -- which never starts while `LoadingDialog` is blocked inside
    `ShowModal()` from `OnInit()`. Switched to the real `wxEvtHandler::CallAfter()` member function (queues on
    the dialog itself, which the nested modal loop does drain); `wx_callafter_compat.h` is now deleted as
    unused.
- Introduced `GameState` (mode, seed, Hard Mode, stage, game-over, target verse, guess history) as the single
  source of truth, replacing `BibirbleWindow`'s scattered `m_targetVerse`/`m_currentStage`/`m_gameOver`
  members, and a single canonical `GuessColor` enum shared by `GameRow` and `GameState`.
- Added Hard Mode (`GameState::CheckHardModeViolation`): guesses must respect the previously confirmed
  book/area and revealed digits, and must reference a verse that exists in the dataset.
- Added `PersistenceManager`: a small JSON file under `wxStandardPaths`' per-user data dir remembers the
  daily lockout date and last-used seed, standing in for the web version's `localStorage`.
- Added `StartScreen` (Daily/Random/Settings, an info card, and a live countdown to next UTC midnight) and
  `SettingsScreen` (Hard Mode toggle, seed entry/randomize/readout), swapped in as `wxPanel`s within the
  existing `BibirbleWindow` frame. Games are now replayable (`GameRow::Reset()`), and post-game controls
  (Main Menu / Play Random / View-Set-Seed) are wired inline.
- Reworked Share to build a real Wordle-style emoji grid (`GameState::BuildShareText()`) from guess history,
  instead of a fixed templated sentence.

Files added:
- `src/SeededRandom.h/cpp`, `src/GameState.h/cpp`, `src/GuessColor.h`, `src/PersistenceManager.h/cpp`,
  `src/StartScreen.h/cpp`, `src/SettingsScreen.h/cpp`

Files removed:
- `src/menu_wireframe.h/cpp`, `src/wx_callafter_compat.h`

Testing guidance:
1. Build cleanly via `./build.sh` (or the CMake commands in the README).
2. Start screen shows Start Daily / Start Random / Settings, the info card, and a ticking countdown.
3. Settings: toggle Hard Mode, enter/randomize a seed, Back returns to the correct screen.
4. Start a Daily game, then relaunch and confirm the "already played today" lockout; a Random game with a
   blank seed generates one, persists it, and prefills Settings on next launch.
5. Deliberately violate a Hard Mode constraint and confirm the guess is rejected without being consumed.
6. Finish a game, Share, and confirm the clipboard has the correct emoji grid (only played rows) + citation.
7. Guess "Nehemiah" against a different Historical-area target verse and confirm yellow, not gray.
