#!/bin/bash

# Exit on error
set -e

# Project configuration
BUILD_DIR="build"
EXECUTABLE="PixelsGate"

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir "$BUILD_DIR"
fi

# Navigate to build directory
cd "$BUILD_DIR"

# Configure the project
echo "Configuring project with CMake..."
cmake ..

# Build the project
echo "Building project..."
cmake --build .

# Check if build was successful
if [ -f "$EXECUTABLE" ]; then
    echo "Build successful! You can run the game with: ./build/$EXECUTABLE"
    
    # Optional: Run the game immediately
    if [[ "$1" == "--run" ]]; then
        echo "Launching $EXECUTABLE..."
        ./"$EXECUTABLE"
    fi
else
    echo "Build failed: Executable not found."
    exit 1
fi
