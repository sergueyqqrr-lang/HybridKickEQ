#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../PluginProcessor.h"
#include "SpectrumAnalyzer.h"

class EQCurveComponent : public juce::Component, private juce::Timer
{
public:
    explicit EQCurveComponent (HybridKickEQAudioProcessor& proc);
    ~EQCurveComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override;

    float freqToX (float freq) const;
    float xToFreq (float x) const;
    float gainToY (float gain) const;
    float yToGain (float y) const;

    int findNodeUnder (juce::Point<float> pos) const;
    void drawGrid (juce::Graphics&);
    void drawResponseCurve (juce::Graphics&);
    void drawNodes (juce::Graphics&);

    HybridKickEQAudioProcessor& processor;
    SpectrumAnalyzer spectrumAnalyzer;

    int draggingBand = -1;
    juce::Rectangle<float> plotArea;

    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr float minGain = -24.0f;
    static constexpr float maxGain = 24.0f;
};
