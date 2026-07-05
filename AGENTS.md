# Bibirble Agent Instructions

## Project Overview

**Bibirble** is a Bible verse guessing game built with C++ and wxWidgets. Players see partial Bible verse text and must guess the book, chapter, and verse reference. The game has a progressive reveal system with difficulty stages.

## Architecture

### Core Data Flow
1. **Bible Data Pipeline**: `tools/sort.py` pre-processes raw Bible JSON files (from `tools/*.json`) into grouped verses, outputting `bible_sections.json`
2. **Game Loader**: `BibleData::loadData()` parses `bible_sections.json` at startup, populating verse objects with `{testament, area, book, chapter, verse, text}`
3. **Mode/seed selection**: `StartScreen`/`SettingsScreen` pick Daily vs. Random, a seed, and Hard Mode; `SeededRandom` resolves that into a verse index (`GetDailySeed()` for Daily, `HashStringToInt()`/`GenerateRandomSeed()` for Random) via `BibleData::getVerseAtIndex()`
4. **Game Loop**: `BibirbleWindow` owns a `GameState` (single source of truth for mode/seed/stage/history), processes turns, and uses `getRevealedText()` with difficulty stages

### Component Responsibilities

- **BibirbleWindow** (`src/BibirbleWindow.h/cpp`): Main wxWidgets frame. Swaps between StartScreen/SettingsScreen/game panel, handles keyboard events and turn processing
- **StartScreen** / **SettingsScreen** (`src/StartScreen.h/cpp`, `src/SettingsScreen.h/cpp`): Mode-select landing screen and Hard Mode/seed configuration, as `wxPanel`s swapped within `BibirbleWindow` rather than separate top-level frames
- **GameRow** (`src/GameRow.h/cpp`): Input component for one guess (book dropdown + 4 digit TextCtrls for chapter/verse). Provides color feedback validation (`GuessColor`) and a `Reset()` for replayable games
- **GameState** (`src/GameState.h/cpp`): Single source of truth for mode, seed, Hard Mode, current stage, game-over, target verse, and guess history (`GuessRecord` list). Also owns Hard Mode constraint checking and the emoji share-text builder
- **GuessColor** (`src/GuessColor.h`): The one canonical Gray/Yellow/Green enum, shared by GameRow (rendering) and GameState (domain model)
- **BibleData** (`src/BibleData.h/cpp`): Verse storage + utility functions. Book areas hardcoded in `getBookArea()` map (must match `tools/sort.py` AREAS definition)
- **SeededRandom** (`src/SeededRandom.h/cpp`): Bit-for-bit port of the web version's `data.js` hash/seed algorithm (`HashStringToInt`, xorshift32 `SeededRandom`, `PickIndexFromSeed`, `GetDailySeed`), so Daily puzzles match the web version exactly
- **PersistenceManager** (`src/PersistenceManager.h/cpp`): Small JSON file under `wxStandardPaths`' per-user data dir, standing in for the web version's `localStorage` (daily lockout date, last seed)
- **main.cpp**: wxApp bootstrapper that shows `LoadingDialog` then creates `BibirbleWindow` on startup
- **StoryData** (`src/StoryData.h/cpp`): Loads `story_sections.json` into `StorySection`/`StoryChapter`/`StoryVerseRef`. Verse refs are just `{book, chapter, verse}`; `BibleData::getVerse()` resolves them to full text at play time
- **StoryProgress** (`src/StoryProgress.h/cpp`): Small JSON file (`bibirble_story_progress.json`, same per-user data dir as `PersistenceManager`) tracking, per section, which chapters are completed and whether each was a "perfect" run (no lost verse rounds). A section is "aced" once every chapter is completed *and* perfect, which unlocks its bonus prophecy chapter
- **IllustrationPanel** (`src/IllustrationPanel.h/cpp`): Resolves and draws a Story Mode image from `assets/story/...`, or a placeholder gradient card with a caption if the file isn't present yet
- **StoryMapScreen** / **StorySectionScreen** / **StoryInterstitialScreen** (`src/Story*.h/cpp`): Story Mode screens -- section-select, chapter-select within a section, and a reusable "story beat" screen (chapter intro with a "Begin" button, chapter-complete summary with a "Continue" button)

### Story Mode gameplay

Story Mode reuses the *same* `GameState`/`GameRow`/reveal-panel/keyboard UI as Daily/Random -- a story
"chapter" is just 10 curated verses (from `StoryChapter::verses`) played back-to-back as individual
verse-guessing rounds (`GameMode::Story`, hard mode off). `BibirbleWindow` tracks the active session
(`m_storySectionId`, `m_storyChapterIndex` -- `-1` for the bonus chapter --, `m_storyVerseIndex`,
`m_storyCorrectCount`, `m_storyChapterPerfect`) and drives it through `StartStoryVerse()` ->
`HandleStoryRoundFinished()` -> either the next verse or `FinishStoryChapter()`, which records the result
in `StoryProgress` and shows the chapter-complete interstitial.

### Data Structure Consistency

**Critical invariant**: The Bible book area classifications are duplicated in TWO places:
1. `tools/sort.py`: AREAS dict (defines how raw data is categorized)
2. `src/BibleData.cpp`: getBookArea() static map (queried by UI for book color feedback)

When modifying book categories, update BOTH locations identically.

## Build & Development Workflow

### Windows Development
```powershell
.\build.ps1              # Build Release (creates ./build/ directory)
.\build.ps1 -BuildType Debug
.\build\Bibirble.exe     # Run executable
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
cd tools/
python sort.py  # Reads *.json, outputs bible_sections.json
cd ..
```

The application expects `bible_sections.json` in the working directory at runtime. Pre-packaged data or auto-load mechanism may exist (check `BibirbleWindow` constructor).

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

### Color Feedback
`GameRow::setDigitColors()` and `setBookColor()` apply validation feedback, taking a `GuessColor` (Gray/Yellow/Green -- see `src/GuessColor.h`), not string literals. Integration with game logic in `BibirbleWindow::ProcessTurn()`, which also records a `GuessRecord` onto `GameState::history`.


## Testing Notes

No automated test framework currently visible. Manual testing workflow:
1. Build project
2. Ensure `bible_sections.json` is accessible
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

### Story Mode (added on top of the above)

A fourth bug was found and fixed while building Story Mode's curated content: `tools/sort.py` only ever
read `"paragraph text"` elements from the raw per-book JSON, so it silently dropped any verse rendered
entirely as poetic `"line text"` (e.g. Genesis 3:15, the Genesis 9:26-27 blessing -- exactly the kind of
passage Story Mode's bonus prophecy chapters need). It also applied its 7-word minimum *before* a verse's
later "line text" lines were counted, so a short opening line like "He said," could cause a genuinely long
poetic verse to be dropped early. `sort.py` now accumulates both `"paragraph text"` and `"line text"` per
`(chapter, verse)`, flushes on every verse-number change, and only *then* applies the 7-word-minimum gate
to the fully assembled text -- so the main game's verse pool still excludes very short verses (e.g. "Jesus
wept.", John 11:35, stays out) but no longer drops long poetic verses that were merely *formatted* as
multiple lines. Regenerating `bible_sections.json` with the fix went from 23,621 to 30,697 verses; a
duplicate check found zero duplicate references, and spot-checked entries (Psalm 23, Genesis 1, Genesis
3:15) read correctly and completely.