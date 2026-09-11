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
    static constexpr int fftOrder = 11;                  // 2048 puntos: mejor resolución en graves
    static constexpr int fftSize = 1 << fftOrder;        // 2048
    static constexpr int overlapFactor = 4;               // 75% overlap
    static constexpr int hopSize = fftSize / overlapFactor; // 256

    void prepare (double sr)
    {
        sampleRate = sr;
        fifo.assign ((size_t) fftSize, 0.0f);
        outputAccum.assign ((size_t) fftSize, 0.0f);
        fftWorkspace.assign ((size_t) (2 * fftSize), 0.0f);
        window.assign ((size_t) fftSize, 0.0f);

        // Ventana Hann estándar (analysis-only), correcta para 75% overlap
        for (int i = 0; i < fftSize; ++i)
            window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / (fftSize - 1));

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
        // Copia la ventana de análisis en orden circular correcto, aplicando la ventana Hann
        for (int i = 0; i < fftSize; ++i)
        {
            auto idx = (size_t) ((pos + i) % fftSize);
            fftWorkspace[(size_t) i] = fifo[idx] * window[(size_t) i];
        }
        for (int i = fftSize; i < 2 * fftSize; ++i)
            fftWorkspace[(size_t) i] = 0.0f;

        static juce::dsp::FFT fft (fftOrder);
        fft.performRealOnlyForwardTransform (fftWorkspace.data());

        // fftWorkspace ahora contiene bins complejos empaquetados: [re0, im0, re1, im1, ...]
        // Aplica la curva de ganancia (real, positiva) respetando simetría conjugada
        for (int bin = 0; bin <= fftSize / 2; ++bin)
        {
            auto g = gainCurveLinear[(size_t) bin];
            fftWorkspace[(size_t) (2 * bin)]     *= g;
            fftWorkspace[(size_t) (2 * bin + 1)] *= g;

            if (bin > 0 && bin < fftSize / 2)
            {
                auto mirror = fftSize - bin;
                fftWorkspace[(size_t) (2 * mirror)]     *= g;
                fftWorkspace[(size_t) (2 * mirror + 1)] *= g;
            }
        }

        fft.performRealOnlyInverseTransform (fftWorkspace.data());

        // Overlap-add: acumula el resultado SIN volver a aplicar la ventana
        // (la ventana ya se aplicó una sola vez en el análisis, antes del FFT;
        // aplicarla de nuevo aquí duplicaba el "taper" y rompía la reconstrucción,
        // causando modulación de amplitud -> distorsión al pasar por la saturación).
        // Corrección de nivel para 75% overlap con ventana Hann (constante COLA = 1.5)
        constexpr float olaCorrection = 1.0f / 1.5f;

        for (int i = 0; i < fftSize; ++i)
        {
            auto idx = (size_t) ((pos + i) % fftSize);
            outputAccum[idx] += fftWorkspace[(size_t) i] * olaCorrection;
        }
    }

    double sampleRate = 44100.0;
    std::vector<float> fifo, outputAccum, fftWorkspace, window;
    int pos = 0;
    int hopCounter = 0;
};
