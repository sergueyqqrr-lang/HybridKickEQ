#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/SpectralEQBand.h"
#include "DSP/AnalogSaturator.h"

class HybridKickEQAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numBands = 5; // High Pass, Sub, Cuerpo, Golpe, Aire

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

    std::array<SpectralEQBand, numBands>& getBands() { return bands; }
    double getCurrentSampleRate() const { return currentSampleRate; }
    float getBandDbAt (int bandIndex, float freq) const;
    static const juce::StringArray& getBandNames();

    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    juce::AbstractFifo audioFifo { fftSize * 4 };
    juce::AudioBuffer<float> fifoBuffer { 1, fftSize * 4 };

    static juce::String getBandActiveParamID (int i)       { return "band" + juce::String (i) + "_active"; }
    static juce::String getBandTypeParamID (int i)          { return "band" + juce::String (i) + "_type"; }
    static juce::String getBandFreqParamID (int i)          { return "band" + juce::String (i) + "_freq"; }
    static juce::String getBandGainParamID (int i)          { return "band" + juce::String (i) + "_gain"; }
    static juce::String getBandQParamID (int i)             { return "band" + juce::String (i) + "_q"; }
    static juce::String getBandProportionalParamID (int i)  { return "band" + juce::String (i) + "_prop"; }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateBandFromParameters (int index);

    std::array<SpectralEQBand, numBands> bands;
    AnalogSaturator saturator;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HybridKickEQAudioProcessor)
};
