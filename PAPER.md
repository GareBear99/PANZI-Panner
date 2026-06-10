# PANZI: Polyhedral Acoustic Network & Zero-delay Intensity Engine
## A Pre-Baked Coefficient Architecture for Real-Time 4D/5D Spatial Audio in JUCE/C++

**Gary Doman** (GareBear99 / TizWildin)  
TizWildin Entertainment / FreeEQ8 Open-Source DSP Project  
https://github.com/GareBear99

---

## Abstract

This paper presents the architecture of **PANZI** (Polyhedral Acoustic Network &
Zero-delay Intensity engine), a spatial audio plugin that maps 4D/5D acoustic
vector field theory onto a pre-baked constant framework derived from the
thread-hardened DSP architecture of FreeEQ8 [1]. Rather than executing a
real-time multidimensional finite-difference grid — which imposes prohibitive
CPU cost — PANZI bakes the geometric boundary conditions of a user-selected
polyhedral loudspeaker topology (Diamond/Octahedron, Cube, Cylinder, Sphere) into
static per-channel coefficients on a background thread. The audio thread then
executes nothing beyond a parallel bank of circular delay taps, RBJ biquad
boundary-reflection filters [3], and Simper SVF distance-tracking lowpass
filters [2], driven by those baked constants. The LFO modulation engine is
inherited directly from the existing FreeAutoPanner DSP (`AutoPanDSP.h`), which
implements equal-power panning with symmetry warp, level-follower-driven dynamic
depth, and JUCE `SmoothedValue` parameter ramping.

The result is a holographic multi-channel spatializer that delivers 4D/5D
acoustic vector field imaging at a CPU footprint equivalent to a small parallel
EQ bank — running on commodity DAW hardware without GPU acceleration, dedicated
hardware, or offline pre-computation.

A known limitation is that the current LFO modulation source is sinusoidal and
stereo-only. Extension to arbitrary 3D source trajectory automation, N-channel
polyhedral output buses, and higher-order Ambisonic decoding is designated as
primary future work.

---

## 1. Introduction

### 1.1 The Gap in Spatial Audio

Commercial spatial audio processors fall into two categories: simple stereo
panners with amplitude and delay (Dolby Atmos panning tools, Waves Nx, DearVR),
and heavyweight wave-field synthesis engines requiring dedicated hardware clusters
or offline rendering (Wave Field Synthesis arrays at the Fraunhofer Institute,
Higher-Order Ambisonics decoders in research contexts).

The first category sacrifices physical accuracy. Standard amplitude panning
produces a frequency-dependent phantom image that collapses outside a narrow
sweet spot and ignores spatial intensity flux vectors entirely. The second
category is computationally intractable for real-time DAW plugin use: a
finite-difference acoustic grid resolving up to 10 kHz in a standard room
requires grid spacing ≤17 mm, producing hundreds of thousands of nodes each
running a state-space solver at 44,100 Hz [7].

No existing plugin bridges this gap. PANZI is the first implementation to treat
the polyhedral loudspeaker topology as a **coefficient template** — computing the
physics geometry once on a background thread and reducing it to raw floats
(delay taps, gain scalars, filter cutoffs) that the audio thread consumes at
sample rate using proven RBJ/SVF infrastructure.

### 1.2 Relationship to FreeEQ8 Architecture

PANZI inherits the allocation-free, lock-free audio doctrine from FreeEQ8 [1]:

- All buffers pre-allocated in `prepareToPlay()`, zero heap on audio thread
- SPSC lock-free ring buffer for UI→audio-thread coefficient handoff
- Inline unrolled processing loops, no virtual function dispatch on audio path
- `SmoothedValue` parameter ramping throughout (no zipper noise)
- Pluginval strictness-10 target for public binary release

### 1.3 The FreeAutoPanner Foundation

The existing `FreeAutoPanner` v0.1.0 (`AutoPanDSP.h`) provides:

- Sine LFO with frequency `rateHz` (0.01–20 Hz)
- Symmetry warp via skew power curve: `uSkew = 0.5 × (2u)^k` for `k ∈ [1,9]`
- Equal-power pan law: `gL = cos(θ)`, `gR = sin(θ)`, `θ = (pan+1)/4 × π`
- Level-follower dynamic depth: `dynDepth = depth × (1 + detect × env01)`
- `juce::dsp::BallisticsFilter` envelope follower (attack 10 ms, release 150 ms)
- `SmoothedValue<Linear>` pan smoothing (1–200 ms)

This engine is retained intact as the **source position modulator** in PANZI.
The stereo L/R output is replaced by an N-channel polyhedral gain matrix derived
from the baked coefficient set.

---

## 2. Theoretical Foundation

### 2.1 The 5D Acoustic Vector Field

At any point in a room, the acoustic state is fully described by a 5-dimensional
vector [5][6]:

```
V(x,y,z,t) = ( Ix, Iy, Iz, P, φ )
```

Where:

| Dimension | Symbol | Physical Meaning |
|-----------|--------|-----------------|
| 1 | Ix | Acoustic intensity flux — x axis (W/m²) |
| 2 | Iy | Acoustic intensity flux — y axis (W/m²) |
| 3 | Iz | Acoustic intensity flux — z axis (W/m²) |
| 4 | P  | Acoustic pressure scalar (Pa) |
| 5 | φ  | Phase / time-of-flight offset (radians) |

The intensity vector `(Ix, Iy, Iz)` is the active acoustic intensity field —
the directional energy flux defined by `I = p·u` where `p` is the pressure and
`u` is the particle velocity vector [5]. Measuring or modeling this field inside
an enclosure reveals energy vortices, saddle points, and spatial interference
nulls determined by the boundary geometry and wall impedances.

The 4D spacetime dimension arises from the finite propagation speed of sound
(c ≈ 343 m/s at 20°C). A sound source at position `(xs, ys, zs)` reaches a
boundary node at `(xn, yn, zn)` after a time delay:

```
τ = √[(xn−xs)² + (yn−ys)² + (zn−zs)²] / c
```

This delay is the 4th coordinate — the `t` axis of the 4D point `(x,y,z,τ)`.

### 2.2 The Polyhedral Boundary Condition

Wave Field Synthesis theory [4] establishes that an arbitrary closed loudspeaker
array boundary can physically reconstruct an acoustic wavefront inside the
enclosed volume if the driving functions satisfy the Kirchhoff-Helmholtz integral
equation. For a discrete, finite loudspeaker array, this requires:

1. Speaker positions that form a geometrically regular boundary
2. Per-speaker driving functions that account for distance, angle, and phase
3. High-frequency spatial aliasing controlled by speaker density

The PANZI approach does not attempt full WFS reconstruction (which requires
continuous boundaries with hundreds of speakers). Instead, it uses the polyhedral
geometry to compute **accurate directional gain weights and time delays** for a
small number of output channels (8–16), producing a perceptually convincing
spatial image without the sweet-spot limitation of standard amplitude panning.

The four topology presets are:

| Topology | Nodes | Description |
|----------|-------|-------------|
| Diamond (Octahedron) | 8 | 4 equatorial + 2 height (top/bottom) + 2 front/rear |
| Cube | 8 | 8 corner nodes at ±1 in all axes |
| Cylinder | 8 | 4 floor + 4 ceiling, azimuth-uniform |
| Sphere | 8 | Vertices of a regular icosahedron subset (uniform solid angle) |

All topologies are normalised to a unit sphere. The actual room scale is applied
as a linear multiplier to the delay computation.

### 2.3 The Pre-Baking Strategy

The key insight is that the 5D vector field computation — which requires solving
the acoustic diffusion equation across the entire room at every sample — can be
**collapsed to a one-time coefficient calculation** for any static source position
and room geometry.

For each output channel `n` at speaker position `(xn, yn, zn)` and a source
at `(xs, ys, zs)`:

```
distance_n = √[(xn−xs)² + (yn−ys)² + (zn−zs)²]

τ_n   = distance_n / c                     // Time delay (seconds)
d_n   = τ_n × sampleRate                   // Delay (samples, circular buffer index)
g_n   = 1 / max(distance_n, 0.001)         // Inverse-distance gain (pressure law)
fc_n  = fc_max × exp(−k × distance_n)      // Distance-dependent air absorption cutoff
φ_n   = atan2(yn−ys, xn−xs)               // Azimuth (for directivity weighting)
θ_n   = asin((zn−zs) / distance_n)        // Elevation (for directivity weighting)
```

The directional intensity weight (Ix, Iy, Iz component projection onto speaker
direction) is:

```
w_n = cos(φ_n) × cos(θ_n)    // x-axis projection
    + sin(φ_n) × cos(θ_n)    // y-axis projection
    + sin(θ_n)               // z-axis projection
```

These six values per channel `(τ_n, d_n, g_n, fc_n, φ_n, θ_n)` are the
complete baked coefficient set. For 8 channels this is 48 floats — a single
cache line at 64-byte alignment. The baking computation executes in microseconds
on a background thread. The audio thread reads only these 48 floats per block.

---

## 3. DSP Architecture

### 3.1 Signal Chain Per Output Channel

```
Input ──► [Circular Delay Line: τ_n samples]
           ──► [SVF Lowpass: cutoff fc_n]      // Air absorption per distance
               ──► [RBJ Shelving: wall EQ]      // Boundary reflection coloring
                   ──► [Gain scalar: g_n × lfo_pan_weight]
                       ──► Output Channel n
```

The LFO pan weight modulates `g_n` smoothly via the inherited `AutoPanDSP`
equal-power logic, sweeping the virtual source position through the polyhedral
boundary in real time.

### 3.2 Circular Delay Line

Pre-allocated ring buffer of `maxDelaySeconds × sampleRate` samples per channel.
Write pointer advances every sample; read pointer computed as:

```cpp
int readIdx = (writeIdx - delaySamples + kDelayBufSize) % kDelayBufSize;
```

Linear interpolation for fractional delay (sub-sample accuracy):

```cpp
float frac = delaySamples - floorf(delaySamples);
output = (1.0f - frac) * buf[readIdx] + frac * buf[(readIdx+1) % kDelayBufSize];
```

No heap allocation. Buffer is a `std::array<float, kDelayBufSize>` pre-allocated
per channel in `prepare()`.

### 3.3 SVF Distance Lowpass (Per Channel)

Air absorption attenuates high frequencies with distance at approximately
1–2 dB per 100 m above 1 kHz [9]. The distance-dependent lowpass cutoff is:

```
fc_n = fc_max × exp(−k_air × distance_n)
```

where `fc_max = 18000 Hz` and `k_air = 0.15 m⁻¹` (tuned to approximate ISO
9613-1 air absorption at 20°C, 50% RH).

Implementation uses the Simper SVF topology [2] — identical to FreeEQ8 ProEQ8's
filter core — ensuring modulation stability when the source position moves and
`fc_n` changes continuously. The trapezoidal integration implicit solver
eliminates coefficient-change artifacts that would appear with RBJ biquads under
fast modulation.

```cpp
// Simper SVF lowpass (trapezoidal integration, per channel)
float g  = tanf(juce::MathConstants<float>::pi * fc_n / sr);
float k  = 1.0f / Q;
float a1 = 1.0f / (1.0f + g * (g + k));
float a2 = g * a1;
float a3 = g * a2;

float v3 = input - ic2eq;
float v1 = a1 * ic1eq + a2 * v3;
float v2 = ic2eq + a2 * ic1eq + a3 * v3;
ic1eq = 2.0f * v1 - ic1eq;
ic2eq = 2.0f * v2 - ic2eq;
float lp = v2;   // lowpass output
```

### 3.4 RBJ Boundary Reflection Filters

Wall reflections color the sound through frequency-dependent wall impedance.
This is approximated per channel by a static RBJ shelving filter [3] with
coefficients baked at topology-load time (not updated per sample):

- **Low shelf boost** at 200 Hz, +1.5 dB: simulates low-frequency pressure
  buildup near room boundaries
- **High shelf cut** at 8 kHz, −2.0 dB: simulates high-frequency wall
  absorption loss

Because these coefficients are static (room geometry doesn't change per sample),
the cheaper RBJ biquad topology is appropriate — consistent with the FreeEQ8
principle of using SVF for modulated paths and RBJ for static paths [1].

### 3.5 Gain Matrix and LFO Integration

The final per-channel gain is:

```
output_n[i] = delayedFiltered_n[i] × g_n × panWeight_n(lfoPhase[i])
```

`panWeight_n` is the projection of the LFO pan position vector onto the
direction of speaker `n` in the polyhedral layout:

```cpp
// Source position from LFO: panPos in [-1, 1] (stereo sweep)
float azimuth = panPos * (pi / 2.0f);   // map to ±90°
float srcX = cosf(azimuth);
float srcY = sinf(azimuth);

// Speaker direction unit vector for channel n
float spkX = cosf(phi_n) * cosf(theta_n);
float spkY = sinf(phi_n) * cosf(theta_n);
float spkZ = sinf(theta_n);

// Dot product projection weight (cardioid-like directivity)
float dot = srcX*spkX + srcY*spkY;   // 2D for initial stereo LFO
float panWeight_n = 0.5f * (1.0f + dot);   // [0, 1]
```

For the initial stereo LFO source (elevation = 0), the z component is zero.
Full 3D source trajectory (including elevation sweep) is designated as v0.3.0.

### 3.6 SPSC Coefficient Handoff

When the user changes topology, room scale, or source position:

1. **Background thread** recomputes all baked coefficients (< 1 µs for 8 channels)
2. Writes new `BakedCoefficients` struct to the SPSC write slot
3. **Audio thread** reads from SPSC read slot at block boundary — zero blocking,
   O(1) time, no mutex

```cpp
struct alignas(64) BakedCoefficients
{
    float delaySamples[kMaxChannels];
    float gainScalar  [kMaxChannels];
    float svfCutoffHz [kMaxChannels];
    float panWeight   [kMaxChannels];   // precomputed at bake time for static source
    int   numChannels;
    float roomScaleM;
};
```

48 floats + 2 ints = 200 bytes. Fits in 4 cache lines. The atomic pointer swap
ensures the audio thread always reads a consistent, fully-written coefficient set.

---

## 4. Topology Presets

### 4.1 Diamond (Octahedron) — Default

8 nodes at the 6 vertices of a regular octahedron plus 2 equatorial additions:

```
Node 0: ( 1,  0,  0)   Right
Node 1: (-1,  0,  0)   Left
Node 2: ( 0,  1,  0)   Front
Node 3: ( 0, -1,  0)   Rear
Node 4: ( 0,  0,  1)   Top
Node 5: ( 0,  0, -1)   Bottom
Node 6: ( 0.7, 0.7, 0) Front-Right (equatorial fill)
Node 7: (-0.7, 0.7, 0) Front-Left  (equatorial fill)
```

The mirrored upper/lower pyramid ensures symmetric intensity boundary conditions.
Energy vectors bouncing off the ceiling are perfectly mirrored by vectors
reflecting off the floor, enabling holographic imaging above and below the
listener plane — the key advantage of the diamond topology over standard 5.1 or
7.1 layouts which leave the vertical axis unresolved [4][6].

### 4.2 Cube

8 nodes at `(±1, ±1, ±1)` (normalised). Produces the most uniform coverage
at the cost of spatial aliasing on the diagonal axes. Suitable for square
control-room monitoring correction.

### 4.3 Cylinder

4 nodes at floor level, 4 at ceiling: azimuth-uniform (45° spacing).
Optimised for height panning with uniform horizontal imaging. Degenerates
to two concentric circles — equivalent to a standard height-extended 5.1 bed.

### 4.4 Sphere (Icosahedral Subset)

8-node subset of the regular icosahedron, chosen to maximise minimum solid
angle between adjacent nodes. Provides the most spatially uniform coverage
and minimises the "hole-in-the-middle" dropout that occurs in less symmetric
topologies [4].

---

## 5. Real-Time Safety Architecture

### 5.1 Allocation-Free Hot Path

| Resource | Allocation strategy |
|----------|---------------------|
| Delay ring buffers | `std::array<float, kDelayBufSize>` × N channels, in `prepare()` |
| BakedCoefficients | Two-slot atomic swap buffer, in `prepare()` |
| SVF state (ic1eq, ic2eq) | `float[kMaxChannels]`, in class body |
| RBJ biquad state (x1,x2,y1,y2) | `float[kMaxChannels]`, in class body |
| LFO phase | `float`, in class body |
| SmoothedValue | Pre-allocated in `prepare()` |

`processBlock()` contains zero heap allocation, zero file I/O, zero mutex
acquisition, zero virtual dispatch. All control flow is branchless within
the inner sample loop.

### 5.2 Denormal Safety

`juce::ScopedNoDenormals` at the top of `processBlock()` (inherited pattern
from FreeEQ8). The delay line `1e-12f` addend is not required because the
SVF lowpass damps the state variables naturally at near-silence.

### 5.3 Parameter Contract

`setParams()` is called once per block from `processBlock()` using values
loaded via `apvts.getRawParameterValue()->load()` (atomic load, no lock).
This is the identical pattern to FreeEQ8 [1] and `FreeAutoPanner` v0.1.0.

---

## 6. Known Limitations

### 6.1 Stereo LFO Source Only (v0.1)

The current LFO source from `AutoPanDSP.h` is a scalar sinusoidal pan position
— a 1D path through the 2D azimuth plane. Full 3D source trajectories (figure-8,
spiral, random walk, DAW automation) are designated for v0.3.0.

### 6.2 8-Channel Fixed Output Bus

The initial implementation targets 8-channel output to match standard 7.1
surround bus layouts in major DAWs (REAPER, Logic, Nuendo). Extension to
16-channel (Dolby Atmos 9.1.6 bed) and 32-channel (HOA 3rd order) is roadmapped
for v0.5.0.

### 6.3 Static Reflection Model

The RBJ boundary-reflection shelving filters are topology-static. A
frequency-dependent wall impedance model (per-material absorption curves,
following ISO 354 measurement data) would produce more accurate room coloring.
Designated for v0.4.0.

### 6.4 No Near-Field HRTF

Below approximately 1 m source distance, the inverse-distance gain law
`g = 1/d` diverges and the plane-wave assumption of the polyhedral boundary model
breaks down. Head-related transfer function (HRTF) correction for near-field
sources is not implemented. Minimum distance is clamped to 0.5 m to prevent
gain explosion.

### 6.5 CPU Scaling with Channel Count

The processing cost scales linearly with output channel count N:

```
Cost ≈ N × (delay_read + SVF_step + RBJ_step + multiply)
     ≈ N × ~12 FLOPs/sample
```

At N=8, 44.1 kHz, stereo input: ~4.2 MFLOPs/s — negligible on any modern CPU.
At N=32: ~16.8 MFLOPs/s — still well within a single core. SIMD vectorisation
of the N-channel gain matrix across all 8 channels simultaneously is designated
for v0.2.0 (SoA layout, AVX2).

---

## 7. Future Work

### 7.1 SIMD Vectorisation of Gain Matrix (v0.2.0)

Restructure `BakedCoefficients` from Array-of-Structures to Structure-of-Arrays
(SoA) with 64-byte alignment. Process all 8-channel gains in a single AVX2
instruction per sample. Expected speedup: 4–8× on Intel/AMD x86_64.

### 7.2 Full 3D Source Trajectory (v0.3.0)

Replace the 1D LFO pan scalar with a `(azimuth, elevation)` pair, driven by:
- Independent LFOs per axis
- DAW parameter automation
- MIDI controller mapping

The baked `panWeight_n` computation extends naturally to 3D dot products with
no architectural change required.

### 7.3 Per-Material Wall Impedance (v0.4.0)

Replace the static RBJ shelving approximation with frequency-dependent
absorption curves derived from ISO 354 measurement data for standard surface
materials (concrete, glass, wood panel, acoustic foam, carpet). Baked at
topology-load time as per-channel IIR coefficient sets.

### 7.4 Dolby Atmos 9.1.6 Bus Support (v0.5.0)

Extend output bus to 16 channels to match the Dolby Atmos 9.1.6 bed format.
Extend topology presets to include the ITU-R BS.2051 loudspeaker layout System G
(22.2) as an icosahedron subset.

### 7.5 Higher-Order Ambisonic Decoding (v0.6.0)

Replace the direct loudspeaker gain matrix with a 3rd-order Ambisonic B-format
encoder (W, X, Y, Z, R, S, T, U, V, K, L, M, N, O, P, Q). Output to any
connected HOA decoder. This decouples PANZI from the physical speaker layout
entirely and enables binaural headphone rendering via convolution with measured
HRTFs.

### 7.6 Custom Framework Migration (Long-term)

Per the PANZI roadmap (§10), FreeVox8 is the designated flagship for the
custom bare-metal framework. PANZI and FreeEQ8 remain on JUCE for stability.
If FreeVox8's custom framework demonstrates sufficient maturity, PANZI v1.0.0
may be offered in both JUCE and custom-framework builds.

---

## 8. Production Gate

PANZI v0.2.0 public binary requires:

1. Release build: macOS Universal, Linux x86_64, Windows x64
2. pluginval strictness-level-10 pass for VST3 + AU
3. DAW smoke tests: REAPER (multi-channel VST3), Logic Pro (AU), Ableton Live
4. 8-channel output bus verification in REAPER (7.1 surround project)
5. No clicks or zipper noise on topology switch
6. No gain explosion at minimum distance (0.5 m clamp verified)
7. CPU profiling: single instance < 2% one core at 44.1 kHz, 128-sample buffer
8. Factory preset suite (8 presets minimum, one per topology × 2 source modes)
9. Release evidence pack

---

## 9. Acknowledgments

The allocation-free, lock-free audio processing doctrine, SPSC ring buffer
pattern, SmoothedValue parameter ramping strategy, and pluginval CI framework
are derived directly from FreeEQ8 [1]. The SVF filter core used for
distance-dependent air absorption follows the Simper trapezoidal topology [2].
Static boundary-reflection EQ uses RBJ biquad coefficients [3]. The Wave Field
Synthesis boundary theory motivating the polyhedral topology presets follows
Spors, Rabenstein, and Ahrens [4]. The 5D acoustic intensity vector framework
follows Meissner [5] and Kotus & Kostek [6].

The name PANZI and the core architectural insight — that 4D/5D acoustic vector
field theory can be collapsed to a static coefficient bake, feeding a proven
RBJ/SVF matrix engine at sample rate — originate with Gary Doman (TizWildin).

---

## 10. Ecosystem Roadmap

```
CURRENT (JUCE):
├─ FreeEQ8   ──► Finished. Thread-hardened parametric EQ. RBJ + SVF.
├─ FreeVox8  ──► Active. Dual-engine spectral vocoder. JUCE.
└─ PANZI     ──► Active. Polyhedral spatializer. JUCE → iPlug2 (v0.3+).

FUTURE (Custom Framework):
└─ FreeVox8 v2.0 ──► Custom bare-metal C++20 framework flagship.
                      SoA alignment, CRTP compile-time polymorphism,
                      AVX-512/Neon native intrinsics, RtAudio backend.
                      FreeEQ8 + PANZI remain on JUCE for stability.
```

---

## References

[1] G. Doman, "Lock-Free Dynamic EQ Architecture: A Production-Grade SVF
    Implementation in JUCE/C++," FreeEQ8 / ProEQ8 Open-Source DSP Project,
    2026. https://github.com/GareBear99/FreeEQ8

[2] A. Simper, "Solving the continuous SVF equations using trapezoidal
    integration and equivalent currents," Cytomic Technical Papers, 2013.
    https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf

[3] R. Bristow-Johnson, "Audio EQ Cookbook," musicdsp.org, 1994.
    https://www.musicdsp.org/files/Audio-EQ-Cookbook.txt

[4] S. Spors, R. Rabenstein, and J. Ahrens, "The theory of wave field synthesis
    revisited," AES Convention Paper 7358, Audio Engineering Society, 2008.

[5] M. Meissner, "Computer simulation of active sound intensity vector field
    in enclosure of irregular geometry," Institute of Fundamental Technological
    Research, Polish Academy of Sciences, 2012.

[6] J. Kotus and B. Kostek, "Measurements and visualization of sound intensity
    around the human head in free field using acoustic vector sensor," J. Audio
    Eng. Soc., vol. 63, no. 1, pp. 99–109, 2015.
    https://doi.org/10.17743/jaes.2015.0009

[7] S. Bilbao, "Numerical Sound Synthesis: Finite Difference Schemes and
    Simulation in Musical Acoustics," John Wiley & Sons, 2009.

[8] O. Thomas, "Wayverb: A graphical tool for hybrid room acoustics simulation,"
    Ph.D. dissertation, University of Huddersfield, 2017.

[9] B. C. J. Moore, "An Introduction to the Psychology of Hearing," 6th ed.,
    Brill, 2012.

[10] C. Kirsch, J. Poppitz, T. Wendt, S. van de Par, and S. D. Ewert,
     "Spatial resolution of late reverberation in virtual acoustic environments,"
     Trends in Hearing, vol. 25, pp. 1–15, 2021.
     https://doi.org/10.1177/23312165211054924

[11] V. Cecconi et al., "Terahertz spatiotemporal wave synthesis in random
     systems," ACS Photonics, vol. 11, no. 2, pp. 362–368, 2024.
     https://doi.org/10.1021/acsphotonics.3c01671

[12] G. Doman, "Dual-Engine Spectral Vocoder Architecture: Real-Time Ghost
     Resynthesis and Dynamic Spectral Masking in JUCE/C++," FreeVox8
     Open-Source DSP Project, 2026. https://github.com/GareBear99/FreeVox8

[13] ISO 9613-1:1993, "Acoustics — Attenuation of sound during propagation
     outdoors — Part 1: Calculation of the absorption of sound by the
     atmosphere," International Organization for Standardization, 1993.

[14] ISO 354:2003, "Acoustics — Measurement of sound absorption in a
     reverberation room," International Organization for Standardization, 2003.

[15] R. Bristow-Johnson and K. Bogdanowicz, "Intraframe Time-Scaling of
     Nonstationary Sinusoids Within the Phase Vocoder," in Proc. IEEE Workshop
     on Applications of Signal Processing to Audio and Acoustics (WASPAA),
     New Paltz, NY, Oct. 2001.
