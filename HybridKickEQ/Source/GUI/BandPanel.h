#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../PluginProcessor.h"

class BandStrip : public juce::Component
{
public:
    BandStrip (HybridKickEQAudioProcessor& proc, int bandIndex);

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    HybridKickEQAudioProcessor& processor;
    int index;

    juce::ToggleButton activeButton { "On" };
    juce::Slider freqSlider, gainSlider, qSlider;
    juce::ToggleButton proportionalButton { "Prop Q" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttach, gainAttach, qAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> activeAttach, propAttach;
};

class BandPanel : public juce::Component
{
public:
    explicit BandPanel (HybridKickEQAudioProcessor& proc);

    void resized() override;

private:
    juce::Viewport viewport;
    juce::Component content;
    juce::OwnedArray<BandStrip> strips;
};
