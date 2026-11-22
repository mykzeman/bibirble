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
  against the actual JS via node for a range of inputs including today's date.
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
- Added `StartScreen` (Daily/Random/Settings, an info card, and a live countdown to the next Daily) and
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

## Story Mode

A new mode on the start screen: a journey through the Bible in levels of 3 random verses each, with
up to 7 stars per level, star-gated unlocks, hidden prophecy bonus levels and a hidden End Times
area. Design: `docs/STORY_MODE.md`; map: `story_map.json`.

- New: `StoryMap` (loads/validates the map), `StoryProgress` (stars, unlocks, retries, save data),
  `StoryMapScreen` (the map UI), `tests/story_tests.cpp` (logic tests, `story_tests` target).
- `BibirbleWindow`: Story bar, 3-verse level loop, fail -> retry prompt, results box.
- `story_map.json` is embedded (Linux `.incbin`, Windows `STORY_MAP_JSON`); a copy on disk wins.
- `PersistenceManager` saves Story progress under `story`; `GameRow::SetGuess` restores a
  half-played verse.
- Fix found while testing: a `wxString::Format` string containing a raw non-ASCII character
  crashed in the C locale, so non-ASCII text is now joined in with `wxString::FromUTF8`.

## Story puzzles (fill in the blank)

Story text can now hold Wordle-style fill-in-the-blank puzzles: `{{answer|hint}}` in a level's
`intro`, `between` (after verses 1 and 2) or `outro`. Unlimited guesses of real English words (or any
answer in the map), green/yellow/gray letters, a hint after 6 misses, and they must be solved to
continue. See `docs/STORY_MODE.md` §3.5.

- New: `src/WordPuzzle.*` (parsing, colouring, word list), `src/StoryPuzzleDialog.*` (the story
  card), `data/words.txt` (the public-domain ENABLE English word list, embedded).
- `StoryRun::storyDone` remembers an unfinished story, so it comes back on Continue.
- `tools/check_story_map.py` checks blanks.
- Fix: verses with curly quotes or other non-ASCII text showed "(null)" in the verse panel at the end
  of a game (every mode). `UpdateRevealText()` now converts with `wxString::FromUTF8`.

## Story pages are full screens; public-domain word list

- Story text and its puzzles now show on a full screen (`src/StoryScreen.*`, swapped in like the
  map) instead of a pop-up. A passed level's results and its outro story share one screen.
- The puzzle word list is now ENABLE (172,823 words, public domain) instead of SCOWL, so no license
  notice has to ship with it.
- Story puzzles have an on-screen QWERTY keyboard (Enter, Del) whose keys turn green/yellow/gray like
  Wordle's.

## Landscape, key colours, credits

- **Landscape mode:** when the window is wider than tall, the number keys sit in a column to the right
  of the verse and guess rows (`BibirbleWindow::ApplyGameOrientation`, checked when a game starts and on
  every resize). On screens too short for the 850px portrait window (most laptops) the app opens
  in landscape (about 1000x680). The window can now be resized down to 420x420.
- **Number keys change colour** like the letter keyboard: each digit shows its best clue so far
  (`UpdateDigitKeyColours`, using `Theme::ClueColour`, the guess rows' colours).
- **No key outlines:** both keyboards use flat `wxBORDER_NONE` keys on the page background.
- **Hard Mode story puzzles** follow Wordle's hard mode (`WordHardModeViolation`).
- The "World English Bible Version" tagline moved from the top of the window to a new **Credits**
  section in Settings (Bible text, word list, icon/original game, Wordle, wxWidgets, nlohmann/json).
  Settings now scrolls.

## Bible versions

Settings has a Bible version picker: World English Bible (default), King James Version, Douay-Rheims
1899 American Edition and Catholic Public Domain Version.

- Data: each version is split into one file per book under `tools/bibles/<version>/` (the WEB files moved
  there from `tools/`). `tools/split_bible.py` does the splitting; `tools/sort.py` now builds
  `bible_sections*.json` for every version (one verse per line, about 7 MB each instead of 9 MB; the WEB
  verses are unchanged, so the Daily is too) and copies them to Bibirble-web.
- Douay-Rheims comes from yoarikso/douayrheimsbible. The first DR file tried skipped 46 chapters and
  numbered the rest straight on (its "Ruth 2" was Ruth 4), so it was replaced. This one lacks only
  Malachi 4 and Esther 11-16.
- The Douay-Rheims and CPDV number most Psalms like the Vulgate (Psalm 22 = Psalm 23). sort.py gives
  each such verse a `"ref"` (its World English Bible place), lining psalms up by their wording. Useful
  verses, Christmas verses, Story pools and Story save keys use it, so they work in every version and
  Story progress carries over between versions.
- The deuterocanonical books (Tobit, Judith, Wisdom, Sirach, Baruch, 1-2 Maccabees) are in the book
  order, names and areas.
- Story Mode: a version missing a passage (DR's Malachi 4) skips it; `check_story_map.py` checks every
  version.
- All four versions are embedded (`Bibirble.rc`, `BIBIRBLE_BLOBS` in CMakeLists.txt).
- Share text names the version; Credits name every translation and data source.
- Fix: `calculateSliceSteps` rounded down where data.js rounds up (no visible change with 7 rows).

## 32 Bible versions, Prophet Alternatives and Maccabees areas, web-style hard mode and sharing

- main's `translations/` (31 ebible.org translations) was replaced: 21 of them had saved every book's
  chapter N to the same file, so only the last book survived. `tools/fetch_ebible.py` re-downloads them
  complete and splits them into `tools/bibles/<id>/<book>.json`; KJV and Douay-Rheims now come from there too
  (the ebible DRA is complete). `tools/bible_versions.json` lists all 32 versions (31 ebible minus the WEB
  duplicate, plus the CPDV); Psalm 151 is skipped.
- `sort.py` writes gzipped `bibles/<id>.json.gz` (42 MB for all 32, instead of ~230 MB of JSON), detects
  Vulgate/Septuagint psalm numbering by itself, and generates the version lists for both games. The WEB
  verses are unchanged (same Daily).
- C++ reads the gzipped files with zlib; CMake embeds every `bibles/*.json.gz` (Windows: `bibles/bibles.rc`).
- New books and areas: Wisdom, Sirach, Prayer of Manasseh and Psalms of Solomon join Poetry; Tobit, Judith and
  Greek Esther join Small Stories; 1 Esdras joins History; new **Prophet Alternatives** (Baruch, Letter of
  Jeremiah, Greek Daniel, Song of the Three, Susanna, Bel, 2 Esdras) and **Maccabees** (1-4) areas.
- Story Mode in partial Bibles: a level without 3 verses in that version shows "Not in this Bible".
- Like the web version: hard mode's yellow book means the same author (`BibleData::getBookAuthor`), book
  hints work in hard mode, and the share text is spoiler-free ("Bibirble Daily 2026-11-23 KJV 4/7*" + grid).

## Story Mode map picture

- Story Mode opens on a map of the Bible lands (`data/story_map.png`, embedded). `story_map.json` gains
  `regions` (24 places, from Rome to Persia) and a `region` for every level, set to where its story happens.
- `StoryWorldMap` draws the picture scaled to the window (at most ~60% of its height) with a pin per region
  (orange: playable, gold: all passed, grey: locked; the number of levels inside). Clicking a pin lists that
  region's levels underneath, each showing its story area; it opens on the region you're up to.
- `StoryMap` and `check_story_map.py` check every level has a known region; tests cover it.

## Story map: sharper picture, Israel close-up, places off the map, shorter book lists

- A sharper map picture (1400 x 752), with its white border made see-through; pins moved to match.
- The crowded land around Israel has a close-up (`closeUp` in story_map.json): one 🔍 pin on the whole map,
  zoomed in each place gets its own named pin; "Whole map" zooms back out.
- Places nobody can pin down (Garden of Eden, Land of Uz, Mount Sinai, the Red Sea crossing) are
  `"offMap"`: buttons under the map instead of guessed spots.
- Story levels' book dropdown lists only the books of the level's story area (5-17 books instead of 66+).
- Mount Sinai and the Red Sea crossing are back on the map at their traditional, approximate spots
  (`"approximate"`: dashed ring, "(approx.)" label); only the Garden of Eden and the Land of Uz stay off it.
- The End Times area (`"allBooks": true`) offers every book in its book list.

## Tutorial level

- Story Mode's first level ("tutorial": true) shows a step-by-step tip bar: pick a book, type the digits,
  submit, then what the player's own colours mean, how stars and passing work. "Skip tips" hides it.
- Enter submits a guess (like the web version's Enter key).
