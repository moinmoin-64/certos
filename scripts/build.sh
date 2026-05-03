#!/bin/bash
set -e

echo "=============================================="
echo " Building CertOS HPC Cloud OS"
echo "=============================================="

# Ensure we are in the project root
cd "$(dirname "$0")/.."

# Check if vcpkg exists, if not, try to detect it
if [ -z "$VCPKG_ROOT" ]; then
    if [ -d "$HOME/vcpkg" ]; then
        export VCPKG_ROOT="$HOME/vcpkg"
        echo "Auto-detected VCPKG_ROOT at $VCPKG_ROOT"
    else
        echo "ERROR: VCPKG_ROOT is not set."
        echo "Please set it to your vcpkg installation path."
        echo "Example: export VCPKG_ROOT=/path/to/vcpkg"
        exit 1
    fi
fi

echo "[1/3] Generating CMake Configuration (Release)..."
cmake --preset default

echo "[2/3] Compiling Project..."
cmake --build build -j$(nproc)

echo "[3/3] Build Complete!"
echo "Collecting binaries into bin/ directory..."
mkdir -p bin
find build -name "certosc-*" -type f -executable -exec cp {} bin/ \;

echo "Binaries are located in the bin/ directory."
echo "You can run the cluster using ./scripts/run_cluster.sh"
