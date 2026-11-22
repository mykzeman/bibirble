# Bibirble Agent Instructions

## Project Overview

**Bibirble** is a Bible verse guessing game built with C++ and wxWidgets. Players see partial Bible verse text and must guess the book, chapter, and verse reference. The game has a progressive reveal system with difficulty stages.

## Architecture

### Core Data Flow
1. **Bible Data Pipeline**: the versions are listed in `tools/bible_versions.json`. Each has one JSON file per book in `tools/bibles/<version>/` (`web` in the world-english-bible project's format; the rest as chapter -> verse -> text, written by `tools/fetch_ebible.py` for the eBible.org versions or `tools/split_bible.py` for others). `tools/sort.py` turns them into `bibles/<id>.json.gz` (gzipped, embedded) and generates `src/BibleVersions.h`, `bibles/bibles.rc` and Bibirble-web's `scripts/version_list.js` + `scripts/bibles/` (the web keeps a plain `scripts/bible_sections.json` for the World English Bible)
2. **Game Loader**: `BibleData::loadVersion()` parses the version picked in Settings (`src/BibleVersions.h`; saved by PersistenceManager as `bibleVersion`), populating verse objects with `{testament, area, book, chapter, verse, text}` plus a standard reference (`stdChapter`/`stdVerse`, from the file's `"ref"`): the verse's World English Bible numbering. Useful verses, Christmas verses, Story Mode pools and Story save data all use the standard reference (`BibleData::findStdRef()`), so they work in every version (the Douay-Rheims' Psalm 22 is psalms 23). Guessing and the shown answer use the version's own numbers. A new version from Settings applies between games (`ApplyBibleVersionSetting()`), never mid-game
3. **Mode/seed selection**: `StartScreen`/`SettingsScreen` pick Daily vs. Random, a seed, and Hard Mode; `SeededRandom` resolves that into a verse index (`GetDailySeed()` for Daily, `HashStringToInt()`/`GenerateRandomSeed()` for Random) via `BibleData::getVerseAtIndex()`
4. **Game Loop**: `BibirbleWindow` owns a `GameState` (single source of truth for mode/seed/stage/history), processes turns, and uses `getRevealedText()` with difficulty stages

### Component Responsibilities

- **BibirbleWindow** (`src/BibirbleWindow.h/cpp`): Main wxWidgets frame. Swaps between StartScreen/SettingsScreen/game panel, handles keyboard events and turn processing
- **StartScreen** / **SettingsScreen** (`src/StartScreen.h/cpp`, `src/SettingsScreen.h/cpp`): Mode-select landing screen and Hard Mode/seed configuration (plus the Bible version picker and the Credits section, which names the translations and data sources), as panels swapped within `BibirbleWindow` rather than separate top-level frames
- **Orientation**: the game panel switches between portrait (number keys below the rows) and landscape (keys on the right) from the window's shape, in `BibirbleWindow::ApplyGameOrientation()`
- **GameRow** (`src/GameRow.h/cpp`): Input component for one guess (book dropdown + 4 digit TextCtrls for chapter/verse). Provides color feedback validation (`GuessColor`) and a `Reset()` for replayable games
- **GameState** (`src/GameState.h/cpp`): Single source of truth for mode, seed, Hard Mode, current stage, game-over, target verse, and guess history (`GuessRecord` list). Also owns Hard Mode constraint checking and the emoji share-text builder
- **GuessColor** (`src/GuessColor.h`): The one canonical Gray/Yellow/Green enum, shared by GameRow (rendering) and GameState (domain model)
- **BibleData** (`src/BibleData.h/cpp`): Verse storage + utility functions. Book areas hardcoded in `getBookArea()` map (must match `tools/sort.py` AREAS definition). Knows the deuterocanonical books (Tobit, Judith, Wisdom, Sirach, Baruch, 1-2 Maccabees) that only the Douay-Rheims and CPDV have
- **SeededRandom** (`src/SeededRandom.h/cpp`): Bit-for-bit port of the web version's `data.js` hash/seed algorithm (`HashStringToInt`, xorshift32 `SeededRandom`, `PickIndexFromSeed`, `GetDailySeed`), so Daily puzzles match the web version exactly
- **PersistenceManager** (`src/PersistenceManager.h/cpp`): Small JSON file under `wxStandardPaths`' per-user data dir, standing in for the web version's `localStorage` (daily lockout date, last seed)
- **Story Mode** (`src/StoryMap.h/cpp`, `src/StoryProgress.h/cpp`, `src/StoryMapScreen.h/cpp`): the map is data, not code: `story_map.json` (embedded like `bible_sections.json`; a copy on disk wins) defines areas, levels, story text, verse pools, star rules, unlocks and map regions: every level has a `region` (a place on the Bible-lands picture `data/story_map.png`, embedded like the other data; regions' `x`/`y` are percentages of the picture). `src/StoryWorldMap.h/cpp` draws the picture with a pin per region (the land around Israel shares one pin that zooms into a `closeUp` square; `offMap` regions such as Eden are buttons under the map), and `StoryMapScreen` lists the picked region's levels under it. In a Story level the book dropdown holds only the books of the level's story area (`BibirbleWindow::StoryAreaBooks`). `StoryProgress` (no wx) holds stars/unlocks/retries and saves under PersistenceManager's `story` key. Design: `docs/STORY_MODE.md`. Logic tests: `tests/story_tests.cpp` (`story_tests` target / `ctest`). Check map edits with `python tools/check_story_map.py` (checks every version; pools are written in World English Bible numbers and must all exist there, while another version may lack a passage, which the game skips there as long as the level keeps 3+ verses). Story text can hold Wordle-style fill-in-the-blank puzzles (`{{answer|hint}}`), handled by `src/WordPuzzle.h/cpp` (no wx) and the full-screen `src/StoryScreen.h/cpp`; guesses are checked against `data/words.txt` (the public-domain ENABLE word list, no license needed), embedded like the other data files
- **main.cpp**: wxApp bootstrapper that shows `LoadingDialog` then creates `BibirbleWindow` on startup

### Data Structure Consistency

**Critical invariant**: The Bible book area classifications are duplicated in THREE places:
1. `tools/sort.py`: AREAS dict (defines how raw data is categorized)
2. `src/BibleData.cpp`: getBookArea() static map (queried by UI for book color feedback)
3. Bibirble-web's `scripts/books.js`: AREAS

When modifying book categories, update ALL of them identically. Hard mode's authors (`BibleData::getBookAuthor()`) must likewise match AUTHORS in Bibirble-web's `scripts/books.js`. The list of Bible versions lives only in `tools/bible_versions.json`; everything else is generated from it by `tools/sort.py`. New areas hold at most 8 books (only the original areas and Paul's letters are bigger).

## Build & Development Workflow

### Windows Development
```powershell
.\build.ps1              # Build Release (creates ./build/ directory)
.\build.ps1 -BuildType Debug
.\build\Release\Bibirble.exe   # Run executable
.\build.ps1 -Triplet x64-windows   # DLL build instead (needs the DLLs + VC++ Redistributable)
```

`build.ps1` defaults to the vcpkg `x64-windows-static` triplet, and `CMakeLists.txt` switches to the static MSVC runtime (`/MT`) for `-static` triplets, so `Bibirble.exe` is a true single file: no wxWidgets/zlib DLLs and no Visual C++ Redistributable. One-time setup on a new machine:
```powershell
C:\vcpkg\vcpkg install wxwidgets zlib nlohmann-json --triplet x64-windows-static
```

### Linux/macOS Development
```bash
./build.sh Release
./build/Bibirble
```

### Build Dependencies
- **wxWidgets 3.2+**: GUI framework. CMake finds via `find_package(wxWidgets)`. Config varies by platform (MinGW on Windows, apt on Linux, Homebrew on macOS)
- **nlohmann/json**: Header-only JSON parser. CMake finds via `find_package(nlohmann_json)`
- **CMake 3.16+**: Build system. C++17 standard required

### Data Pipeline
```bash
python tools/sort.py  # from the repo root: reads tools/bibles/<version>/*.json, writes bibles/<id>.json.gz
python tools/check_story_map.py
```

New source data: `python tools/fetch_ebible.py [ids]` re-downloads the eBible.org versions; `python tools/split_bible.py <version> <whole-bible.json or folder>` splits anything else. Psalm 151 is skipped on purpose.

Every `bibles/*.json.gz` is compiled into the program (`Bibirble.rc` -> `bibles/bibles.rc` on Windows; a CMake-generated `.incbin` blob plus id table on Linux) and unpacked with zlib so the game ships as a single file; `src/EmbeddedData.cpp` reads it. A `bibles/<id>.json.gz` found on disk (see `BibleData::ResolveDataFilePath()`) still wins, so new `tools/sort.py` output can be tested without rebuilding. macOS has no embedded copy yet and still needs the file next to the app.

The app icon lives in `icons/` (artwork copied from Bibirble-web's `icons/`): `bibirble.ico` is the `appicon` resource in `Bibirble.rc` (Explorer icon + title bar on Windows), and `bibirble.png` is `.incbin`'d on Linux and set as the window icon by `LoadAppIcons()` in `BibirbleWindow.cpp`. `bibirble.desktop` adds it to a Linux app menu.

## Key Patterns & Conventions

### Verse Reveal System
Progressive text reveal across game stages (see `BibleData::getRevealedText()`):
- Stage 0: Minimal text (e.g., 1st word only)
- Stage N: More words revealed
- Final stage: Full verse

Implementation uses `sliceList()` helper to partition verse words into chunks based on stage count.

### Event Handling
wxWidgets event pattern throughout:
- Buttons/inputs emit `wxCommandEvent`
- Handlers declared in `wxDECLARE_EVENT_TABLE()` and bound in `.cpp` via `EVT_*` macros
- Example: `OnVirtualKeyClicked()` processes virtual keyboard presses

### Memory Management
wxWidgets uses parent-child ownership (constructor parameter `wxWindow* parent`). Avoid manual `delete`; destruction cascades automatically when parent destroyed.

### Async calls from background threads
`LoadingDialog` (`src/loading_dialog.h`) runs its JSON parse on a background `std::thread` and needs to touch the gauge/end the modal from the main thread. Use the real `wxEvtHandler::CallAfter()` **member function** (e.g. `this->CallAfter([this]{...})`), not a free `wxCallAfter(fn)` -- this wx 3.2 packaging has no `<wx/callafter.h>`, and a handler-less shim that queues onto `wxTheApp` will never be dispatched while the dialog is shown via `ShowModal()` from inside `wxApp::OnInit()` (before the main loop starts drains `wxTheApp`'s pending events). `CallAfter()` on the dialog itself queues onto the dialog's own pending-event list, which the nested modal loop does drain correctly.

### Text encoding
Verse and story text is UTF-8 (curly quotes, dashes, emoji). Always convert it with `wxString::FromUTF8()`. Passing a `std::string` to `wxString::Format("%s", ...)`, or to wxString's constructor, uses the C locale: non-ASCII text turns into "(null)", and a raw non-ASCII format string can crash.

### Flexible layout
The window can be resized freely (down to 420x420), so nothing may rely on a fixed width. Use `WrappingText` (`src/WrappingText.h`) instead of `wxStaticText` + `Wrap(N)` for any multi-line text, and add it with `wxEXPAND` (or a proportion in a horizontal sizer) so it re-wraps to the width it's given. Rows of buttons use `wxWrapSizer` so they move onto a second line rather than overflow.

### Color Feedback
`GameRow::setDigitColors()` and `setBookColor()` apply validation feedback, taking a `GuessColor` (Gray/Yellow/Green -- see `src/GuessColor.h`), not string literals. Integration with game logic in `BibirbleWindow::ProcessTurn()`, which also records a `GuessRecord` onto `GameState::history`.


## Testing Notes

Story Mode's logic has tests: `cmake --build build --target story_tests && ctest --test-dir build`. Everything else is tested by hand:
1. Build project
2. Ensure `bibles/*.json.gz` are built (they're embedded)
3. Run executable and manually test game flow
4. Verify color feedback and verse reveal progression
## Project History
Originally I worked on it a year ago but stopped . Recently I made AI recreate the project from scratch with improved code quality and modern C++ practices. Because of this , some files are useless and not implemented properly .

### Files not needed
1. `src/bibirble.cpp` - Placeholder file, not used
2. `src/vc140.pdb` - Debug symbols file, not needed for development

`src/menu_wireframe.h/cpp` was removed (was never wired into the app) when the real Start/Settings screens were built.

### Feature parity with the web version (`Bibirble-web`)
The C++ port now matches the web version's front end: Daily vs. Random mode selection, a seeded/shareable-puzzle
system (bit-for-bit identical daily verse via `SeededRandom`), Hard Mode, a Settings screen, daily-lockout
persistence (`PersistenceManager`), and a Wordle-style emoji share grid (`GameState::BuildShareText()`). See
`CHANGES.md` for the detailed commit-by-commit history of that work, including three bug fixes made along the
way: Nehemiah was miscategorized as a minor prophet, `getRevealedText()` could only ever reveal the first
occurrence of a repeated word, and the loading screen could hang forever (see the CallAfter note above).