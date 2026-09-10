#include "PluginEditor.h"

HybridKickEQAudioProcessorEditor::HybridKickEQAudioProcessorEditor (HybridKickEQAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), curveComponent (p), bandPanel (p)
{
    setLookAndFeel (&kickLookAndFeel);

    titleLabel.setFont (juce::Font (18.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, KickColours::accent);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    auto sr = p.getCurrentSampleRate() > 0.0 ? p.getCurrentSampleRate() : 44100.0;
    auto latencyMs = 1000.0 * (double) p.getLatencySamples() / sr;
    latencyLabel.setText ("Latencia: ~" + juce::String (latencyMs, 1) + " ms",
                           juce::dontSendNotification);

    latencyLabel.setFont (juce::Font (10.0f));
    latencyLabel.setColour (juce::Label::textColourId, KickColours::textDim);
    latencyLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (latencyLabel);

    addAndMakeVisible (curveComponent);
    addAndMakeVisible (bandPanel);

    for (auto* s : { &saturationSlider, &outputGainSlider })
    {
        s->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
        addAndMakeVisible (s);
    }

    for (auto* l : { &saturationLabel, &outputLabel })
    {
        l->setJustificationType (juce::Justification::centred);
        l->setFont (juce::Font (10.0f, juce::Font::bold));
        l->setColour (juce::Label::textColourId, KickColours::textDim);
        addAndMakeVisible (l);
    }

    addAndMakeVisible (bypassButton);

    using APVTS = juce::AudioProcessorValueTreeState;
    saturationAttach = std::make_unique<APVTS::SliderAttachment> (p.apvts, "saturation", saturationSlider);
    outputAttach = std::make_unique<APVTS::SliderAttachment> (p.apvts, "outputGain", outputGainSlider);
    bypassAttach = std::make_unique<APVTS::ButtonAttachment> (p.apvts, "bypass", bypassButton);

    setResizable (true, true);
    setResizeLimits (900, 560, 1600, 1000);
    setSize (1100, 680);
}

HybridKickEQAudioProcessorEditor::~HybridKickEQAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void HybridKickEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (KickColours::background);
}

void HybridKickEQAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    auto header = area.removeFromTop (44);
    titleLabel.setBounds (header.removeFromLeft (260).reduced (12, 0));

    auto controlsWidth = 220;
    auto sideControls = header.removeFromRight (controlsWidth);
    bypassButton.setBounds (sideControls.removeFromLeft (80).reduced (4));

    auto satArea = sideControls.removeFromLeft (70);
    saturationLabel.setBounds (satArea.removeFromTop (14));
    saturationSlider.setBounds (satArea.reduced (2));

    auto outArea = sideControls;
    outputLabel.setBounds (outArea.removeFromTop (14));
    outputGainSlider.setBounds (outArea.reduced (2));

    latencyLabel.setBounds (header.reduced (8, 0));

    auto bottomPanel = area.removeFromBottom (170);
    bandPanel.setBounds (bottomPanel.reduced (4));

    curveComponent.setBounds (area.reduced (4));
}
