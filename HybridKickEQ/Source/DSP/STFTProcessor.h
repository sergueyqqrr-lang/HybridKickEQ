#pragma once
#include <juce_dsp/juce_dsp.h>

/**
    Motor de EQ de FASE LINEAL basado en FFT (overlap-add / STFT).
    A diferencia de un EQ tradicional (cascada de filtros IIR), aquí NO hay
    "bandas" físicas que interactúen entre sí: se calcula UNA sola curva de
    ganancia (en dB) sumando la contribución de todas las bandas activas en
    cada frecuencia, y esa curva se aplica UNA vez, directamente sobre el
    espectro real de la señal. Esto significa que si tu banda de "Golpe" en
    3.5kHz no tiene ninguna contribución matemática en 60Hz, esa frecuencia
    queda exactamente en 0dB sin importar qué hagas con las otras bandas.
    Es la misma técnica usada en modos "Linear Phase" de EQs quirúrgicos
    profesionales. El costo es latencia (retraso), inherente a cualquier
    EQ de fase lineal, sea plugin o hardware.
*/
class LinearPhaseEQEngine
{
public:
    static constexpr int fftOrder = 10;                  // 1024 puntos
    static constexpr int fftSize  = 1 << fftOrder;       // 1024
    static constexpr int overlapFactor = 4;              // 75% overlap
    static constexpr int hopSize = fftSize / overlapFactor; // 256

    void prepare (double sr)
    {
        sampleRate = sr;
        fifo.assign ((size_t) fftSize, 0.0f);
        outputAccum.assign ((size_t) fftSize, 0.0f);
        fftWorkspace.assign ((size_t) (2 * fftSize), 0.0f);
        window.assign ((size_t) fftSize, 0.0f);

        // Ventana Hann
        for (int i = 0; i < fftSize; ++i)
            window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (fftSize - 1));

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

    // gainCurveLinear: ganancia LINEAL (no dB) para cada bin de 0 .. fftSize/2 inclusive
    float processSample (float inputSample, const std::array<float, fftSize / 2 + 1>& gainCurveLinear)
    {
        float outSample = outputAccum[(size_t) pos];
        outputAccum[(size_t) pos] = 0.0f; // limpiar para la próxima acumulación

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
        // 1. Copia circular + ventana de análisis (Hann)
        for (int i = 0; i < fftSize; ++i)
        {
            const auto idx = (size_t) ((pos + i) % fftSize);
            fftWorkspace[(size_t) i] = fifo[idx] * window[(size_t) i];
        }

        // Rellenar la segunda mitad con ceros
        for (int i = fftSize; i < 2 * fftSize; ++i)
            fftWorkspace[(size_t) i] = 0.0f;

        static juce::dsp::FFT fft (fftOrder);
        fft.performRealOnlyForwardTransform (fftWorkspace.data());

        // 2. Aplicar la curva de ganancia SOLO a los bins 0 .. N/2
        //    (JUCE real-only FFT solo guarda la mitad positiva del espectro)
        for (int bin = 0; bin <= fftSize / 2; ++bin)
        {
            const float g = gainCurveLinear[(size_t) bin];
            fftWorkspace[(size_t) (2 * bin)]     *= g;   // real
            fftWorkspace[(size_t) (2 * bin + 1)] *= g;   // imag
        }

        // 3. Transformada inversa
        fft.performRealOnlyInverseTransform (fftWorkspace.data());

        // 4. Overlap-add con ventana de síntesis + corrección de nivel
        //    Hann + 75% overlap → suma de ventanas = 1.5 → factor = 2/3
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
