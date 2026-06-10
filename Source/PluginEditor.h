#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class PanziAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit PanziAudioProcessorEditor (PanziAudioProcessor&);
    ~PanziAudioProcessorEditor() override = default;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    void timerCallback() override {}

    PanziAudioProcessor& proc;

    // Spatial controls
    juce::ComboBox topologyBox;
    juce::Label    topologyLabel;
    juce::Slider   roomScale;
    juce::Label    roomScaleLabel;

    // LFO controls (inherited from FreeAutoPanner)
    juce::Slider   rate, depth, center, symmetry, smooth, detect, sensitivity;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ComboAtt>  topologyAtt;
    std::unique_ptr<SliderAtt> roomScaleAtt, rateAtt, depthAtt, centerAtt,
                                symmetryAtt, smoothAtt, detectAtt, sensitivityAtt;

    void initKnob (juce::Slider& s, juce::Label& l, const juce::String& name);
    void initKnob (juce::Slider& s, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanziAudioProcessorEditor)
};
