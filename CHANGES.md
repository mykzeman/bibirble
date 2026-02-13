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
