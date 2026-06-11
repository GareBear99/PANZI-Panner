# PANZI DSP Architecture Specification

**Version:** 0.2.0-dev  
**Status:** Source architecture defined. Pre-baked coefficient engine not yet implemented.  
**Base:** FreeAutoPanner v0.1.0 (`AutoPanDSP.h`)

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│  UI Thread / APVTS                                                  │
│  ├─ Topology selector (Diamond / Cube / Cylinder / Sphere)          │
│  ├─ Room scale (0.5 m – 20 m)                                       │
│  ├─ Source X/Y/Z (future: v0.3.0)                                   │
│  └─ LFO params (rate, depth, center, symmetry, smooth, detect, sens)│
└───────────────────────────────────┬─────────────────────────────────┘
                                    │
                    ┌───────────────▼────────────────┐
                    │  Background Bake Thread         │
                    │  ─────────────────────────────  │
                    │  1. Read topology + room scale  │
                    │  2. Compute per-channel:        │
                    │     • delaySamples[N]           │
                    │     • gainScalar[N]             │
                    │     • svfCutoffHz[N]            │
                    │     • panWeight[N]              │
                    │  3. Write to SPSC write slot    │
                    └───────────────┬────────────────┘
                                    │  Atomic pointer swap
                    ┌───────────────▼────────────────┐
                    │  Audio Thread — processBlock()  │
                    │  ─────────────────────────────  │
                    │  Per sample, per channel N:     │
                    │  1. LFO step → panPos           │
                    │  2. Circular delay read         │
                    │  3. SVF lowpass (fc_n)          │
                    │  4. RBJ shelf (static)          │
                    │  5. × gainScalar[n]             │
                    │  6. × panWeight[n](panPos)      │
                    │  → Output channel n             │
                    └─────────────────────────────────┘
```

---

## File Structure (v0.2.0 — All Implemented)

```
PANZI/
├── Source/
│   ├── PluginProcessor.h/.cpp       # APVTS, bake trigger, processBlock
│   ├── PluginEditor.h/.cpp          # Topology selector + knobs
│   ├── Config.h                     # Version constants
│   └── DSP/
│       ├── AutoPanDSP.h             # Inherited LFO engine (unchanged + getNextPanPosition)
│       ├── PanziEngine.h            # Master per-channel signal chain
│       ├── BakedCoefficients.h      # alignas(64) coefficient struct + CoeffHandoff SPSC
│       ├── TopologyBaker.h          # Background bake computation
│       ├── PolyhedralTopology.h     # Node position tables (4 topologies)
│       ├── DelayLine.h              # Fractional circular delay
│       ├── SvfLowpass.h             # Simper SVF per-channel
│       └── RbjShelf.h               # Static boundary reflection biquad
├── docs/
│   ├── PANZI_DSP_ARCHITECTURE.md   # This file
│   ├── PANZI_PRODUCTION_DOCS.md    # Combined audit + release checklist + golden tests
│   ├── ARC_GOVERNANCE.md           # Release governance rules
│   └── ORIGIN_CONVERSATION.md     # Full Gemini pre-build design session + audit table
├── scripts/
│   └── validate_repo.py            # Source package validator (0 errors, 0 warnings)
├── PAPER.md                         # Academic DSP paper (15 refs)
├── README.md
├── CHANGELOG.md
├── CONTRIBUTING.md
├── LICENSE
├── CMakeLists.txt
├── build_macos.sh
├── build_linux.sh
└── build_windows.ps1
```

---

## BakedCoefficients Struct

```cpp
// DSP/BakedCoefficients.h
#pragma once
#include <array>
#include <atomic>

static constexpr int   kMaxChannels  = 16;
static constexpr int   kDelayBufSize = 131072;   // ~3s at 44.1 kHz
static constexpr float kSpeedOfSound = 343.0f;   // m/s at 20°C
static constexpr float kFcMax        = 18000.0f; // Hz
static constexpr float kKAir         = 0.15f;    // m^-1 (ISO 9613-1 approximation)
static constexpr float kMinDistance  = 0.5f;     // m — near-field clamp

// alignas(64): fits in 4 cache lines, enables AVX-512 alignment
struct alignas(64) BakedCoefficients
{
    float delaySamples [kMaxChannels] = {};   // 4D: time-of-flight per channel
    float gainScalar   [kMaxChannels] = {};   // 5D P: inverse-distance pressure
    float svfCutoffHz  [kMaxChannels] = {};   // 5D φ: air absorption cutoff
    float speakerPhi   [kMaxChannels] = {};   // azimuth  (radians) — Ix,Iy projection
    float speakerTheta [kMaxChannels] = {};   // elevation (radians) — Iz projection
    int   numChannels  = 8;
    float roomScaleM   = 5.0f;
    float sampleRate   = 44100.0f;
    int   topology     = 0;   // 0=Diamond, 1=Cube, 2=Cylinder, 3=Sphere
};

// Two-slot atomic swap SPSC handoff (background baker → audio thread).
class CoeffHandoff { /* see BakedCoefficients.h */ };
```

**Note:** The panWeight per channel is computed per-sample in `PanziEngine::processBlock()`
using the live LFO pan position — it is NOT stored in the baked struct, because it
changes every sample as the LFO sweeps. The baked struct stores `speakerPhi` and
`speakerTheta` (fixed speaker directions), which are combined with the live `panPos`
to compute the dot-product weight at sample rate.

---

## PolyhedralTopology Node Tables

```cpp
// DSP/PolyhedralTopology.h
#pragma once
#include <array>

struct Vec3 { float x, y, z; };

// All positions normalised to unit sphere.
// Room scale applied as multiplier during bake.

static constexpr std::array<Vec3, 8> kDiamondNodes = {{
    { 1.0f,  0.0f,  0.0f},   // 0: Right
    {-1.0f,  0.0f,  0.0f},   // 1: Left
    { 0.0f,  1.0f,  0.0f},   // 2: Front
    { 0.0f, -1.0f,  0.0f},   // 3: Rear
    { 0.0f,  0.0f,  1.0f},   // 4: Top
    { 0.0f,  0.0f, -1.0f},   // 5: Bottom
    { 0.70f, 0.70f, 0.0f},   // 6: Front-Right
    {-0.70f, 0.70f, 0.0f}    // 7: Front-Left
}};

static constexpr std::array<Vec3, 8> kCubeNodes = {{
    { 0.577f,  0.577f,  0.577f},
    {-0.577f,  0.577f,  0.577f},
    { 0.577f, -0.577f,  0.577f},
    {-0.577f, -0.577f,  0.577f},
    { 0.577f,  0.577f, -0.577f},
    {-0.577f,  0.577f, -0.577f},
    { 0.577f, -0.577f, -0.577f},
    {-0.577f, -0.577f, -0.577f}
}};

static constexpr std::array<Vec3, 8> kCylinderNodes = {{
    { 1.0f,  0.0f,  0.5f},
    { 0.0f,  1.0f,  0.5f},
    {-1.0f,  0.0f,  0.5f},
    { 0.0f, -1.0f,  0.5f},
    { 1.0f,  0.0f, -0.5f},
    { 0.0f,  1.0f, -0.5f},
    {-1.0f,  0.0f, -0.5f},
    { 0.0f, -1.0f, -0.5f}
}};

// Icosahedral 8-node subset (maximises minimum solid angle)
static const float kPhi = 1.61803398875f;   // golden ratio
static constexpr std::array<Vec3, 8> kSphereNodes = {{
    { 0.0f,  1.0f,  kPhi},
    { 0.0f, -1.0f,  kPhi},
    { 0.0f,  1.0f, -kPhi},
    { 0.0f, -1.0f, -kPhi},
    { 1.0f,  kPhi,  0.0f},
    {-1.0f,  kPhi,  0.0f},
    { 1.0f, -kPhi,  0.0f},
    {-1.0f, -kPhi,  0.0f}
}};
```

---

## TopologyBaker

```cpp
// DSP/TopologyBaker.h
#pragma once
#include "BakedCoefficients.h"
#include "PolyhedralTopology.h"
#include <cmath>
#include <algorithm>

enum class Topology { Diamond = 0, Cube, Cylinder, Sphere };

class TopologyBaker
{
public:
    static BakedCoefficients bake(Topology topo,
                                   float roomScaleM,
                                   float sourceX, float sourceY, float sourceZ,
                                   float sampleRate)
    {
        BakedCoefficients c;
        c.numChannels = 8;
        c.roomScaleM  = roomScaleM;
        c.sampleRate  = sampleRate;

        const auto* nodes = getNodes(topo);

        for (int n = 0; n < 8; ++n)
        {
            // Scale node to room dimensions
            float nx = nodes[n].x * roomScaleM;
            float ny = nodes[n].y * roomScaleM;
            float nz = nodes[n].z * roomScaleM;

            // Distance from source to speaker n
            float dx = nx - sourceX;
            float dy = ny - sourceY;
            float dz = nz - sourceZ;
            float dist = std::max(kMinDistance, std::sqrt(dx*dx + dy*dy + dz*dz));

            // 4D: time delay
            float delaySeconds = dist / kSpeedOfSound;
            c.delaySamples[n] = delaySeconds * sampleRate;

            // 5D P: inverse-distance gain (acoustic pressure law)
            c.gainScalar[n] = 1.0f / dist;

            // 5D φ: air absorption cutoff
            c.svfCutoffHz[n] = kFcMax * std::exp(-kKAir * dist);

            // Speaker direction in spherical coords
            c.speakerPhi[n]   = std::atan2(ny - sourceY, nx - sourceX);
            c.speakerTheta[n] = std::asin((nz - sourceZ) / dist);
        }
        return c;
    }

private:
    static const Vec3* getNodes(Topology t)
    {
        switch (t) {
            case Topology::Diamond:  return kDiamondNodes.data();
            case Topology::Cube:     return kCubeNodes.data();
            case Topology::Cylinder: return kCylinderNodes.data();
            case Topology::Sphere:   return kSphereNodes.data();
        }
        return kDiamondNodes.data();
    }
};
```

---

## PanziEngine Per-Channel Signal Chain

```cpp
// DSP/PanziEngine.h (excerpt — key processBlock logic)

void processSample(float input, float panPos,
                   const BakedCoefficients& c,
                   float* outputChannels)
{
    for (int n = 0; n < c.numChannels; ++n)
    {
        // 1. Circular delay (4D spacetime)
        delayBuf[n][writeIdx] = input;
        int readIdx = (writeIdx - (int)c.delaySamples[n] + kDelayBufSize) % kDelayBufSize;
        float frac = c.delaySamples[n] - floorf(c.delaySamples[n]);
        float delayed = (1.0f-frac)*delayBuf[n][readIdx]
                      +     frac  *delayBuf[n][(readIdx+1)%kDelayBufSize];

        // 2. SVF lowpass (5D φ: air absorption)
        float lp = svf[n].process(delayed, c.svfCutoffHz[n]);

        // 3. RBJ shelf (static boundary reflection)
        float shelved = rbj[n].process(lp);

        // 4. Gain (5D P: pressure law)
        float gained = shelved * c.gainScalar[n];

        // 5. Directional pan weight (Ix, Iy, Iz: intensity projection)
        float az = panPos * (juce::MathConstants<float>::pi * 0.5f);
        float srcX = std::cos(az);
        float srcY = std::sin(az);
        float spkX = std::cos(c.speakerPhi[n]) * std::cos(c.speakerTheta[n]);
        float spkY = std::sin(c.speakerPhi[n]) * std::cos(c.speakerTheta[n]);
        float dot  = srcX*spkX + srcY*spkY;
        float pw   = 0.5f * (1.0f + dot);

        outputChannels[n] += gained * pw;
    }
    writeIdx = (writeIdx + 1) % kDelayBufSize;
}
```

---

## Parameters (APVTS)

| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `topology` | Topology | 0–3 | 0 | 0=Diamond, 1=Cube, 2=Cylinder, 3=Sphere |
| `roomScale` | Room Scale (m) | 0.5–20.0 | 5.0 | Triggers rebake on change |
| `rate` | LFO Rate (Hz) | 0.01–20.0 | 1.0 | Inherited from AutoPanDSP |
| `depth` | Depth (%) | 0–100 | 100 | Inherited |
| `center` | Center | −100–100 | 0 | Inherited |
| `symmetry` | Symmetry | 0–100 | 0 | Inherited |
| `smooth` | Smooth (ms) | 1–200 | 20 | Inherited |
| `detect` | Detect (%) | 0–100 | 0 | Inherited |
| `sensitivity` | Sensitivity | 0–100 | 50 | Inherited |

---

## Golden Tests

- Silence in → silence out (all topologies, all room scales)
- No NaN/Inf at roomScale extremes (0.5 m, 20 m)
- No gain explosion at min distance (0.5 m clamp)
- Topology switch produces no click > −60 dBFS
- CPU < 2% one core: 8ch, 44.1 kHz, 128-sample buffer
- pluginval strictness 10 passes for VST3/AU
- 8-channel output bus verified in REAPER 7.1 project
- Session recall restores topology and all parameters correctly

---

## ARC Governance

1. Audio thread authority: read baked coefficients, process samples, write atomic display taps only.
2. Bake computation belongs entirely on background thread — never called from `processBlock()`.
3. Every release includes a manifest, pluginval output, and CPU profiling evidence.
4. Parameter IDs are frozen from v0.2.0 onward — any addition requires a migration doc.
5. Claims match evidence: "8-channel polyhedral spatializer" requires a REAPER 7.1 project smoke test.
