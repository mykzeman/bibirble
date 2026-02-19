# Bibirble++

A Bible verse guessing game built with C++ and wxWidgets.

## Project Structure

```
src/
├── main.cpp                 # Application entry point
├── BibirbleWindow.h/cpp     # Main game window (wxWidgets)
├── GameRow.h/cpp            # Bible verse input row component
├── BibleData.h/cpp          # Bible data loading and logic
├── loading_dialog.h         # Loading dialog for data import
├── menu_wireframe.h         # Menu wireframe (optional)
├── wx_callafter_compat.h    # wxWidgets compatibility helper
└── bibirble.cpp             # Placeholder for future features

tools/
└── sort.py                  # Utility script

CMakeLists.txt              # Build configuration
```

## Requirements

### Windows (MinGW)
- **wxWidgets 3.2+** - Download from https://www.wxwidgets.org/
- **nlohmann/json** - C++ JSON library (header-only)
- **CMake 3.16+**
- **MinGW-w64** (g++ 8.0+)

### Ubuntu/Linux
- `sudo apt install libwxgtk3.2-dev`
- `sudo apt install nlohmann-json3-dev`
- `sudo apt install cmake build-essential`

### macOS
- `brew install wxwidgets`
- `brew install nlohmann-json`
- `brew install cmake`

## Build Instructions

### Windows (CMake with MinGW)

```powershell
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build . --config Release

# Run
./Release/Bibirble.exe
```

### Linux/macOS

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build . --config Release

# Run
./Bibirble
```

## Installation Steps

### Step 1: Install Dependencies

**Windows (MinGW):**
1. Download wxWidgets from https://www.wxwidgets.org/downloads/
# Bibirble

Bibirble is a small Bible-verse guessing game written in modern C++ using wxWidgets.

- Players see a partially revealed verse and must guess the book, chapter, and verse.
- Progressive reveal, color-coded feedback, and a small local verse dataset.

**Quick links**
- Source: https://github.com/mykzeman/bibirble
- Data pipeline: `tools/sort.py` → `bible_sections.json`

## Quickstart (Linux / macOS)

Prerequisites:
- wxWidgets 3.2+ (system package or Homebrew)
- nlohmann/json (packaged or header-only)
- CMake 3.16+ and a C++17 toolchain

Build and run:

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
./Bibirble
```

## Quickstart (Windows, MinGW)

Install wxWidgets and make sure `wx-config` or `wxWidgets_DIR` is available to CMake.

```powershell
# from project root
mkdir build
cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
# run the produced executable
.\Bibirble.exe
```

## Project layout

- `src/` - application source (UI, game logic, data loader)
- `tools/sort.py` - helper to convert raw book JSON into `bible_sections.json`
- `bible_sections.json` - compiled verse data used at runtime

## Data format

The game expects `bible_sections.json` next to the executable. Each verse entry should look like:

```json
{
  "testament": "Old Testament",
  "area": "Torah",
  "book": "Genesis",
  "chapter": 1,
  "verse": 1,
  "text": "In the beginning God created the heavens and the earth."
}
```

If you have the original per-book JSON files in `tools/`, run:

```bash
python3 tools/sort.py
# this generates or updates bible_sections.json
```

## Notes and known issues

- The loading dialog and progress UI are minimal; the loader may appear to hang briefly while parsing.
- Book area mappings are defined in both `tools/sort.py` and `src/BibleData.cpp::getBookArea()`; keep them in sync when changing categories.

## Contributing

Contributions welcome. Suggested improvements:
- Add a proper main menu and settings
- Improve loading UX and progress reporting
- Add story mode or verse-of-the-day features

## License & Credits

This project includes or references the World English Bible dataset. See `LICENSE` for project licensing.

Data source reference: https://github.com/TehShrike/world-english-bible

---
Updated README to provide a concise quickstart, data notes, and contribution ideas.
