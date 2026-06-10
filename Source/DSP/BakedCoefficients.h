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
    float speakerPhi   [kMaxChannels] = {};   // azimuth (radians)
    float speakerTheta [kMaxChannels] = {};   // elevation (radians)
    int   numChannels  = 8;
    float roomScaleM   = 5.0f;
    float sampleRate   = 44100.0f;
    int   topology     = 0;                   // 0=Diamond,1=Cube,2=Cylinder,3=Sphere
};

// Two-slot atomic swap for SPSC handoff (background baker → audio thread).
// Audio thread always reads the last fully-written slot.
class CoeffHandoff
{
public:
    void write (const BakedCoefficients& c) noexcept
    {
        int slot = 1 - readSlot.load(std::memory_order_relaxed);
        buf[slot] = c;
        readSlot.store(slot, std::memory_order_release);
        pending.store(true, std::memory_order_release);
    }

    // Returns true if a fresh set was available; caller should re-read coeffs.
    bool tryRead (BakedCoefficients& out) noexcept
    {
        if (!pending.load(std::memory_order_acquire)) return false;
        out = buf[readSlot.load(std::memory_order_acquire)];
        pending.store(false, std::memory_order_release);
        return true;
    }

private:
    BakedCoefficients        buf[2];
    std::atomic<int>         readSlot { 0 };
    std::atomic<bool>        pending  { false };
};
