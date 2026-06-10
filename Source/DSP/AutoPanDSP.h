\
#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>

class AutoPanDSP
{
public:
    void prepare(double sampleRate, int /*samplesPerBlock*/)
    {
        sr = sampleRate;
        phase = 0.0f;

        panSm.reset(sr, smoothSeconds);
        panSm.setCurrentAndTargetValue(0.0f);

        levelFollower.setAttackTime(attackMs);
        levelFollower.setReleaseTime(releaseMs);
        levelFollower.setSampleRate(sr);
        levelFollower.reset();
    }

    void reset()
    {
        phase = 0.0f;
        panSm.setCurrentAndTargetValue(0.0f);
        levelFollower.reset();
    }

    void setParams(float rateHz_,
                   float depth01_,
                   float center01_,        // -1..+1
                   float symmetry01_,      // 0..1
                   float smoothMs_,
                   float detect01_,        // 0..1
                   float sensitivity01_)   // 0..1
    {
        rateHz = std::clamp(rateHz_, 0.01f, 20.0f);
        depth01 = std::clamp(depth01_, 0.0f, 1.0f);
        center01 = std::clamp(center01_, -1.0f, 1.0f);
        symmetry01 = std::clamp(symmetry01_, 0.0f, 1.0f);
        detect01 = std::clamp(detect01_, 0.0f, 1.0f);
        sensitivity01 = std::clamp(sensitivity01_, 0.0f, 1.0f);

        const float smMs = std::clamp(smoothMs_, 1.0f, 200.0f);
        smoothSeconds = smMs / 1000.0f;
        panSm.reset(sr, smoothSeconds);
    }

    // Returns the next LFO pan position in [-1, 1] and advances phase.
    // Used by PanziEngine to drive per-channel directional weight.
    // `monoInput` is used for level detection (Detect parameter).
    float getNextPanPosition (float monoInput) noexcept
    {
        levelFollower.processSample(0, std::abs(monoInput));
        const float envOut = levelFollower.getCurrentLevel();
        const float expn   = 0.2f + 3.0f * sensitivity01;
        const float env01  = std::clamp(std::pow(envOut * 4.0f, expn), 0.0f, 1.0f);
        const float dynDepth = std::clamp(depth01 * (1.0f + detect01 * env01), 0.0f, 1.0f);

        phase += rateHz / (float)sr;
        if (phase >= 1.0f) phase -= 1.0f;

        float x = std::sin(juce::MathConstants<float>::twoPi * phase);
        float u = 0.5f * (x + 1.0f);
        const float skew = 1.0f + 8.0f * symmetry01;
        float uSkew = (u < 0.5f)
            ? 0.5f * std::pow(u * 2.0f, skew)
            : 1.0f - 0.5f * std::pow((1.0f - u) * 2.0f, skew);
        u = juce::jmap(symmetry01, u, uSkew);
        x = 2.0f * u - 1.0f;

        float pan = center01 + x * dynDepth;
        panSm.setTargetValue(std::clamp(pan, -1.0f, 1.0f));
        return panSm.getNextValue();
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        const int ch = buffer.getNumChannels();
        const int n = buffer.getNumSamples();
        if (ch < 2) return;

        auto* L = buffer.getWritePointer(0);
        auto* R = buffer.getWritePointer(1);

        // Simple mean-abs level for detection
        float absSum = 0.0f;
        for (int i = 0; i < n; ++i)
            absSum += std::abs(0.5f * (L[i] + R[i]));
        const float meanAbs = absSum / (float)std::max(1, n);

        levelFollower.processSample(0, meanAbs);
        const float envOut = levelFollower.getCurrentLevel();

        // env -> 0..1 with sensitivity curve
        const float expn = 0.2f + 3.0f * sensitivity01;   // 0.2..3.2
        const float env01 = std::clamp(std::pow(envOut * 4.0f, expn), 0.0f, 1.0f);

        const float dynDepth = std::clamp(depth01 * (1.0f + detect01 * env01), 0.0f, 1.0f);

        const float phaseInc = rateHz / (float)sr;

        for (int i = 0; i < n; ++i)
        {
            phase += phaseInc;
            if (phase >= 1.0f) phase -= 1.0f;

            float x = std::sin(juce::MathConstants<float>::twoPi * phase); // -1..1

            // Symmetry warp using skew power curve in 0..1 domain
            float u = 0.5f * (x + 1.0f); // 0..1
            const float skew = 1.0f + 8.0f * symmetry01; // 1..9

            float uSkew = (u < 0.5f)
                ? 0.5f * std::pow(u * 2.0f, skew)
                : 1.0f - 0.5f * std::pow((1.0f - u) * 2.0f, skew);

            u = juce::jmap(symmetry01, u, uSkew);
            x = 2.0f * u - 1.0f;

            float pan = center01 + x * dynDepth;
            pan = std::clamp(pan, -1.0f, 1.0f);

            panSm.setTargetValue(pan);
            const float panS = panSm.getNextValue();

            // Equal-power pan
            const float theta = (panS + 1.0f) * 0.25f * juce::MathConstants<float>::pi; // 0..pi/2
            const float gL = std::cos(theta);
            const float gR = std::sin(theta);

            L[i] *= gL;
            R[i] *= gR;
        }
    }

private:
    double sr = 44100.0;
    float phase = 0.0f;

    float rateHz = 1.0f;
    float depth01 = 1.0f;
    float center01 = 0.0f;
    float symmetry01 = 0.0f;
    float detect01 = 0.0f;
    float sensitivity01 = 0.5f;

    float smoothSeconds = 0.02f;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> panSm;

    juce::dsp::BallisticsFilter<float> levelFollower;
    float attackMs = 10.0f;
    float releaseMs = 150.0f;
};
