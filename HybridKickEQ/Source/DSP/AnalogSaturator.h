#pragma once
#include <juce_dsp/juce_dsp.h>

/**
    Etapa de saturación suave que simula el "calor" de una consola analógica.
    Se aplica DESPUÉS del núcleo de fase lineal, así que no afecta el
    aislamiento de las bandas — solo agrega color armónico global.
*/
class AnalogSaturator
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
            spec.numChannels, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
        oversampler->initProcessing (spec.maximumBlockSize);

        waveShaper.functionToUse = [] (float x)
        {
            auto y = std::tanh (x * 1.5f);
            auto asym = 0.08f * x * x * (x < 0.0f ? -1.0f : 1.0f);
            return y + asym;
        };
    }

    void reset()
    {
        if (oversampler)
            oversampler->reset();
    }

    void process (juce::dsp::AudioBlock<float>& block, float drive)
    {
        if (drive <= 0.001f || oversampler == nullptr)
            return;

        auto driveGain = juce::jmap (drive, 0.0f, 1.0f, 1.0f, 3.0f);
        auto makeupGain = 1.0f / std::sqrt (driveGain);

        block.multiplyBy (driveGain);

        auto oversampledBlock = oversampler->processSamplesUp (block);
        juce::dsp::ProcessContextReplacing<float> ctx (oversampledBlock);
        waveShaper.process (ctx);
        oversampler->processSamplesDown (block);

        block.multiplyBy (makeupGain);
    }

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::dsp::WaveShaper<float> waveShaper;
};
