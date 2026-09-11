#pragma once
#include <juce_dsp/juce_dsp.h>

/**
    Motor de EQ de FASE LINEAL mediante convolucion rapida (overlap-add) con
    un filtro FIR disenado a partir de la curva de ganancia deseada.

    A diferencia de la version anterior (que multiplicaba el espectro
    directamente sin acotar el largo del filtro resultante -> generaba
    "envolvimiento circular" / distorsion), aqui:

    1) Se disena un filtro FIR de 'kernelLength' taps a partir de la curva
       de ganancia deseada (metodo estandar: IFFT -> centrar -> truncar ->
       ventanear), acotando explicitamente cuanto "dura" el filtro en tiempo.
    2) Se aplica mediante convolucion rapida overlap-add con un tamano de FFT
       (fftSize) suficientemente grande para que la convolucion sea LINEAL
       exacta, sin envolvimiento circular, sin importar cuantas bandas o que
       tan angostas sean.

    Esto es lo mismo que hacen los modos "Linear Phase" de EQs profesionales.
*/
class LinearPhaseEQEngine
{
public:
    static constexpr int fftOrder = 12;                 // 4096 puntos (M)
    static constexpr int fftSize = 1 << fftOrder;        // 4096
    static constexpr int blockSize = 1024;               // B: muestras nuevas por bloque
    static constexpr int kernelLength = 2047;            // L: taps del FIR (impar)

    using KernelSpectrum = std::array<float, 2 * fftSize>;

    void prepare (double sr)
    {
        sampleRate = sr;
        inputAccum.assign ((size_t) blockSize, 0.0f);
        inputCount = 0;
        outputAccum.assign ((size_t) fftSize, 0.0f);
        readPos = 0;
        frameWorkspace.assign ((size_t) (2 * fftSize), 0.0f);
    }

    void reset()
    {
        std::fill (inputAccum.begin(), inputAccum.end(), 0.0f);
        std::fill (outputAccum.begin(), outputAccum.end(), 0.0f);
        inputCount = 0;
        readPos = 0;
    }

    static constexpr int getLatencySamples() noexcept
    {
        return blockSize + (kernelLength - 1) / 2;
    }

    static KernelSpectrum designKernel (const std::array<float, fftSize / 2 + 1>& gainCurveLinear)
    {
        static juce::dsp::FFT fft (fftOrder);

        std::array<float, 2 * fftSize> workspace {};

        for (int bin = 0; bin <= fftSize / 2; ++bin)
        {
            workspace[(size_t) (2 * bin)] = gainCurveLinear[(size_t) bin];
            workspace[(size_t) (2 * bin + 1)] = 0.0f;

            if (bin > 0 && bin < fftSize / 2)
            {
                auto mirror = fftSize - bin;
                workspace[(size_t) (2 * mirror)] = gainCurveLinear[(size_t) bin];
                workspace[(size_t) (2 * mirror + 1)] = 0.0f;
            }
        }

        fft.performRealOnlyInverseTransform (workspace.data());

        std::array<float, fftSize> shifted {};
        for (int n = 0; n < fftSize; ++n)
            shifted[(size_t) n] = workspace[(size_t) ((n + fftSize / 2) % fftSize)];

        KernelSpectrum kernelBuffer {};
        int startIdx = fftSize / 2 - (kernelLength - 1) / 2;
        for (int n = 0; n < kernelLength; ++n)
        {
            auto w = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * n / (kernelLength - 1));
            kernelBuffer[(size_t) n] = shifted[(size_t) (startIdx + n)] * w;
        }

        fft.performRealOnlyForwardTransform (kernelBuffer.data());
        return kernelBuffer;
    }

    float processSample (float inputSample, const KernelSpectrum& kernelSpectrum)
    {
        float outSample = outputAccum[(size_t) readPos];
        outputAccum[(size_t) readPos] = 0.0f;
        readPos = (readPos + 1) % fftSize;

        inputAccum[(size_t) inputCount++] = inputSample;

        if (inputCount >= blockSize)
        {
            inputCount = 0;
            processFrame (kernelSpectrum);
        }

        return outSample;
    }

private:
    void processFrame (const KernelSpectrum& kernelSpectrum)
    {
        static juce::dsp::FFT fft (fftOrder);

        std::fill (frameWorkspace.begin(), frameWorkspace.end(), 0.0f);
        for (int n = 0; n < blockSize; ++n)
            frameWorkspace[(size_t) n] = inputAccum[(size_t) n];

        fft.performRealOnlyForwardTransform (frameWorkspace.data());

        for (int bin = 0; bin < fftSize; ++bin)
        {
            auto xr = frameWorkspace[(size_t) (2 * bin)];
            auto xi = frameWorkspace[(size_t) (2 * bin + 1)];
            auto hr = kernelSpectrum[(size_t) (2 * bin)];
            auto hi = kernelSpectrum[(size_t) (2 * bin + 1)];

            frameWorkspace[(size_t) (2 * bin)]     = xr * hr - xi * hi;
            frameWorkspace[(size_t) (2 * bin + 1)] = xr * hi + xi * hr;
        }

        fft.performRealOnlyInverseTransform (frameWorkspace.data());

        for (int n = 0; n < fftSize; ++n)
        {
            auto idx = (size_t) ((readPos + n) % fftSize);
            outputAccum[idx] += frameWorkspace[(size_t) n];
        }
    }

    double sampleRate = 44100.0;
    std::vector<float> inputAccum;
    int inputCount = 0;
    std::vector<float> outputAccum;
    int readPos = 0;
    std::vector<float> frameWorkspace;
};
