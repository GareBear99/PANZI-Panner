#include "PluginProcessor.h"
#include "PluginEditor.h"

PanziAudioProcessor::PanziAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::create7point1(), true))
    , apvts (*this, nullptr, "PANZI_STATE", createParams())
{
    bakeThread.startThread (juce::Thread::Priority::low);
}

PanziAudioProcessor::~PanziAudioProcessor()
{
    bakeThread.stopThread (1000);
}

juce::AudioProcessorValueTreeState::ParameterLayout PanziAudioProcessor::createParams()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    // Spatial params
    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        "topology", "Topology",
        juce::StringArray { "Diamond", "Cube", "Cylinder", "Sphere" }, 0));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        "roomScale", "Room Scale (m)",
        juce::NormalisableRange<float> (0.5f, 20.0f, 0.01f, 0.5f), 5.0f));

    // LFO params (inherited from FreeAutoPanner)
    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        "rate", "Rate (Hz)",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.0001f, 0.5f), 1.0f));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        "depth", "Depth (%)",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 100.0f));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        "center", "Center",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.01f), 0.0f));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        "symmetry", "Symmetry",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 0.0f));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        "smooth", "Smooth (ms)",
        juce::NormalisableRange<float> (1.0f, 200.0f, 0.01f), 20.0f));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        "detect", "Detect (%)",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 0.0f));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        "sensitivity", "Sensitivity",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 50.0f));

    return { p.begin(), p.end() };
}

bool PanziAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Accept stereo input with either stereo or 7.1 output
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo()
        || out == juce::AudioChannelSet::create7point1();
}

void PanziAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr        = sampleRate;
    blockSize = samplesPerBlock;
    engine.prepare(sr, blockSize);
    engine.reset();
    pullLfoParams();
    pullBakeParams();
    triggerBake();
}

void PanziAudioProcessor::pullLfoParams()
{
    rateHz        = apvts.getRawParameterValue("rate")       ->load();
    depth01       = apvts.getRawParameterValue("depth")      ->load() / 100.0f;
    center01      = apvts.getRawParameterValue("center")     ->load() / 100.0f;
    symmetry01    = apvts.getRawParameterValue("symmetry")   ->load() / 100.0f;
    smoothMs      = apvts.getRawParameterValue("smooth")     ->load();
    detect01      = apvts.getRawParameterValue("detect")     ->load() / 100.0f;
    sensitivity01 = apvts.getRawParameterValue("sensitivity")->load() / 100.0f;
}

void PanziAudioProcessor::pullBakeParams()
{
    topologyIdx = static_cast<int>(apvts.getRawParameterValue("topology") ->load());
    roomScaleM  = apvts.getRawParameterValue("roomScale")->load();
}

void PanziAudioProcessor::triggerBake()
{
    bakePending.store(true, std::memory_order_release);
    bakeThread.notify();
}

void PanziAudioProcessor::BakeThread::run()
{
    while (!threadShouldExit())
    {
        wait(20);  // idle until notified or 20ms poll
        if (!proc.bakePending.load(std::memory_order_acquire))
            continue;

        proc.pullBakeParams();
        const auto topo = static_cast<Topology>(proc.topologyIdx);

        // Bake with source at room center (0,0,0) — v0.2.0 default.
        // Full 3D source trajectory designated for v0.3.0.
        auto c = TopologyBaker::bake(topo,
                                     proc.roomScaleM,
                                     0.0f, 0.0f, 0.0f,
                                     static_cast<float>(proc.sr));

        proc.handoff.write(c);
        proc.bakePending.store(false, std::memory_order_release);
    }
}

void PanziAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Pull fresh coefficients if bake completed
    BakedCoefficients fresh;
    if (handoff.tryRead(fresh))
        engine.applyCoefficients(fresh);

    // Update LFO params every block (atomic loads, no lock)
    pullLfoParams();
    engine.setLfoParams(rateHz, depth01, center01,
                        symmetry01, smoothMs, detect01, sensitivity01);

    const int numOut = getTotalNumOutputChannels();
    const int N      = std::min(numOut, 8);
    const int ns     = buffer.getNumSamples();

    // Build output channel pointer array
    // Zero each output channel before accumulation
    float* outPtrs[kMaxChannels] = {};
    for (int n = 0; n < N; ++n)
    {
        outPtrs[n] = buffer.getWritePointer(n);
        juce::FloatVectorOperations::clear(outPtrs[n], ns);
    }

    engine.processBlock(buffer, outPtrs, N, ns);
}

void PanziAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PanziAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        triggerBake();
    }
}

juce::AudioProcessorEditor* PanziAudioProcessor::createEditor()
{
    return new PanziAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PanziAudioProcessor();
}
