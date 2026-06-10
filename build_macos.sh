#!/usr/bin/env bash
# build_macos.sh — PANZI macOS Universal build (arm64 + x86_64)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

if [ ! -f "JUCE/CMakeLists.txt" ]; then
    echo "Initializing JUCE submodule..."
    git submodule update --init --recursive
fi

cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13

cmake --build build --config Release --parallel

echo ""
echo "Build complete."
echo "VST3 → ~/Library/Audio/Plug-Ins/VST3/PANZI.vst3"
echo "AU   → ~/Library/Audio/Plug-Ins/Components/PANZI.component"
