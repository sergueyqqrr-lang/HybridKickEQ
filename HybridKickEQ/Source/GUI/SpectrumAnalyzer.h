#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "../PluginProcessor.h"

class SpectrumAnalyzer : public juce::Component, private juce::Timer
{
public:
    explicit SpectrumAnalyzer (HybridKickEQAudioProcessor& proc);
    ~SpectrumAnalyzer() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    void pullFromFifo();
    void drawFrame();

    HybridKickEQAudioProcessor& processor;

    static constexpr int fftOrder = HybridKickEQAudioProcessor::fftOrder;
    static constexpr int fftSize  = HybridKickEQAudioProcessor::fftSize;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };

    std::array<float, fftSize * 2> fftData {};
    std::array<float, fftSize> fifoLocal {};
    int fifoIndex = 0;

    std::array<float, 512> scopeData {};
};
