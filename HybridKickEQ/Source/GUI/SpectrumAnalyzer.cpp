#include "SpectrumAnalyzer.h"
#include "LookAndFeel.h"

SpectrumAnalyzer::SpectrumAnalyzer (HybridKickEQAudioProcessor& proc) : processor (proc)
{
    scopeData.fill (0.0f);
    startTimerHz (30);
    setInterceptsMouseClicks (false, false);
}

SpectrumAnalyzer::~SpectrumAnalyzer() { stopTimer(); }

void SpectrumAnalyzer::timerCallback()
{
    pullFromFifo();
    repaint();
}

void SpectrumAnalyzer::pullFromFifo()
{
    auto numReady = processor.audioFifo.getNumReady();
    if (numReady <= 0) return;

    auto scope = processor.audioFifo.read (numReady);
    auto copyChunk = [this] (int start, int count)
    {
        for (int i = 0; i < count; ++i)
        {
            if (fifoIndex < fftSize)
                fifoLocal[(size_t) fifoIndex++] = processor.fifoBuffer.getSample (0, start + i);

            if (fifoIndex == fftSize)
            {
                fftData.fill (0.0f);
                std::copy (fifoLocal.begin(), fifoLocal.end(), fftData.begin());
                window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
                fft.performFrequencyOnlyForwardTransform (fftData.data());
                drawFrame();
                fifoIndex = 0;
            }
        }
    };

    if (scope.blockSize1 > 0) copyChunk (scope.startIndex1, scope.blockSize1);
    if (scope.blockSize2 > 0) copyChunk (scope.startIndex2, scope.blockSize2);
}

void SpectrumAnalyzer::drawFrame()
{
    auto sr = processor.getCurrentSampleRate();
    auto numBins = (size_t) scopeData.size();

    for (size_t i = 0; i < numBins; ++i)
    {
        auto prop = (float) i / (float) (numBins - 1);
        auto freq = 20.0f * std::pow (1000.0f, prop);
        auto fftBin = juce::jlimit (0, fftSize / 2 - 1, (int) (freq / (float) sr * fftSize));

        auto level = fftData[(size_t) fftBin];
        auto db = juce::Decibels::gainToDecibels (level, -100.0f);
        auto normalised = juce::jmap (db, -100.0f, 0.0f, 0.0f, 1.0f);

        scopeData[i] = juce::jmax (normalised, scopeData[i] * 0.82f);
    }
}

void SpectrumAnalyzer::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (8.0f, 8.0f);
    auto numBins = scopeData.size();

    juce::Path path;
    for (size_t i = 0; i < numBins; ++i)
    {
        auto x = bounds.getX() + (float) i / (float) (numBins - 1) * bounds.getWidth();
        auto y = bounds.getBottom() - scopeData[i] * bounds.getHeight() * 0.85f;
        if (i == 0) path.startNewSubPath (x, bounds.getBottom());
        path.lineTo (x, y);
    }
    path.lineTo (bounds.getRight(), bounds.getBottom());
    path.closeSubPath();

    g.setColour (KickColours::spectrum.withAlpha (0.35f));
    g.fillPath (path);
}
