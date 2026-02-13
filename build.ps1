# Build script for Bibirble wxWidgets project (Windows)
# Usage: .\build.ps1 -BuildType Release

param(
    [string]$BuildType = "Release",
    [string]$VcpkgPath = "C:\vcpkg",
    [string]$Generator = "Visual Studio 18 2026",
    [string]$Arch = "x64"
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

Write-Host "Configuring CMake (generator: $Generator, arch: $Arch) ..." -ForegroundColor Yellow
cmake -DCMAKE_TOOLCHAIN_FILE="$toolchain" `
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
