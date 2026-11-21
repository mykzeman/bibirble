# Build script for Bibirble wxWidgets project (Windows)
# Usage: .\build.ps1 -BuildType Release
#        .\build.ps1 -Triplet x64-windows   # DLL build (exe needs the DLLs + VC++ runtime)
#
# The default triplet, x64-windows-static, links wxWidgets, zlib and the C++
# runtime into Bibirble.exe, so the exe runs on its own. First time:
#   C:\vcpkg\vcpkg install wxwidgets zlib nlohmann-json --triplet x64-windows-static

param(
    [string]$BuildType = "Release",
    [string]$VcpkgPath = "C:\vcpkg",
    [string]$Generator = "Visual Studio 18 2026",
    [string]$Arch = "x64",
    [string]$Triplet = "x64-windows-static"
)

Write-Host "Building Bibirble in $BuildType mode..." -ForegroundColor Cyan

# Ensure build directory exists
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Name "build" | Out-Null
}

Set-Location build

# Remove old CMake cache and intermediate files to ensure a clean configure
if (Test-Path "CMakeCache.txt") {
    Write-Host "Removing old CMakeCache.txt..." -ForegroundColor Yellow
    Remove-Item -Force "CMakeCache.txt"
}
if (Test-Path "CMakeFiles") {
    Write-Host "Removing old CMakeFiles directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "CMakeFiles"
}

# Validate vcpkg toolchain file
$toolchain = Join-Path -Path $VcpkgPath -ChildPath "scripts\buildsystems\vcpkg.cmake"
if (-not (Test-Path $toolchain)) {
    Write-Host "vcpkg toolchain not found at $toolchain" -ForegroundColor Red
    Write-Host "Please install vcpkg to $VcpkgPath or pass -VcpkgPath to this script." -ForegroundColor Red
    exit 1
}

Write-Host "Configuring CMake (generator: $Generator, arch: $Arch, triplet: $Triplet) ..." -ForegroundColor Yellow
cmake -DCMAKE_TOOLCHAIN_FILE="$toolchain" `
      -DVCPKG_TARGET_TRIPLET="$Triplet" `
      -G "$Generator" -A $Arch `
      -DCMAKE_BUILD_TYPE=$BuildType ..

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    exit 1
}

# Build the project
Write-Host "Building project..." -ForegroundColor Yellow
cmake --build . --config $BuildType

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "Build successful!" -ForegroundColor Green
Write-Host "Executable location: .\$BuildType\Bibirble.exe" -ForegroundColor Green
