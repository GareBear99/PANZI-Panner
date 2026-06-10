#pragma once
#include <array>
#include <cmath>
#include <algorithm>
#include "BakedCoefficients.h"

// Fractional circular delay line per channel.
// Pre-allocated in prepare(), zero heap on audio thread.
// Sub-sample accuracy via linear interpolation.
template <int N = kMaxChannels>
class DelayLine
{
public:
    void prepare (float /*sampleRate*/) noexcept
    {
        for (auto& b : buf) b.fill(0.0f);
        writeIdx = 0;
    }

    void reset() noexcept
    {
        for (auto& b : buf) b.fill(0.0f);
        writeIdx = 0;
    }

    // Write input to all channel delay lines at current write index.
    void write (float input) noexcept
    {
        for (int ch = 0; ch < N; ++ch)
            buf[ch][writeIdx] = input;

        writeIdx = (writeIdx + 1) % kDelayBufSize;
    }

    // Read channel `ch` with fractional delay `delaySamples`.
    float read (int ch, float delaySamples) const noexcept
    {
        const int   d0  = static_cast<int>(delaySamples);
        const float frc = delaySamples - static_cast<float>(d0);

        const int r0 = (writeIdx - d0     + kDelayBufSize) % kDelayBufSize;
        const int r1 = (writeIdx - d0 - 1 + kDelayBufSize) % kDelayBufSize;

        return (1.0f - frc) * buf[ch][r0] + frc * buf[ch][r1];
    }

private:
    std::array<std::array<float, kDelayBufSize>, N> buf {};
    int writeIdx = 0;
};
