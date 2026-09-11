#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>

class LinearPhaseEQEngine
{
public:
    static constexpr int fftOrder = 10;
    static constexpr int fftSize  = 1 << fftOrder;       // 1024
    static constexpr int overlapFactor = 4;
    static constexpr int hopSize = fftSize / overlapFactor; // 256

    void prepare (double sr)
    {
        sampleRate = sr;
        fifo.assign ((size_t) fftSize, 0.0f);
        outputAccum.assign ((size_t) fftSize, 0.0f);
        fftWorkspace.assign ((size_t) (2 * fftSize), 0.0f);
        window.assign ((size_t) fftSize, 0.0f);

        // √Hann → mejor reconstrucción (análisis * síntesis = Hann)
        for (int i = 0; i < fftSize; ++i)
        {
            const float hann = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (fftSize - 1));
            window[(size_t) i] = std::sqrt (hann);
        }

        pos = 0;
        hopCounter = 0;
    }

    void reset()
    {
        std::fill (fifo.begin(), fifo.end(), 0.0f);
        std::fill (outputAccum.begin(), outputAccum.end(), 0.0f);
        pos = 0;
        hopCounter = 0;
    }

    int getLatencySamples() const noexcept { return fftSize; }

    float processSample (float inputSample, const std::array<float, fftSize / 2 + 1>& gainCurveLinear)
    {
        float outSample = outputAccum[(size_t) pos];
        outputAccum[(size_t) pos] = 0.0f;

        fifo[(size_t) pos] = inputSample;
        pos = (pos + 1) % fftSize;

        if (++hopCounter >= hopSize)
        {
            hopCounter = 0;
            processFrame (gainCurveLinear);
        }

        return outSample;
    }

private:
    void processFrame (const std::array<float, fftSize / 2 + 1>& gainCurveLinear)
    {
        // Análisis: aplicar √Hann
        for (int i = 0; i < fftSize; ++i)
        {
            const auto idx = (size_t) ((pos + i) % fftSize);
            fftWorkspace[(size_t) i] = fifo[idx] * window[(size_t) i];
        }

        for (int i = fftSize; i < 2 * fftSize; ++i)
            fftWorkspace[(size_t) i] = 0.0f;

        static juce::dsp::FFT fft (fftOrder);
        fft.performRealOnlyForwardTransform (fftWorkspace.data());

        // Ganancia espectral (solo bins 0 .. N/2)
        for (int bin = 0; bin <= fftSize / 2; ++bin)
        {
            const float g = gainCurveLinear[(size_t) bin];
            fftWorkspace[(size_t) (2 * bin)]     *= g;
            fftWorkspace[(size_t) (2 * bin + 1)] *= g;
        }

        fft.performRealOnlyInverseTransform (fftWorkspace.data());

        // Síntesis: aplicar √Hann otra vez + corrección
        // Con √Hann + √Hann el producto es Hann, y con 75% overlap la suma es ≈ 1.5
        constexpr float olaCorrection = 2.0f / 3.0f;

        for (int i = 0; i < fftSize; ++i)
        {
            const auto idx = (size_t) ((pos + i) % fftSize);
            outputAccum[idx] += fftWorkspace[(size_t) i] * window[(size_t) i] * olaCorrection;
        }
    }

    double sampleRate = 44100.0;
    std::vector<float> fifo, outputAccum, fftWorkspace, window;
    int pos = 0;
    int hopCounter = 0;
};
