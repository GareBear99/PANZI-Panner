#!/usr/bin/env bash
# build_linux.sh — PANZI Linux build (Debian/Ubuntu)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Install JUCE dependencies
sudo apt-get update -qq
sudo apt-get install -y \
    libasound2-dev libjack-jackd2-dev libfreetype6-dev \
    libx11-dev libxcomposite-dev libxcursor-dev libxext-dev \
    libxfixes-dev libxinerama-dev libxrandr-dev libxrender-dev \
    libwebkit2gtk-4.0-dev libglu1-mesa-dev mesa-common-dev \
    cmake g++ ninja-build

if [ ! -f "JUCE/CMakeLists.txt" ]; then
    git submodule update --init --recursive
fi

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build --parallel

echo ""
echo "Build complete. Copy build/PANZI_artefacts/Release/VST3/PANZI.vst3 to ~/.vst3/"
