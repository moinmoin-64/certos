#!/bin/bash
# CertOS GitHub Update Script
# Usage: ./certos-update.sh [repo_url] [branch]

REPO_URL=${1:-"https://github.com/moinmoin-64/certos.git"}
BRANCH=${2:-main}
DEST_DIR="/usr/local/bin"

echo "Checking for CertOS updates from $REPO_URL [$BRANCH]..."

# Create temp dir
TMP_DIR=$(mktemp -d)
cd $TMP_DIR

# Clone only the latest commit to save time/space
git clone --depth 1 -b $BRANCH $REPO_URL .

if [ $? -ne 0 ]; then
    echo "Error: Failed to reach GitHub repository."
    exit 1
fi

# Build logic (Simplified for now - assumes pre-built binaries or quick build)
echo "Building latest components..."
# In a real ISO, we might download pre-built release artifacts instead of building
# But for now, we'll simulate a build
mkdir build && cd build
cmake .. && make -j$(nproc)

if [ $? -eq 0 ]; then
    echo "Update successful! Installing..."
    sudo systemctl stop certos-gateway certos-master certos-agent
    sudo cp bin/certosc-* $DEST_DIR/
    sudo systemctl start certos-gateway certos-master certos-agent
    echo "CertOS services restarted with new version."
else
    echo "Update failed during build process."
fi

rm -rf $TMP_DIR
