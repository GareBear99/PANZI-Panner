#pragma once
#include <JuceHeader.h>
#include "DSP/PanziEngine.h"
#include "DSP/BakedCoefficients.h"
#include "DSP/TopologyBaker.h"
#include "DSP/PolyhedralTopology.h"

class PanziAudioProcessor : public juce::AudioProcessor
{
public:
    PanziAudioProcessor();
    ~PanziAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PANZI"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Called from editor when topology or roomScale changes.
    // Triggers a background bake — safe to call from any thread.
    void triggerBake();

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();
    void pullLfoParams();
    void pullBakeParams();

    PanziEngine  engine;
    CoeffHandoff handoff;

    double sr          = 44100.0;
    int    blockSize   = 512;

    // Bake trigger: set by any thread, cleared by bake thread.
    std::atomic<bool> bakePending { false };

    // Background bake thread (JUCE thread)
    struct BakeThread : public juce::Thread
    {
        BakeThread (PanziAudioProcessor& p) : juce::Thread("PANZI_Baker"), proc(p) {}
        void run() override;
        PanziAudioProcessor& proc;
    };
    BakeThread bakeThread { *this };

    // LFO params (read each block)
    float rateHz       = 1.0f;
    float depth01      = 1.0f;
    float center01     = 0.0f;
    float symmetry01   = 0.0f;
    float smoothMs     = 20.0f;
    float detect01     = 0.0f;
    float sensitivity01= 0.5f;

    // Bake params (read at bake time)
    int   topologyIdx  = 0;
    float roomScaleM   = 5.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanziAudioProcessor)
};
