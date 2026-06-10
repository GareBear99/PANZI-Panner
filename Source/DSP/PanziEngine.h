#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>

#include "BakedCoefficients.h"
#include "DelayLine.h"
#include "SvfLowpass.h"
#include "RbjShelf.h"
#include "AutoPanDSP.h"

// PanziEngine: the master per-channel audio processing core.
//
// Signal chain per output channel n:
//   Input
//    → [Circular delay: τ_n samples]       // 4D spacetime
//    → [SVF Lowpass: fc_n Hz]              // 5D φ: air absorption
//    → [RBJ shelf: static boundary EQ]     // boundary reflection coloring
//    → [× gainScalar_n]                    // 5D P: pressure law
//    → [× panWeight_n(lfoPos)]             // Ix,Iy,Iz: directional intensity
//    → outputChannels[n]
//
// The LFO source position is provided by AutoPanDSP (inherited FreeAutoPanner engine).
// Zero heap allocation on audio thread. All state pre-allocated in prepare().
class PanziEngine
{
public:
    void prepare (double sampleRate_, int /*samplesPerBlock*/)
    {
        sr = static_cast<float>(sampleRate_);
        delay.prepare(sr);
        svf.prepare(sr);
        rbj.prepare(sr);
        lfo.prepare(sr, 0);
        coeffs = BakedCoefficients{};
    }

    void reset()
    {
        delay.reset();
        svf.reset();
        rbj.reset();
        lfo.reset();
    }

    void setLfoParams (float rateHz, float depth01, float center01,
                       float symmetry01, float smoothMs,
                       float detect01, float sensitivity01)
    {
        lfo.setParams(rateHz, depth01, center01, symmetry01,
                      smoothMs, detect01, sensitivity01);
    }

    // Called by audio thread at block boundary if handoff has fresh coefficients.
    void applyCoefficients (const BakedCoefficients& c) noexcept
    {
        coeffs = c;
        // Recompute static boundary shelf for new sample rate if needed.
        // (rbj re-prepares only if sr changed — cheap guard)
        rbj.prepare(c.sampleRate);
    }

    // Process one stereo input block → N-channel output buffer.
    // outputChannels: array of N write pointers, each `numSamples` long.
    // Caller must zero outputChannels before calling (additive accumulation).
    void processBlock (const juce::AudioBuffer<float>& input,
                       float* outputChannels[kMaxChannels],
                       int numChannels,
                       int numSamples)
    {
        juce::ScopedNoDenormals noDenormals;

        const auto* L = input.getReadPointer(0);
        const auto* R = input.getNumChannels() > 1
                        ? input.getReadPointer(1) : L;

        const int N = std::min(numChannels, coeffs.numChannels);

        for (int i = 0; i < numSamples; ++i)
        {
            const float mono = 0.5f * (L[i] + R[i]);

            // LFO step: returns pan position in [-1, 1]
            // We drive the LFO with a single-channel buffer trick:
            // just read the phase-derived pan value directly.
            const float panPos = lfo.getNextPanPosition(mono);

            // Write input to all delay lines
            delay.write(mono);

            for (int n = 0; n < N; ++n)
            {
                // 1. Fractional delay (4D spacetime)
                float s = delay.read(n, coeffs.delaySamples[n]);

                // 2. SVF lowpass (5D φ: air absorption)
                s = svf.process(n, s, coeffs.svfCutoffHz[n]);

                // 3. RBJ shelf (static boundary reflection)
                s = rbj.process(n, s);

                // 4. Inverse-distance gain (5D P: pressure law)
                s *= coeffs.gainScalar[n];

                // 5. Directional intensity weight (Ix, Iy, Iz projection)
                //    Project LFO pan position onto speaker direction vector.
                //    Elevation = 0 for current stereo-sweep LFO (v0.2.0).
                const float az  = panPos * (3.14159265f * 0.5f);  // ±90°
                const float srcX = std::cos(az);
                const float srcY = std::sin(az);
                const float spkX = std::cos(coeffs.speakerPhi[n])
                                  * std::cos(coeffs.speakerTheta[n]);
                const float spkY = std::sin(coeffs.speakerPhi[n])
                                  * std::cos(coeffs.speakerTheta[n]);
                const float dot  = srcX * spkX + srcY * spkY;
                const float pw   = 0.5f * (1.0f + dot);  // cardioid [0, 1]

                outputChannels[n][i] += s * pw;
            }
        }
    }

private:
    float             sr = 44100.0f;
    BakedCoefficients coeffs;
    DelayLine<kMaxChannels>  delay;
    SvfLowpass<kMaxChannels> svf;
    RbjShelf<kMaxChannels>   rbj;
    AutoPanDSP               lfo;
};
