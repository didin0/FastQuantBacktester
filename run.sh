#!/bin/bash
set -e

echo "Starting FastQuantBacktester..."

if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build
echo "Configuring CMake..."
cmake ..

echo "Building project..."
cmake --build .

echo "Starting Server..."
./fastquant_server
