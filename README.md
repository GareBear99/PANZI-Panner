<div align="center">
  <img src="https://capsule-render.vercel.app/api?type=waving&height=200&color=0:0a0b12,50:1a0533,100:6d28d9&text=PANZI&fontSize=72&fontColor=e8eaf0&animation=fadeIn&fontAlignY=36&desc=Polyhedral%20Acoustic%20Network%20%26%20Zero-delay%20Intensity%20Engine&descSize=16&descColor=a0a6bc&descAlignY=58" width="100%">
</div>

<div align="center">
  <img src="https://img.shields.io/badge/Version-0.2.0--dev-6d28d9?style=flat" alt="Version" />
  <img src="https://img.shields.io/badge/License-GPL--3.0-3b82f6?style=flat&logo=gnu&logoColor=white" alt="GPL-3.0" />
  <img src="https://img.shields.io/badge/Formats-VST3%20%7C%20AU-6c7bbd?style=flat" alt="Formats" />
  <img src="https://img.shields.io/badge/Platform-macOS%20%7C%20Windows%20%7C%20Linux-94a3b8?style=flat" alt="Platform" />
  <img src="https://img.shields.io/badge/JUCE-7.0.12-f59e0b?style=flat" alt="JUCE 7.0.12" />
  <img src="https://img.shields.io/badge/Channels-8--channel%20polyhedral-22c55e?style=flat" alt="8-channel" />
</div>

<br>

> 🎛️ Part of the [TizWildin Plugin Ecosystem](https://garebear99.github.io/TizWildinEntertainmentHUB/) — 20+ free audio plugins.
>
> [FreeEQ8](https://github.com/GareBear99/FreeEQ8) · [FreeVox8](https://github.com/GareBear99/FreeVox8) · [Voxel Audio](https://github.com/GareBear99/Voxel_Audio) · [TizWildinEntertainmentHUB](https://github.com/GareBear99/TizWildinEntertainmentHUB)
>
> 🎧 **[All Platforms](https://ffm.bio/no4km87)** · ☁️ **[SoundCloud](https://soundcloud.com/tizwildin)** · ▶️ **[YouTube](https://www.youtube.com/@garebearproductionz)**

---

**PANZI** (Polyhedral Acoustic Network & Zero-delay Intensity engine) is a free,
open-source (GPL-3.0) multi-channel spatial audio plugin. It maps 4D/5D acoustic
vector field theory onto a pre-baked constant framework — delivering holographic
polyhedral speaker imaging at the CPU footprint of a small parallel EQ bank.

> **"Great sound shouldn't cost anything"**

## How It Works

Standard stereo panners use amplitude alone. PANZI models the physics:

For each output channel in the selected polyhedral speaker topology, PANZI
computes — once, on a background thread — the exact **time delay**, **air
absorption cutoff**, **inverse-distance gain**, and **directional intensity
weight** from the virtual source to that speaker position. These are stored as
48 raw floats. The audio thread reads those floats and executes nothing but:

```
Input → [Delay τ_n] → [SVF Lowpass fc_n] → [RBJ Shelf] → [× gain_n × panWeight_n] → Output n
```

That's it. No real-time matrix solving. No dynamic room simulation. No GPU.
The geometry does the physics work once; the DSP does the audio work at sample rate.

## Topology Presets

| Topology | Nodes | Best For |
|----------|-------|----------|
| **Diamond** (Octahedron) | 8 | Full-room holographic imaging, height panning |
| **Cube** | 8 | Control-room monitoring correction |
| **Cylinder** | 8 | Height panning, stage-style front-back imaging |
| **Sphere** (Icosahedron subset) | 8 | Maximum spatial uniformity, VR/binaural prep |

## The 5D Vector Field — Simply

Every point in the room can be described by 5 numbers. PANZI maps each one to a DSP operation:

| Dimension | Meaning | PANZI Implementation |
|-----------|---------|---------------------|
| Ix, Iy, Iz | Directional energy flux | Dot-product pan weight per speaker |
| P | Acoustic pressure | Inverse-distance gain scalar |
| φ | Phase / time-of-flight | Circular delay line + SVF cutoff |

## Architecture

Built on the FreeEQ8 real-time safety doctrine:
- **Zero heap allocation** on the audio thread
- **SPSC lock-free** coefficient handoff from background baker to audio thread
- **SVF topology** (Simper trapezoidal) for distance-modulated lowpass — stable under fast source movement
- **RBJ biquad** for static boundary reflection shelving — cheap, correct for fixed geometry
- **SmoothedValue** parameter ramping throughout — no zipper noise on LFO or room scale changes

## Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| Topology | Diamond / Cube / Cylinder / Sphere | Speaker geometry preset |
| Room Scale | 0.5 – 20 m | Physical room radius |
| Rate | 0.01 – 20 Hz | LFO sweep rate |
| Depth | 0 – 100% | Pan sweep width |
| Center | −100 – +100 | Average pan position |
| Symmetry | 0 – 100% | LFO warp (linger on one side) |
| Smooth | 1 – 200 ms | Pan smoothing |
| Detect | 0 – 100% | Level-follower depth modulation |
| Sensitivity | 0 – 100% | Detect response curve |

## 🆚 How PANZI Compares

| Feature | **PANZI** | Dolby Atmos Tools | DearVR Pro | Waves Nx |
|---------|:---------:|:-----------------:|:----------:|:--------:|
| **Price** | **Free** | $varies | $99 | $99 |
| **Open Source** | **GPL-3.0** | — | — | — |
| Physical time delay modeling | **✓** | ✓ | partial | — |
| Air absorption per distance | **✓** | — | partial | — |
| Polyhedral topology presets | **✓** | — | — | — |
| 5D intensity vector imaging | **✓** | — | — | — |
| Zero-heap audio thread | **✓** | unknown | unknown | unknown |
| DAW CPU overhead | **< 2%** | varies | varies | varies |
| Sweet-spot independence | **✓** (physical) | partial | — | — |
| Formats | VST3, AU | VST3, AU, AAX | VST3, AU, AAX | VST3, AU, AAX |

## 🔬 DSP Paper

[**PAPER.md**](PAPER.md) — Full academic architecture paper covering:
- 5D acoustic vector field theory and the polyhedral boundary condition
- Pre-baking strategy: collapsing 4D/5D field computation to static coefficients
- Per-channel DSP signal chain (delay → SVF → RBJ → gain matrix)
- Real-time safety architecture
- Known limitations and future work
- Full references (Spors/Rabenstein/Ahrens, Meissner, Bilbao, Simper, RBJ, ISO 9613-1)

## 🏗️ Status

**v0.1.0** — `FreeAutoPanner`: stereo LFO auto-panner, proven DSP foundation. ✅ Shipped.

**v0.2.0** — PANZI: polyhedral coefficient engine + 8-channel output bus. 🔧 In development.

## 🔧 Build

```bash
git submodule add https://github.com/juce-framework/JUCE.git JUCE
git -C JUCE checkout 7.0.12
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## 🛣️ Roadmap

### v0.1.0 — FreeAutoPanner (✅ Done)
- [x] Sine LFO with symmetry warp
- [x] Equal-power pan law
- [x] Level-follower dynamic depth
- [x] SmoothedValue parameter ramping
- [x] APVTS session save/restore

### v0.2.0 — PANZI Core (🔧 Source complete, binary certification pending)
- [x] Polyhedral topology node tables (Diamond, Cube, Cylinder, Sphere)
- [x] TopologyBaker: background-thread coefficient computation
- [x] SPSC atomic coefficient handoff (CoeffHandoff)
- [x] 8-channel output bus (JUCE 7.1 channel set)
- [x] Per-channel: circular delay + SVF lowpass + RBJ shelf + gain matrix
- [x] Topology selector parameter
- [x] Room scale parameter (0.5–20 m)
- [x] PanziEngine.h — full signal chain implemented
- [x] All DSP headers: BakedCoefficients, DelayLine, SvfLowpass, RbjShelf, TopologyBaker, PolyhedralTopology
- [ ] pluginval strictness-10 pass (requires local JUCE build)
- [ ] REAPER 7.1 surround project smoke test
- [ ] CPU profiling evidence (< 2% target)
- [ ] Factory preset suite (8 presets)
- [ ] Public binary release

### v0.3.0 — SIMD + 3D Source Trajectory
- [ ] AVX2/SSE2 SoA vectorisation of N-channel gain matrix
- [ ] Independent azimuth + elevation LFOs
- [ ] DAW automation for source X/Y/Z

### v0.4.0 — Wall Impedance Model
- [ ] Per-material frequency-dependent absorption (ISO 354 data)
- [ ] Material selector preset per room face

### v0.5.0 — Dolby Atmos 9.1.6 Bus
- [ ] 16-channel output bus
- [ ] ITU-R BS.2051 System G topology preset

### v0.6.0 — HOA Output
- [ ] 3rd-order Ambisonic B-format encoder
- [ ] HRTF binaural render option

## 📝 License

GNU General Public License v3.0.

## 🏷️ GitHub Setup

After pushing, add these topics to the repo for discoverability:

```
audio-plugin  vst3  au  spatial-audio  juce  cpp  dsp  surround-sound  open-source  cpp17
```

Tag the FreeAutoPanner foundation release:

```bash
git tag -a v0.1.0 -m "FreeAutoPanner — stereo LFO auto-panner, proven DSP foundation"
git push origin v0.1.0
```

## 🙏 Acknowledgments

- **Robert Bristow-Johnson** — RBJ Audio EQ Cookbook [ref 3]
- **Andy Simper (Cytomic)** — SVF trapezoidal topology [ref 2]
- **FreeEQ8 architecture** — allocation-free, lock-free audio doctrine [ref 1]
- **Spors, Rabenstein & Ahrens** — Wave Field Synthesis boundary theory [ref 4]
- **Meissner; Kotus & Kostek** — 5D acoustic intensity vector field framework [refs 5, 6]

## 💖 Support

<a href="https://github.com/sponsors/GareBear99"><img src="https://img.shields.io/badge/sponsor-GitHub%20Sponsors-ea4aaa?logo=githubsponsors&style=for-the-badge" alt="GitHub Sponsors"></a>
<a href="https://buymeacoffee.com/garebear99"><img src="https://img.shields.io/badge/Buy%20Me%20a%20Coffee-ffdd00?logo=buy-me-a-coffee&logoColor=black&style=for-the-badge" alt="Buy Me a Coffee"></a>

---

**Built with ❤️ by Gary Doman (GareBear99 / TizWildin)**

*"Great sound shouldn't cost anything"*

<p align="center">
  <img src="https://capsule-render.vercel.app/api?type=waving&section=footer&height=140&color=0:0a0b12,50:1a0533,100:6d28d9" alt="footer" />
</p>
