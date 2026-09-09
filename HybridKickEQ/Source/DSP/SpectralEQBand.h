#pragma once
#include <juce_dsp/juce_dsp.h>
#include "STFTProcessor.h"

/**
    Cada banda se define como una función matemática pura de la frecuencia
    (no como un filtro físico), lo que garantiza aislamiento real: la
    contribución de una banda en una frecuencia lejana a su centro es
    exactamente 0dB, sin importar la ganancia de las demás bandas.
*/
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

    static float getEffectiveQ (float baseQ, float gainDb, bool proportional)
    {
        if (! proportional) return baseQ;
        auto factor = 1.0f + (std::abs (gainDb) / 24.0f) * 2.0f;
        return juce::jlimit (0.1f, 18.0f, baseQ * factor);
    }

    // Devuelve la ganancia en dB que aporta ESTA banda a una frecuencia dada
    static float getDbAt (Type type, float freq, float gainDb, float q, bool proportional, float atFreq)
    {
        auto effectiveQ = getEffectiveQ (q, gainDb, proportional);

        switch (type)
        {
            case Type::Bell:
            {
                // Campana gaussiana en dominio log-frecuencia. El ancho en octavas
                // se deriva de Q de forma aproximada (est\u00e1ndar: BW(oct) ~ 2/Q).
                auto bwOctaves = juce::jmax (0.05f, 2.0f / effectiveQ);
                auto sigma = bwOctaves * 0.5f;
                auto distOct = std::log2 (juce::jmax (1.0f, atFreq) / freq);
                auto shape = std::exp (-0.5f * (distOct * distOct) / (sigma * sigma));
                return gainDb * shape;
            }
            case Type::LowShelf:
            {
                auto bwOctaves = juce::jmax (0.1f, 2.0f / effectiveQ);
                auto t = std::log2 (juce::jmax (1.0f, atFreq) / freq) / bwOctaves;
                auto shape = 1.0f / (1.0f + std::exp (t * 4.0f)); // sigmoide: 1 debajo, 0 arriba
                return gainDb * shape;
            }
            case Type::HighShelf:
            {
                auto bwOctaves = juce::jmax (0.1f, 2.0f / effectiveQ);
                auto t = std::log2 (juce::jmax (1.0f, atFreq) / freq) / bwOctaves;
                auto shape = 1.0f / (1.0f + std::exp (-t * 4.0f)); // sigmoide: 0 debajo, 1 arriba
                return gainDb * shape;
            }
            case Type::HighPass:
            {
                // Pendiente ~24dB/oct, magnitud pura (no usa gainDb, siempre corta)
                auto ratio = freq / juce::jmax (1.0f, atFreq);
                auto magnitudeSquared = 1.0f / (1.0f + std::pow (ratio, 8.0f));
                return 10.0f * std::log10 (juce::jmax (1.0e-8f, magnitudeSquared));
            }
        }
        return 0.0f;
    }
};
