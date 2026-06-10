# Contributing to PANZI

Thank you for your interest in contributing. PANZI is a production-grade spatial
audio plugin — contributions must meet the same real-time safety standards as
FreeEQ8 and FreeVox8.

## Before You Start

Read `PAPER.md` and `docs/PANZI_DSP_ARCHITECTURE.md`. Understand the ARC
governance rules before touching any DSP code.

## ARC Governance Rules (Non-Negotiable)

1. **Audio thread authority is narrow:** read baked coefficients, process
   samples, write atomic display taps. Nothing else.
2. **No heap allocation on the audio thread.** No `new`, no `std::vector`
   growth, no file I/O, no mutex acquisition in `processBlock`.
3. **Baking belongs on the background thread.** `TopologyBaker::bake()` must
   never be called from `processBlock` or any code path reachable from it.
4. **Parameter IDs are frozen** from v0.2.0 onward. Adding a parameter requires
   a migration doc; renaming one is a breaking change.
5. **Claims must match evidence.** If you add a feature, add a golden test for
   it in `docs/PANZI_PRODUCTION_DOCS.md`.

## Development Workflow

```bash
# 1. Fork and clone
git clone https://github.com/YOUR_USERNAME/PANZI.git
cd PANZI

# 2. Add JUCE submodule
git submodule add https://github.com/juce-framework/JUCE.git JUCE
git -C JUCE checkout 7.0.12

# 3. Validate source package
python3 scripts/validate_repo.py

# 4. Build
./build_macos.sh   # or build_linux.sh / build_windows.ps1

# 5. Make your changes

# 6. Validate again before PR
python3 scripts/validate_repo.py
```

## PR Requirements

- `scripts/validate_repo.py` passes clean (0 errors, 0 warnings)
- No new heap allocation on the audio thread
- If DSP changes: describe what changed and why in the PR description
- If new parameters: update `CHANGELOG.md` and `docs/PANZI_PRODUCTION_DOCS.md`
- If new topology: add node table to `PolyhedralTopology.h` and golden test

## Areas for Contribution

- 🔊 Additional topology presets (ITU-R BS.2051 layouts)
- ⚡ SIMD vectorisation of the N-channel gain matrix (SoA + AVX2)
- 🎛️ 3D source trajectory (elevation LFO, DAW automation)
- 🧪 Golden tests and CPU profiling evidence
- 📚 Documentation improvements
- 🐛 Bug fixes

## Code Style

- C++17, same style as `FreeEQ8/Source/DSP/`
- `alignas(64)` on all per-channel data structs
- `noexcept` on all inner-loop functions
- No STL containers in hot path (use `std::array` with fixed sizes)

## Contact

- Issues: [GitHub Issues](https://github.com/GareBear99/PANZI/issues)
- Email: neovectr.inc@gmail.com
