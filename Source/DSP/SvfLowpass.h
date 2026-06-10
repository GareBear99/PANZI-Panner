#pragma once
#include <array>
#include <cmath>
#include "BakedCoefficients.h"

// Simper trapezoidal SVF lowpass (one instance per output channel).
// Topology: Andy Simper, Cytomic, 2013.
// Stable under fast cutoff modulation — used for distance-based air absorption.
// Q fixed at 0.707 (Butterworth, maximally flat magnitude response).
template <int N = kMaxChannels>
class SvfLowpass
{
public:
    void prepare (float sampleRate_) noexcept
    {
        sr = sampleRate_;
        reset();
    }

    void reset() noexcept
    {
        ic1eq.fill(0.0f);
        ic2eq.fill(0.0f);
    }

    // Process one sample for channel `ch` at cutoff `fc` Hz.
    float process (int ch, float input, float fc) noexcept
    {
        const float g  = std::tan(3.14159265f * fc / sr);
        const float k  = 1.41421356f;  // 1/Q = sqrt(2) for Butterworth
        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float a2 = g * a1;
        const float a3 = g * a2;

        const float v3 = input - ic2eq[ch];
        const float v1 = a1 * ic1eq[ch] + a2 * v3;
        const float v2 = ic2eq[ch] + a2 * ic1eq[ch] + a3 * v3;

        ic1eq[ch] = 2.0f * v1 - ic1eq[ch];
        ic2eq[ch] = 2.0f * v2 - ic2eq[ch];

        return v2;  // lowpass output
    }

private:
    float sr = 44100.0f;
    std::array<float, N> ic1eq {};
    std::array<float, N> ic2eq {};
};
