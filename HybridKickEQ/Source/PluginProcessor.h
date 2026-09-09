#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/SpectralEQBand.h"
#include "DSP/STFTProcessor.h"
#include "DSP/AnalogSaturator.h"

class HybridKickEQAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numBands = 5; // HP, Sub, Cuerpo, Golpe, Aire

    HybridKickEQAudioProcessor();
    ~HybridKickEQAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Hybrid Kick EQ"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    double getCurrentSampleRate() const { return currentSampleRate; }
    float getBandDbAt (int bandIndex, float freq) const;
    static const juce::StringArray& getBandNames();

    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    juce::AbstractFifo audioFifo { fftSize * 4 };
    juce::AudioBuffer<float> fifoBuffer { 1, fftSize * 4 };

    static juce::String getBandActiveParamID (int i)       { return "band" + juce::String (i) + "_active"; }
    static juce::String getBandFreqParamID (int i)          { return "band" + juce::String (i) + "_freq"; }
    static juce::String getBandGainParamID (int i)          { return "band" + juce::String (i) + "_gain"; }
    static juce::String getBandQParamID (int i)             { return "band" + juce::String (i) + "_q"; }
    static juce::String getBandProportionalParamID (int i)  { return "band" + juce::String (i) + "_prop"; }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void computeGainCurve();

    std::array<SpectralEQBand::Type, numBands> bandTypes {
        SpectralEQBand::Type::HighPass, SpectralEQBand::Type::Bell,
        SpectralEQBand::Type::Bell, SpectralEQBand::Type::Bell,
        SpectralEQBand::Type::HighShelf
    };

    std::array<LinearPhaseEQEngine, 2> engines; // uno por canal (hasta estéreo)
    std::array<float, LinearPhaseEQEngine::fftSize / 2 + 1> gainCurveLinear {};

    AnalogSaturator saturator;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HybridKickEQAudioProcessor)
};
