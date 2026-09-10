#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "GUI/LookAndFeel.h"
#include "GUI/EQCurveComponent.h"
#include "GUI/BandPanel.h"

class HybridKickEQAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit HybridKickEQAudioProcessorEditor (HybridKickEQAudioProcessor&);
    ~HybridKickEQAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HybridKickEQAudioProcessor& processorRef;

    KickLookAndFeel kickLookAndFeel;

    EQCurveComponent curveComponent;
    BandPanel bandPanel;

    juce::Slider saturationSlider, outputGainSlider;
    juce::Label saturationLabel { {}, "Drive" }, outputLabel { {}, "Output" };
    juce::ToggleButton bypassButton { "Bypass" };
    juce::Label titleLabel { {}, "HYBRID KICK EQ" };
    juce::Label latencyLabel { {}, {} };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> saturationAttach, outputAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HybridKickEQAudioProcessorEditor)
};
