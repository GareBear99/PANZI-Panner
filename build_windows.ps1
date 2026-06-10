# build_windows.ps1 — PANZI Windows build
$ErrorActionPreference = "Stop"

if (-not (Test-Path "JUCE\CMakeLists.txt")) {
    git submodule update --init --recursive
}

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel

Write-Host ""
Write-Host "Build complete."
Write-Host "Copy build\PANZI_artefacts\Release\VST3\PANZI.vst3 to C:\Program Files\Common Files\VST3\"
