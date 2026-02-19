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
2. Extract and follow the MinGW build instructions in `docs/msw/install.txt`
3. Install nlohmann/json:
   - Download from https://github.com/nlohmann/json/releases
   - Extract to a known location

**Linux (Ubuntu/Debian):**
```bash
sudo apt update
sudo apt install libwxgtk3.2-dev nlohmann-json3-dev cmake build-essential
```

**macOS:**
```bash
brew install wxwidgets nlohmann-json cmake
```

### Step 2: Configure Build Environment

**Windows (MinGW):** Add wxWidgets and nlohmann_json paths to CMake:
```powershell
# Edit CMakeLists.txt or use environment variables
$env:wxWidgets_DIR = "C:\path\to\wxWidgets"
$env:nlohmann_json_DIR = "C:\path\to\nlohmann_json"
```

### Step 3: Build Project

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

### Step 4: Run Application

The executable will be in `build/Release/` or `build/` depending on platform.

```bash
./Release/Bibirble  # Windows
./Bibirble         # Linux/macOS
```

## Game Data

The game requires a `bible_sections.json` file in the same directory as the executable. This file should contain an array of Bible verse objects with the following structure:

```json
[
  {
    "testament": "Old Testament",
    "area": "Torah",
    "book": "Genesis",
    "chapter": 1,
    "verse": 1,
    "text": "In the beginning God created the heavens and the earth."
  },
  ...
]
```

## Key Features

- **Interactive Gameplay**: Guess Bible verses by book, chapter, and verse number
- **7 Attempts**: Get 7 guesses to find the correct answer
- **Progressive Revelation**: More of the verse text reveals with each incorrect guess
- **Color-Coded Feedback**: 
  - Green: Correct guess
  - Yellow: Correct book region/digit in wrong position
  - Gray: Incorrect
- **Share Results**: Copy your game results to clipboard

## Troubleshooting

### CMake can't find wxWidgets
- Set `wxWidgets_CONFIG_EXECUTABLE` to the path of `wx-config` script
- On Windows, set `wxWidgets_DIR` to your wxWidgets installation directory

### JSON parsing errors
- Ensure `nlohmann/json.hpp` is in your include path
- Verify `bible_sections.json` format is valid JSON

### Compilation errors on Windows
- Use the same build system (MinGW) for both wxWidgets and your project
- Ensure Unicode mode is enabled in wxWidgets configuration

## Contributing

Feel free to fix and extend this project with:
- More game modes
- Statistics tracking
- Difficulty settings
- Custom verse sets

### Known Issues 
- Some character encoding issues with certain Bible verses (check `bible_sections.json` for anomalies)
- False warning on data load even the file is there 
- Loading bar doesn't update at all (closing the dialog gets you into the game)
- No main menu or settings yet 
- Text is not vertically or horizontally centered
- No sound effects or animations 

# Important Infomation

The data for this game was sourced from TehShrike's programmatic version of the World English Bible, which is available on GitHub:
[Repo](https://github.com/TehShrike/world-english-bible)



   Copyright 2026 Mykal Mayne

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     [http://www.apache.org/licenses/LICENSE-2.0](http://www.apache.org/licenses/LICENSE-2.0)

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
