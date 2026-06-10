#!/usr/bin/env python3
"""PANZI source package validator.
Run before any commit or release candidate.
Exits 0 on pass, 1 on failure.
"""
import os, sys, re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

ERRORS = []
WARNINGS = []

def check(cond, msg, warn=False):
    if not cond:
        (WARNINGS if warn else ERRORS).append(msg)

def file_exists(rel):
    return os.path.isfile(os.path.join(ROOT, rel))

def file_contains(rel, text):
    try:
        return text in open(os.path.join(ROOT, rel)).read()
    except FileNotFoundError:
        return False

# Required files
required = [
    "CMakeLists.txt",
    "PAPER.md",
    "README.md",
    "CHANGELOG.md",
    "CONTRIBUTING.md",
    "LICENSE",
    "build_macos.sh",
    "build_linux.sh",
    "build_windows.ps1",
    "Source/Config.h",
    "Source/PluginProcessor.h",
    "Source/PluginProcessor.cpp",
    "Source/PluginEditor.h",
    "Source/PluginEditor.cpp",
    "Source/DSP/AutoPanDSP.h",
    "Source/DSP/BakedCoefficients.h",
    "Source/DSP/PolyhedralTopology.h",
    "Source/DSP/TopologyBaker.h",
    "Source/DSP/DelayLine.h",
    "Source/DSP/SvfLowpass.h",
    "Source/DSP/RbjShelf.h",
    "Source/DSP/PanziEngine.h",
    "docs/PANZI_DSP_ARCHITECTURE.md",
    "docs/PANZI_PRODUCTION_DOCS.md",
    "docs/ORIGIN_CONVERSATION.md",
    "docs/ARC_GOVERNANCE.md",
    "scripts/validate_repo.py",
]

for f in required:
    check(file_exists(f), f"MISSING: {f}")

# Version consistency
def extract_version(path, pattern):
    try:
        m = re.search(pattern, open(os.path.join(ROOT, path)).read())
        return m.group(1) if m else None
    except FileNotFoundError:
        return None

cmake_ver = extract_version("CMakeLists.txt", r"project\(PANZI VERSION ([\d.]+)")
config_major = extract_version("Source/Config.h", r"PANZI_VERSION_MAJOR (\d+)")
config_minor = extract_version("Source/Config.h", r"PANZI_VERSION_MINOR (\d+)")
config_patch = extract_version("Source/Config.h", r"PANZI_VERSION_PATCH (\d+)")

config_ver = f"{config_major}.{config_minor}.{config_patch}" if all([config_major, config_minor, config_patch]) else None

check(cmake_ver == config_ver,
      f"Version mismatch: CMakeLists={cmake_ver}, Config.h={config_ver}")

# Safety checks
check(not file_contains("Source/DSP/PanziEngine.h", "std::vector"),
      "std::vector in PanziEngine (dynamic allocation risk)", warn=True)
check(file_contains("Source/PluginProcessor.cpp", "ScopedNoDenormals")
      or file_contains("Source/DSP/PanziEngine.h", "ScopedNoDenormals"),
      "ScopedNoDenormals missing from audio path", warn=True)
check(file_contains("Source/DSP/BakedCoefficients.h", "alignas(64)"),
      "BakedCoefficients missing 64-byte alignment")

# ARC governance: TopologyBaker::bake must not be in processBlock directly.
# Check that the processBlock function body does not call it.
proc_text = open(os.path.join(ROOT, "Source/PluginProcessor.cpp")).read()
# Extract processBlock body only
pb_match = re.search(r'processBlock\s*\([^)]*\)[^{]*\{(.+?)^\}', proc_text,
                      re.DOTALL | re.MULTILINE)
pb_body = pb_match.group(1) if pb_match else ""
check("TopologyBaker::bake" not in pb_body,
      "TopologyBaker::bake found inside processBlock body — move to background thread")

# Summary
print(f"\nPANZI source validation")
print(f"  Errors:   {len(ERRORS)}")
print(f"  Warnings: {len(WARNINGS)}")

for e in ERRORS:
    print(f"  [ERROR]   {e}")
for w in WARNINGS:
    print(f"  [WARN]    {w}")

if ERRORS:
    print("\n❌ FAILED — fix errors before commit.\n")
    sys.exit(1)
else:
    print("\n✅ PASSED\n")
    sys.exit(0)
