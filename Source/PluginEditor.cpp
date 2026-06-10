#include "PluginEditor.h"

PanziAudioProcessorEditor::PanziAudioProcessorEditor (PanziAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    setSize (860, 340);

    // Topology combo
    topologyLabel.setText ("Topology", juce::dontSendNotification);
    topologyLabel.setJustificationType (juce::Justification::centred);
    topologyLabel.setFont (juce::Font (12.0f));
    addAndMakeVisible (topologyLabel);

    topologyBox.addItem ("Diamond",  1);
    topologyBox.addItem ("Cube",     2);
    topologyBox.addItem ("Cylinder", 3);
    topologyBox.addItem ("Sphere",   4);
    topologyBox.onChange = [this] { proc.triggerBake(); };
    addAndMakeVisible (topologyBox);
    topologyAtt = std::make_unique<ComboAtt> (proc.apvts, "topology", topologyBox);

    // Room scale knob
    initKnob (roomScale, roomScaleLabel, "Room Scale (m)");
    roomScale.onValueChange = [this] { proc.triggerBake(); };
    roomScaleAtt = std::make_unique<SliderAtt> (proc.apvts, "roomScale", roomScale);

    // LFO knobs
    initKnob (rate,        "Rate (Hz)");
    initKnob (depth,       "Depth (%)");
    initKnob (center,      "Center");
    initKnob (symmetry,    "Symmetry");
    initKnob (smooth,      "Smooth (ms)");
    initKnob (detect,      "Detect (%)");
    initKnob (sensitivity, "Sensitivity");

    rateAtt        = std::make_unique<SliderAtt> (proc.apvts, "rate",        rate);
    depthAtt       = std::make_unique<SliderAtt> (proc.apvts, "depth",       depth);
    centerAtt      = std::make_unique<SliderAtt> (proc.apvts, "center",      center);
    symmetryAtt    = std::make_unique<SliderAtt> (proc.apvts, "symmetry",    symmetry);
    smoothAtt      = std::make_unique<SliderAtt> (proc.apvts, "smooth",      smooth);
    detectAtt      = std::make_unique<SliderAtt> (proc.apvts, "detect",      detect);
    sensitivityAtt = std::make_unique<SliderAtt> (proc.apvts, "sensitivity", sensitivity);

    startTimerHz (30);
}

void PanziAudioProcessorEditor::initKnob (juce::Slider& s, juce::Label& l,
                                           const juce::String& name)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
    addAndMakeVisible (s);
    l.setText (name, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setFont (juce::Font (11.0f));
    addAndMakeVisible (l);
}

void PanziAudioProcessorEditor::initKnob (juce::Slider& s, const juce::String& name)
{
    s.setName (name);
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
    addAndMakeVisible (s);
}

void PanziAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0a0b12));

    // Header
    g.setColour (juce::Colour (0xff6d28d9));
    g.fillRect (0, 0, getWidth(), 44);
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (22.0f, juce::Font::bold));
    g.drawText ("PANZI", 14, 8, 120, 28, juce::Justification::left);
    g.setFont (juce::Font (11.0f));
    g.setColour (juce::Colours::white.withAlpha(0.6f));
    g.drawText ("Polyhedral Acoustic Network & Zero-delay Intensity Engine  |  v0.2.0  |  TizWildin",
                140, 14, getWidth()-154, 16, juce::Justification::left);

    // Section dividers
    g.setColour (juce::Colours::white.withAlpha(0.08f));
    g.drawLine (200, 54, 200, 320, 1.0f);

    g.setColour (juce::Colours::white.withAlpha(0.4f));
    g.setFont (juce::Font (10.0f));
    g.drawText ("SPATIAL", 10, 52, 180, 14, juce::Justification::left);
    g.drawText ("LFO / MODULATION", 210, 52, 600, 14, juce::Justification::left);
}

void PanziAudioProcessorEditor::resized()
{
    // Spatial section (left column)
    topologyLabel .setBounds (10,  68,  180, 14);
    topologyBox   .setBounds (10,  84,  180, 26);
    roomScaleLabel.setBounds (10, 118,  180, 14);
    roomScale     .setBounds (10, 132,  180, 170);

    // LFO knobs (right of divider)
    const int top    = 66;
    const int pad    = 8;
    const int cols   = 7;
    const int startX = 208;
    const int w      = (getWidth() - startX - pad * (cols + 1)) / cols;
    const int h      = 240;

    juce::Slider* sliders[7] = { &rate, &depth, &center, &symmetry,
                                  &smooth, &detect, &sensitivity };
    for (int i = 0; i < cols; ++i)
        sliders[i]->setBounds (startX + pad + i * (w + pad), top, w, h);
}
