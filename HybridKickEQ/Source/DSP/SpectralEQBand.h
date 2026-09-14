#pragma once
#include <juce_dsp/juce_dsp.h>

class SpectralEQBand
{
public:
    enum class Type
    {
        HighPass = 0,
        Bell,
        LowShelf,
        HighShelf
    };

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        for (auto& f : filters)
            f.prepare (spec);
        sampleRate = spec.sampleRate;
    }

    void reset()
    {
        for (auto& f : filters)
            f.reset();
    }

    static float getEffectiveQ (float baseQ, float gainDb, bool proportional)
    {
        if (! proportional)
            return baseQ;

        auto factor = 1.0f + (std::abs (gainDb) / 24.0f) * 2.0f;
        return juce::jlimit (0.1f, 18.0f, baseQ * factor);
    }

    void update (Type type, float freqHz, float gainDb, float q, bool proportionalQ)
    {
        currentType = type;
        currentFreq = freqHz;
        currentGainDb = gainDb;
        currentQ = q;
        currentProportional = proportionalQ;

        auto effectiveQ = getEffectiveQ (q, gainDb, proportionalQ);
        auto gainLinear = juce::Decibels::decibelsToGain (gainDb);

        juce::dsp::IIR::Coefficients<float>::Ptr coeffs;

        switch (type)
        {
            case Type::HighPass:
                coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, freqHz, effectiveQ);
                break;
            case Type::Bell:
                coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, freqHz, effectiveQ, gainLinear);
                break;
            case Type::LowShelf:
                coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf (sampleRate, freqHz, effectiveQ, gainLinear);
                break;
            case Type::HighShelf:
                coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, freqHz, effectiveQ, gainLinear);
                break;
        }

        currentCoefficients = coeffs;
        for (auto& f : filters)
            f.coefficients = coeffs;
    }

    void process (juce::dsp::AudioBlock<float>& block)
    {
        for (size_t ch = 0; ch < block.getNumChannels() && ch < filters.size(); ++ch)
        {
            auto singleChannel = block.getSingleChannelBlock (ch);
            juce::dsp::ProcessContextReplacing<float> ctx (singleChannel);
            filters[ch].process (ctx);
        }
    }

    float getMagnitudeForFrequency (double freqHz) const
    {
        if (currentCoefficients == nullptr)
            return 0.0f;

        return (float) juce::Decibels::gainToDecibels (
            currentCoefficients->getMagnitudeForFrequency (freqHz, sampleRate));
    }

    Type getType() const noexcept          { return currentType; }
    float getFrequency() const noexcept    { return currentFreq; }
    float getGainDb() const noexcept       { return currentGainDb; }
    float getQ() const noexcept            { return currentQ; }
    bool isProportional() const noexcept   { return currentProportional; }

private:
    std::array<juce::dsp::IIR::Filter<float>, 2> filters;
    juce::dsp::IIR::Coefficients<float>::Ptr currentCoefficients;
    double sampleRate = 44100.0;

    Type currentType = Type::Bell;
    float currentFreq = 1000.0f;
    float currentGainDb = 0.0f;
    float currentQ = 1.0f;
    bool currentProportional = true;
};
