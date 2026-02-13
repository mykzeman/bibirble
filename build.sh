#!/bin/bash
# Build script for Bibirble wxWidgets project
# Usage: ./build.sh [Release|Debug]

BUILD_TYPE=${1:-Release}

echo "Building Bibirble in $BUILD_TYPE mode..."

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# Run CMake configuration
echo "Configuring CMake..."
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

# Build the project
echo "Building project..."
cmake --build . --config $BUILD_TYPE

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build successful!"
echo "Executable location: ./Release/Bibirble (Linux/macOS) or ./Release/Bibirble.exe (Windows)"
