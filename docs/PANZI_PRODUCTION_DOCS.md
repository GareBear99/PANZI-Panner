# PANZI Production Docs

---

## PANZI_PRODUCTION_AUDIT.md

**v0.2.0** is a source architecture definition. The DSP specification, academic paper, topology node tables, BakedCoefficients struct, and signal chain are defined. Implementation of the multi-channel engine in C++ is the active work item.

### What is defined

- BakedCoefficients struct (alignas 64, 48 floats for 8 channels)
- PolyhedralTopology node tables (Diamond, Cube, Cylinder, Sphere)
- TopologyBaker background-thread coefficient computation
- PanziEngine per-channel signal chain (delay → SVF → RBJ → gain × panWeight)
- SPSC atomic coefficient handoff pattern (from FreeEQ8)
- Full APVTS parameter set
- Academic paper with complete references

### What is not yet implemented

- `PanziEngine.h` C++ implementation
- `TopologyBaker.h` C++ implementation
- `DelayLine.h` fractional circular delay
- `SvfLowpass.h` per-channel (can be ported from FreeEQ8 `SvfBiquad.h`)
- `RbjShelf.h` static boundary reflection biquad
- 8-channel output bus JUCE configuration in `CMakeLists.txt`
- Topology + roomScale parameters wired to APVTS
- pluginval run
- DAW 7.1 smoke test

### Remaining certification gates before v0.2.0 public binary

1. Implement all DSP headers per `PANZI_DSP_ARCHITECTURE.md`
2. Configure JUCE 8-channel output bus in `CMakeLists.txt`
3. Wire topology + roomScale parameters to bake trigger
4. Compile on macOS Universal, Linux x86_64, Windows x64
5. pluginval strictness-10 pass VST3 + AU
6. REAPER 7.1 surround project smoke test (8-channel output verified)
7. Logic Pro AU scan + load
8. Ableton Live VST3 load (note: Ableton does not natively route 8-channel plugins to surround — REAPER is the primary 8ch DAW target)
9. CPU profiling: < 2% one core, 8ch, 44.1 kHz, 128-sample buffer
10. Golden tests pass (see below)
11. Release evidence pack

---

## PANZI_RELEASE_CHECKLIST.md

### Source gate
- [ ] `cmake -S . -B build` succeeds with JUCE submodule
- [ ] VST3 builds on macOS, Linux, Windows
- [ ] AU builds on macOS
- [ ] No FreeAutoPanner product name remaining (CMakeLists, plugin identity)
- [ ] README version matches `Config.h` and `CMakeLists.txt`
- [ ] CHANGELOG updated

### Audio gate
- [ ] Bypass is click-free
- [ ] Topology switch: no click > −60 dBFS
- [ ] Room scale change: no click > −60 dBFS
- [ ] No NaN/Inf at parameter extremes
- [ ] No gain explosion at roomScale = 0.5 m
- [ ] No heap allocation in audio callback (verified with ASan)
- [ ] SmoothedValue ramps on all parameters

### Host gate
- [ ] pluginval strictness-10 pass (VST3)
- [ ] REAPER 7.1 surround project: all 8 output channels active
- [ ] Ableton Live VST3 load (stereo downmix acceptable for initial test)
- [ ] Logic Pro AU scan + load
- [ ] Standalone launches (if built)

### Public release gate
- [ ] CPU profiling evidence (< 2% one core at 128-sample buffer)
- [ ] Factory preset suite (8 presets minimum)
- [ ] Session recall verified (all parameters restore after project reload)
- [ ] Release notes finalized
- [ ] PAPER.md accurate to shipped implementation

---

## PANZI_GOLDEN_TESTS.md

### Must pass

- Silence in → silence out (all 4 topologies, roomScale 0.5 m and 20 m)
- No NaN/Inf at any parameter extreme
- Topology switch (Diamond → Cube → Cylinder → Sphere → Diamond): no click > −60 dBFS, no state corruption
- Room scale sweep (0.5 → 20 m, 1 Hz LFO): gain remains bounded [0, 2]
- Min distance clamp: roomScale = 0.5 m, source at center — no gain explosion
- Host bypass: bit-stable dry signal returned
- Sample rates: 44.1, 48, 88.2, 96, 192 kHz — no crash, no parameter change
- Buffer sizes: 1, 16, 32, 64, 128, 512, 1024, 2048 samples — no crash
- pluginval strictness 10: VST3 + AU

### Musical acceptance

- Diamond topology: LFO sweep at 0.5 Hz produces clear front-rear-height sweep on 7.1 monitor bus
- Cube topology: corner imaging stable, no phase cancellation dropout on diagonal axes
- Sphere topology: most spatially uniform — no single dead zone at any LFO phase
- Detect=100%, Sensitivity=100%, loud signal: depth modulation audible and bounded

### ARC governance verification

- Audio thread allocates zero bytes during processBlock (verified with custom allocator hook or ASan)
- No mutex acquisition on audio thread
- Bake computation never called from processBlock
- SPSC swap completes in O(1) time (verified by inspection)
