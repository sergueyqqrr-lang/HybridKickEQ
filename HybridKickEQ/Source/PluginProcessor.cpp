#include "PluginProcessor.h"
#include "PluginEditor.h"

HybridKickEQAudioProcessor::HybridKickEQAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    setLatencySamples (0);
}

HybridKickEQAudioProcessor::~HybridKickEQAudioProcessor() {}

const juce::StringArray& HybridKickEQAudioProcessor::getBandNames()
{
    static juce::StringArray names { "High Pass", "Sub / Peso", "Cuerpo", "Golpe", "Aire" };
    return names;
}

juce::AudioProcessorValueTreeState::ParameterLayout HybridKickEQAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    struct Defaults { float freq, gain, q; bool active; int type; };
    const Defaults defaults[numBands] = {
        { 30.0f,   0.0f, 0.7f, true, (int) SpectralEQBand::Type::HighPass },
        { 60.0f,   3.0f, 1.2f, true, (int) SpectralEQBand::Type::Bell },
        { 300.0f, -3.0f, 1.0f, true, (int) SpectralEQBand::Type::Bell },
        { 3500.0f, 4.0f, 1.0f, true, (int) SpectralEQBand::Type::Bell },
        { 9000.0f, 2.0f, 0.7f, true, (int) SpectralEQBand::Type::HighShelf },
    };

    juce::StringArray typeChoices { "High Pass", "Bell", "Low Shelf", "High Shelf" };

    const auto& names = getBandNames();

    for (int i = 0; i < numBands; ++i)
    {
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            getBandActiveParamID (i), names[i] + " Active", defaults[i].active));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            getBandTypeParamID (i), names[i] + " Type", typeChoices, defaults[i].type));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            getBandFreqParamID (i), names[i] + " Freq",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f), defaults[i].freq));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            getBandGainParamID (i), names[i] + " Gain",
            juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), defaults[i].gain));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            getBandQParamID (i), names[i] + " Q",
            juce::NormalisableRange<float> (0.1f, 18.0f, 0.01f, 0.4f), defaults[i].q));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            getBandProportionalParamID (i), names[i] + " Proportional Q", true));
    }

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "saturation", "Analog Saturation", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "outputGain", "Output Gain", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> ("bypass", "Bypass", false));

    return { params.begin(), params.end() };
}

void HybridKickEQAudioProcessor::updateBandFromParameters (int i)
{
    bool active = apvts.getRawParameterValue (getBandActiveParamID (i))->load() > 0.5f;
    if (! active)
        return;

    int typeIdx = (int) apvts.getRawParameterValue (getBandTypeParamID (i))->load();
    float freq  = apvts.getRawParameterValue (getBandFreqParamID (i))->load();
    float gain  = apvts.getRawParameterValue (getBandGainParamID (i))->load();
    float q     = apvts.getRawParameterValue (getBandQParamID (i))->load();
    bool prop   = apvts.getRawParameterValue (getBandProportionalParamID (i))->load() > 0.5f;

    bands[(size_t) i].update ((SpectralEQBand::Type) typeIdx, freq, gain, q, prop);
}

float HybridKickEQAudioProcessor::getBandDbAt (int bandIndex, float freq) const
{
    bool active = apvts.getRawParameterValue (getBandActiveParamID (bandIndex))->load() > 0.5f;
    if (! active) return 0.0f;

    return bands[(size_t) bandIndex].getMagnitudeForFrequency (freq);
}

void HybridKickEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    setLatencySamples (0);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) getTotalNumOutputChannels();

    for (auto& band : bands)
        band.prepare (spec);

    saturator.prepare (spec);

    for (int i = 0; i < numBands; ++i)
        updateBandFromParameters (i);
}

void HybridKickEQAudioProcessor::releaseResources() {}

bool HybridKickEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

void HybridKickEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    bool bypass = apvts.getRawParameterValue ("bypass")->load() > 0.5f;

    for (int i = 0; i < numBands; ++i)
        updateBandFromParameters (i);

    if (! bypass)
    {
        juce::dsp::AudioBlock<float> block (buffer);

        for (int i = 0; i < numBands; ++i)
        {
            bool active = apvts.getRawParameterValue (getBandActiveParamID (i))->load() > 0.5f;
            if (active)
                bands[(size_t) i].process (block);
        }

        float drive = apvts.getRawParameterValue ("saturation")->load();
        saturator.process (block, drive);

        float outGainDb = apvts.getRawParameterValue ("outputGain")->load();
        block.multiplyBy (juce::Decibels::decibelsToGain (outGainDb));
    }

    auto numSamples = buffer.getNumSamples();
    auto* channelData = buffer.getReadPointer (0);
    auto scope = audioFifo.write (numSamples);

    if (scope.blockSize1 > 0)
        fifoBuffer.copyFrom (0, scope.startIndex1, channelData, scope.blockSize1);
    if (scope.blockSize2 > 0)
        fifoBuffer.copyFrom (0, scope.startIndex2, channelData + scope.blockSize1, scope.blockSize2);
}

juce::AudioProcessorEditor* HybridKickEQAudioProcessor::createEditor()
{
    return new HybridKickEQAudioProcessorEditor (*this);
}

void HybridKickEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void HybridKickEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HybridKickEQAudioProcessor();
}
