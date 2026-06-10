# PANZI Changelog

## 0.2.0-dev — PANZI Core Engine (In Development)

- Renamed project from FreeAutoPanner → PANZI (Polyhedral Acoustic Network & Zero-delay Intensity engine)
- Added `PAPER.md` — full academic DSP architecture paper: 5D vector field theory, pre-baking strategy, per-channel signal chain, real-time safety architecture, complete references
- Added `docs/PANZI_DSP_ARCHITECTURE.md` — implementation spec: file structure, BakedCoefficients struct, PolyhedralTopology node tables, TopologyBaker, PanziEngine signal chain, parameter table, golden tests, ARC governance
- Added `docs/PANZI_PRODUCTION_AUDIT.md`, `PANZI_RELEASE_CHECKLIST.md`, `PANZI_GOLDEN_TESTS.md`
- Defined four topology presets: Diamond (Octahedron), Cube, Cylinder, Sphere (icosahedron subset)
- Defined 5D → DSP mapping: Ix/Iy/Iz → directional pan weight; P → inverse-distance gain; φ → delay + SVF cutoff
- Architecture: background baker → SPSC atomic handoff → audio thread reads 48 raw floats
- SVF lowpass (Simper trapezoidal) for per-channel air absorption — stable under source movement
- RBJ biquad shelving for static boundary reflection coloring
- Fractional circular delay line per channel (sub-sample interpolation)
- Retained `AutoPanDSP.h` LFO engine intact as source position modulator
- 8-channel output bus target (REAPER 7.1 surround project)
- Rewrote README with full technical description, comparison table, roadmap

## 0.1.0 — FreeAutoPanner (2026-01-29)

- Initial release: stereo auto-panner VST3/AU
- Sine LFO (0.01–20 Hz) with symmetry warp (skew power curve, exponent 1–9)
- Equal-power pan law: gL = cos(θ), gR = sin(θ)
- Level-follower dynamic depth via `juce::dsp::BallisticsFilter` (attack 10ms, release 150ms)
- `SmoothedValue<Linear>` pan smoothing (1–200 ms)
- 7 parameters via APVTS: rate, depth, center, symmetry, smooth, detect, sensitivity
- Session save/restore via APVTS XML serialisation
- VST3 + AU, macOS/Windows/Linux, JUCE 7.0.12
