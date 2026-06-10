#pragma once
#include <array>
#include <cmath>
#include "BakedCoefficients.h"

// Static RBJ biquad shelving filter for boundary reflection coloring.
// Coefficients computed once in prepare() — never updated per sample.
// (RBJ used here because coefficients are static; SVF used for modulated paths.)
//
// Models:
//   Low shelf +1.5 dB @ 200 Hz  — low-frequency pressure buildup near boundaries
//   High shelf -2.0 dB @ 8 kHz  — high-frequency wall absorption loss
//
// Both filters cascaded in series per channel.
template <int N = kMaxChannels>
class RbjShelf
{
public:
    void prepare (float sampleRate) noexcept
    {
        sr = sampleRate;
        computeCoeffs();
        resetState();
    }

    void reset() noexcept { resetState(); }

    // Process one sample for channel `ch`.
    float process (int ch, float x) noexcept
    {
        // Low shelf
        float y = biquad(x, ch, 0,
                         b0ls, b1ls, b2ls, a1ls, a2ls);
        // High shelf (cascaded)
        return biquad(y, ch, 1,
                      b0hs, b1hs, b2hs, a1hs, a2hs);
    }

private:
    float sr = 44100.0f;

    // Biquad coefficients (same for all channels — static geometry)
    float b0ls=1,b1ls=0,b2ls=0,a1ls=0,a2ls=0;
    float b0hs=1,b1hs=0,b2hs=0,a1hs=0,a2hs=0;

    // Per-channel, per-filter state: [channel][filter 0=ls, 1=hs]
    std::array<float, N> x1 [2] {};
    std::array<float, N> x2 [2] {};
    std::array<float, N> y1 [2] {};
    std::array<float, N> y2 [2] {};

    void resetState() noexcept
    {
        for (int f = 0; f < 2; ++f)
        {
            x1[f].fill(0.0f);
            x2[f].fill(0.0f);
            y1[f].fill(0.0f);
            y2[f].fill(0.0f);
        }
    }

    void computeCoeffs() noexcept
    {
        // RBJ low shelf: fc=200 Hz, S=1, dBgain=+1.5
        computeLowShelf (200.0f,  1.5f, b0ls, b1ls, b2ls, a1ls, a2ls);
        // RBJ high shelf: fc=8000 Hz, S=1, dBgain=-2.0
        computeHighShelf(8000.0f, -2.0f, b0hs, b1hs, b2hs, a1hs, a2hs);
    }

    void computeLowShelf (float fc, float dBgain,
                          float& b0, float& b1, float& b2,
                          float& a1, float& a2) noexcept
    {
        const float A  = std::pow(10.0f, dBgain / 40.0f);
        const float w0 = 2.0f * 3.14159265f * fc / sr;
        const float cw = std::cos(w0);
        const float sw = std::sin(w0);
        const float S  = 1.0f;
        const float al = sw / 2.0f * std::sqrt((A + 1.0f/A) * (1.0f/S - 1.0f) + 2.0f);

        const float a0r = 1.0f / ((A+1) + (A-1)*cw + 2*std::sqrt(A)*al);
        b0 = A * ((A+1) - (A-1)*cw + 2*std::sqrt(A)*al) * a0r;
        b1 = 2*A * ((A-1) - (A+1)*cw)                   * a0r;
        b2 = A * ((A+1) - (A-1)*cw - 2*std::sqrt(A)*al) * a0r;
        a1 = -2 * ((A-1) + (A+1)*cw)                    * a0r;
        a2 = ((A+1) + (A-1)*cw - 2*std::sqrt(A)*al)     * a0r;
    }

    void computeHighShelf (float fc, float dBgain,
                           float& b0, float& b1, float& b2,
                           float& a1, float& a2) noexcept
    {
        const float A  = std::pow(10.0f, dBgain / 40.0f);
        const float w0 = 2.0f * 3.14159265f * fc / sr;
        const float cw = std::cos(w0);
        const float sw = std::sin(w0);
        const float S  = 1.0f;
        const float al = sw / 2.0f * std::sqrt((A + 1.0f/A) * (1.0f/S - 1.0f) + 2.0f);

        const float a0r = 1.0f / ((A+1) - (A-1)*cw + 2*std::sqrt(A)*al);
        b0 = A * ((A+1) + (A-1)*cw + 2*std::sqrt(A)*al) * a0r;
        b1 = -2*A * ((A-1) + (A+1)*cw)                  * a0r;
        b2 = A * ((A+1) + (A-1)*cw - 2*std::sqrt(A)*al) * a0r;
        a1 = 2 * ((A-1) - (A+1)*cw)                     * a0r;
        a2 = ((A+1) - (A-1)*cw - 2*std::sqrt(A)*al)     * a0r;
    }

    float biquad (float x, int ch, int f,
                  float b0, float b1, float b2,
                  float a1, float a2) noexcept
    {
        const float y = b0*x + b1*x1[f][ch] + b2*x2[f][ch]
                             - a1*y1[f][ch] - a2*y2[f][ch];
        x2[f][ch] = x1[f][ch]; x1[f][ch] = x;
        y2[f][ch] = y1[f][ch]; y1[f][ch] = y;
        return y;
    }
};
