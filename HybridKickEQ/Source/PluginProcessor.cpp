#include "PluginProcessor.h"
#include "PluginEditor.h"

HybridKickEQAudioProcessor::HybridKickEQAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    setLatencySamples (LinearPhaseEQEngine::getLatencySamples());
}

HybridKickEQAudioProcessor::~HybridKickEQAudioProcessor() {}

const juce::StringArray& HybridKickEQAudioProcessor::getBandNames()
{
    static juce::StringArray names {
        "High Pass", "Sub / Peso", "Cuerpo", "Golpe", "Aire",
        "Extra 1", "Extra 2", "Extra 3", "Extra 4", "Extra 5"
    };
    return names;
}

juce::AudioProcessorValueTreeState::ParameterLayout HybridKickEQAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    struct Defaults { float freq, gain, q; bool active; int type; };
    const Defaults defaults[numBands] = {
        { 30.0f,   0.0f, 0.7f, true,  (int) SpectralEQBand::Type::HighPass },
        { 60.0f,   3.0f, 1.2f, true,  (int) SpectralEQBand::Type::Bell },
        { 300.0f, -3.0f, 1.0f, true,  (int) SpectralEQBand::Type::Bell },
        { 3500.0f, 4.0f, 1.0f, true,  (int) SpectralEQBand::Type::Bell },
        { 9000.0f, 2.0f, 0.7f, true,  (int) SpectralEQBand::Type::HighShelf },
        { 1000.0f, 0.0f, 1.0f, false, (int) SpectralEQBand::Type::Bell },
        { 1000.0f, 0.0f, 1.0f, false, (int) SpectralEQBand::Type::Bell },
        { 1000.0f, 0.0f, 1.0f, false, (int) SpectralEQBand::Type::Bell },
        { 1000.0f, 0.0f, 1.0f, false, (int) SpectralEQBand::Type::Bell },
        { 1000.0f, 0.0f, 1.0f, false, (int) SpectralEQBand::Type::Bell },
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

float HybridKickEQAudioProcessor::getBandDbAt (int bandIndex, float freq) const
{
    auto activeId = getBandActiveParamID (bandIndex);
    bool active = apvts.getRawParameterValue (activeId)->load() > 0.5f;
    if (! active) return 0.0f;

    float f = apvts.getRawParameterValue (getBandFreqParamID (bandIndex))->load();
    float g = apvts.getRawParameterValue (getBandGainParamID (bandIndex))->load();
    float q = apvts.getRawParameterValue (getBandQParamID (bandIndex))->load();
    bool prop = apvts.getRawParameterValue (getBandProportionalParamID (bandIndex))->load() > 0.5f;
    int typeIdx = (int) apvts.getRawParameterValue (getBandTypeParamID (bandIndex))->load();

    return SpectralEQBand::getDbAt ((SpectralEQBand::Type) typeIdx, f, g, q, prop, freq);
}

void HybridKickEQAudioProcessor::computeGainCurve()
{
    auto binWidth = (float) (currentSampleRate / LinearPhaseEQEngine::fftSize);
    constexpr int numBins = LinearPhaseEQEngine::fftSize / 2 + 1;

    std::array<float, numBins> rawDb {};

    for (int bin = 0; bin < numBins; ++bin)
    {
        auto freq = juce::jmax (1.0f, bin * binWidth);
        float totalDb = 0.0f;

        for (int b = 0; b < numBands; ++b)
            totalDb += getBandDbAt (b, freq);

        rawDb[(size_t) bin] = juce::jlimit (-60.0f, 24.0f, totalDb);
    }

    for (int bin = 0; bin < numBins; ++bin)
    {
        auto prev = rawDb[(size_t) juce::jmax (0, bin - 1)];
        auto curr = rawDb[(size_t) bin];
        auto next = rawDb[(size_t) juce::jmin (numBins - 1, bin + 1)];
        auto smoothedDb = (prev + 2.0f * curr + next) / 4.0f;

        gainCurveLinear[(size_t) bin] = juce::Decibels::decibelsToGain (smoothedDb);
    }
}

void HybridKickEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    setLatencySamples (LinearPhaseEQEngine::getLatencySamples());

    for (auto& e : engines)
    {
        e.prepare (sampleRate);
        e.reset();
    }

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) getTotalNumOutputChannels();
    saturator.prepare (spec);

    computeGainCurve();
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

    auto numChannels = juce::jmin (buffer.getNumChannels(), 2);
    auto numSamples = buffer.getNumSamples();

    if (! bypass)
    {
        computeGainCurve();
        auto kernelSpectrum = LinearPhaseEQEngine::designKernel (gainCurveLinear);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                data[i] = engines[(size_t) ch].processSample (data[i], kernelSpectrum);
        }

        juce::dsp::AudioBlock<float> block (buffer);
        float drive = apvts.getRawParameterValue ("saturation")->load();
        saturator.process (block, drive);

        float outGainDb = apvts.getRawParameterValue ("outputGain")->load();
        block.multiplyBy (juce::Decibels::decibelsToGain (outGainDb));
    }

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
