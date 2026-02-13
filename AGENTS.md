# Bibirble Agent Instructions

## Project Overview

**Bibirble** is a Bible verse guessing game built with C++ and wxWidgets. Players see partial Bible verse text and must guess the book, chapter, and verse reference. The game has a progressive reveal system with difficulty stages.

## Architecture

### Core Data Flow
1. **Bible Data Pipeline**: `tools/sort.py` pre-processes raw Bible JSON files (from `tools/*.json`) into grouped verses, outputting `bible_sections.json`
2. **Game Loader**: `BibleData::loadData()` parses `bible_sections.json` at startup, populating verse objects with `{testament, area, book, chapter, verse, text}`
3. **Game Loop**: `BibirbleWindow` manages UI state, calls `BibleData::getRandomVerse()` for new rounds, and uses `getRevealedText()` with difficulty stages

### Component Responsibilities

- **BibirbleWindow** (`src/BibirbleWindow.h/cpp`): Main wxWidgets frame. Handles UI layout, keyboard events, game state (m_currentStage, m_gameOver), and turn processing
- **GameRow** (`src/GameRow.h/cpp`): Input component for one guess (book dropdown + 4 digit TextCtrls for chapter/verse). Provides color feedback validation
- **BibleData** (`src/BibleData.h/cpp`): Verse storage + utility functions. Book areas hardcoded in `getBookArea()` map (must match `tools/sort.py` AREAS definition)
- **main.cpp**: wxApp bootstrapper that creates BibirbleWindow on startup

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
wxWidgets uses parent-child ownership (constructor parameter `wxWindow* parent`). Avoid manual `delete`; destruction cascades automatically when parent destroyed. See [wx_callafter_compat.h](src/wx_callafter_compat.h) for platform-specific async call helpers.

### Color Feedback
`GameRow::setDigitColors()` and `setBookColor()` apply validation feedback. Colors passed as strings ("green", "yellow", "red" etc). Integration with game logic in `BibirbleWindow::ProcessTurn()`.


## Testing Notes

No automated test framework currently visible. Manual testing workflow:
1. Build project
2. Ensure `bible_sections.json` is accessible
3. Run executable and manually test game flow
4. Verify color feedback and verse reveal progression
## Project History 
Originally I worked on it a year ago but stopped . Recently I made AI recreate the project from scratch with improved code quality and modern C++ practices. Because of this , some files are useless and not implemented properly . 

### Files not needed 
1. `src/menu_wireframe.h` - Not used in the project
2. `src/bibirble.cpp` - Placeholder file , not used
3. ``menu_wireframe.cpp` - Not used in the project
4. `src/vc140.pdb` - Debug symbols file , not needed for development

### files that need to be implemented properly
1. `src/loading_dialog.h/cpp` - Placeholder for loading screen dialog, not fully implemented, should be where the game loads `bible_sections.json` and shows progress and enter point.

### what i need you to do
manage  and see if ther are any files that are not inculed within the main loop and needed for delevopment. Fully intergrate the files so that they are unafided togeather and is ready for development. Update `CHANGES.md` with the changes you made and the files you integrated.