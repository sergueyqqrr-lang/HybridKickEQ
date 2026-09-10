#include "EQCurveComponent.h"
#include "LookAndFeel.h"
#include "../DSP/SpectralEQBand.h"

EQCurveComponent::EQCurveComponent (HybridKickEQAudioProcessor& proc)
    : processor (proc), spectrumAnalyzer (proc)
{
    addAndMakeVisible (spectrumAnalyzer);
    startTimerHz (30);
}

EQCurveComponent::~EQCurveComponent() { stopTimer(); }

void EQCurveComponent::resized()
{
    plotArea = getLocalBounds().toFloat().reduced (8.0f, 8.0f);
    spectrumAnalyzer.setBounds (getLocalBounds());
}

void EQCurveComponent::timerCallback() { repaint(); }

float EQCurveComponent::freqToX (float freq) const
{
    auto logMin = std::log10 (minFreq);
    auto logMax = std::log10 (maxFreq);
    auto logF = std::log10 (juce::jlimit (minFreq, maxFreq, freq));
    return plotArea.getX() + (logF - logMin) / (logMax - logMin) * plotArea.getWidth();
}

float EQCurveComponent::xToFreq (float x) const
{
    auto logMin = std::log10 (minFreq);
    auto logMax = std::log10 (maxFreq);
    auto prop = juce::jlimit (0.0f, 1.0f, (x - plotArea.getX()) / plotArea.getWidth());
    return std::pow (10.0f, logMin + prop * (logMax - logMin));
}

float EQCurveComponent::gainToY (float gain) const
{
    auto prop = (gain - minGain) / (maxGain - minGain);
    return plotArea.getBottom() - prop * plotArea.getHeight();
}

float EQCurveComponent::yToGain (float y) const
{
    auto prop = juce::jlimit (0.0f, 1.0f, (plotArea.getBottom() - y) / plotArea.getHeight());
    return minGain + prop * (maxGain - minGain);
}

void EQCurveComponent::paint (juce::Graphics& g)
{
    g.fillAll (KickColours::background);
    drawGrid (g);
    drawResponseCurve (g);
    drawNodes (g);
}

void EQCurveComponent::drawGrid (juce::Graphics& g)
{
    const float freqLines[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    g.setFont (10.0f);

    for (auto f : freqLines)
    {
        auto x = freqToX (f);
        bool major = (f == 100 || f == 1000 || f == 10000);
        g.setColour (major ? KickColours::gridLineMain : KickColours::gridLine);
        g.drawVerticalLine ((int) x, plotArea.getY(), plotArea.getBottom());

        g.setColour (KickColours::textDim);
        juce::String label = f >= 1000 ? juce::String (f / 1000.0f, (f == 1000 || f == 10000) ? 0 : 1) + "k"
                                        : juce::String ((int) f);
        g.drawText (label, (int) x - 15, (int) plotArea.getBottom() - 14, 30, 12, juce::Justification::centred);
    }

    for (float gdb = minGain; gdb <= maxGain; gdb += 6.0f)
    {
        auto y = gainToY (gdb);
        g.setColour (juce::approximatelyEqual (gdb, 0.0f) ? KickColours::gridLineMain : KickColours::gridLine);
        g.drawHorizontalLine ((int) y, plotArea.getX(), plotArea.getRight());

        g.setColour (KickColours::textDim);
        g.drawText (juce::String ((int) gdb), (int) plotArea.getX() + 2, (int) y - 12, 30, 12, juce::Justification::left);
    }
}

void EQCurveComponent::drawResponseCurve (juce::Graphics& g)
{
    juce::Path path;
    const int numPoints = 300;

    for (int i = 0; i < numPoints; ++i)
    {
        float prop = (float) i / (float) (numPoints - 1);
        float x = plotArea.getX() + prop * plotArea.getWidth();
        float freq = xToFreq (x);

        float totalDb = 0.0f;
        for (int b = 0; b < HybridKickEQAudioProcessor::numBands; ++b)
            totalDb += processor.getBandDbAt (b, freq);

        float y = gainToY (juce::jlimit (minGain, maxGain, totalDb));
        if (i == 0) path.startNewSubPath (x, y);
        else path.lineTo (x, y);
    }

    g.setColour (KickColours::curve);
    g.strokePath (path, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path fillPath = path;
    fillPath.lineTo (plotArea.getRight(), gainToY (0.0f));
    fillPath.lineTo (plotArea.getX(), gainToY (0.0f));
    fillPath.closeSubPath();
    g.setColour (KickColours::curve.withAlpha (0.08f));
    g.fillPath (fillPath);
}

void EQCurveComponent::drawNodes (juce::Graphics& g)
{
    const auto& names = HybridKickEQAudioProcessor::getBandNames();

    for (int b = 0; b < HybridKickEQAudioProcessor::numBands; ++b)
    {
        bool active = processor.apvts.getRawParameterValue (
            HybridKickEQAudioProcessor::getBandActiveParamID (b))->load() > 0.5f;
        if (! active) continue;

        auto freq = processor.apvts.getRawParameterValue (HybridKickEQAudioProcessor::getBandFreqParamID (b))->load();
        auto gain = processor.apvts.getRawParameterValue (HybridKickEQAudioProcessor::getBandGainParamID (b))->load();

        auto x = freqToX (freq);
        auto y = gainToY (gain);

        auto colour = juce::Colour::fromHSV ((float) b / (float) HybridKickEQAudioProcessor::numBands,
                                              0.6f, 1.0f, 1.0f);
        bool isDragging = (draggingBand == b);
        float radius = isDragging ? 9.0f : 7.0f;

        g.setColour (colour.withAlpha (0.25f));
        g.fillEllipse (x - radius - 3, y - radius - 3, (radius + 3) * 2, (radius + 3) * 2);

        g.setColour (colour);
        g.fillEllipse (x - radius, y - radius, radius * 2, radius * 2);
        g.setColour (KickColours::background);
        g.drawEllipse (x - radius, y - radius, radius * 2, radius * 2, 1.5f);

        g.setColour (colour);
        g.setFont (10.0f);
        g.drawText (names[b], (int) x - 30, (int) y - 22, 60, 14, juce::Justification::centred);
    }
}

int EQCurveComponent::findNodeUnder (juce::Point<float> pos) const
{
    for (int b = 0; b < HybridKickEQAudioProcessor::numBands; ++b)
    {
        bool active = processor.apvts.getRawParameterValue (
            HybridKickEQAudioProcessor::getBandActiveParamID (b))->load() > 0.5f;
        if (! active) continue;

        auto freq = processor.apvts.getRawParameterValue (HybridKickEQAudioProcessor::getBandFreqParamID (b))->load();
        auto gain = processor.apvts.getRawParameterValue (HybridKickEQAudioProcessor::getBandGainParamID (b))->load();

        auto x = freqToX (freq);
        auto y = gainToY (gain);
        if (pos.getDistanceFrom ({ x, y }) < 12.0f)
            return b;
    }
    return -1;
}

void EQCurveComponent::mouseDown (const juce::MouseEvent& e)
{
    auto node = findNodeUnder (e.position);

    if (e.mods.isRightButtonDown() && node >= 0)
    {
        juce::PopupMenu menu;
        menu.addItem (1, "High Pass");
        menu.addItem (2, "Bell");
        menu.addItem (3, "Low Shelf");
        menu.addItem (4, "High Shelf");
        menu.addSeparator();

        bool prop = processor.apvts.getRawParameterValue (
            HybridKickEQAudioProcessor::getBandProportionalParamID (node))->load() > 0.5f;
        menu.addItem (5, "Proportional Q (estilo API)", true, prop);
        menu.addSeparator();
        menu.addItem (6, "Desactivar banda");

        menu.showMenuAsync (juce::PopupMenu::Options(), [this, node] (int result)
        {
            if (result >= 1 && result <= 4)
            {
                auto id = HybridKickEQAudioProcessor::getBandTypeParamID (node);
                auto* p = processor.apvts.getParameter (id);
                p->setValueNotifyingHost (p->convertTo0to1 ((float) (result - 1)));
            }
            else if (result == 5)
            {
                auto id = HybridKickEQAudioProcessor::getBandProportionalParamID (node);
                bool cur = processor.apvts.getRawParameterValue (id)->load() > 0.5f;
                processor.apvts.getParameter (id)->setValueNotifyingHost (cur ? 0.0f : 1.0f);
            }
            else if (result == 6)
            {
                auto id = HybridKickEQAudioProcessor::getBandActiveParamID (node);
                processor.apvts.getParameter (id)->setValueNotifyingHost (0.0f);
            }
        });
        return;
    }

    draggingBand = node;
}

void EQCurveComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    for (int b = 0; b < HybridKickEQAudioProcessor::numBands; ++b)
    {
        auto activeId = HybridKickEQAudioProcessor::getBandActiveParamID (b);
        bool active = processor.apvts.getRawParameterValue (activeId)->load() > 0.5f;
        if (active) continue;

        auto freq = xToFreq (e.position.x);
        auto gain = yToGain (e.position.y);

        auto typeId = HybridKickEQAudioProcessor::getBandTypeParamID (b);
        processor.apvts.getParameter (typeId)->setValueNotifyingHost (
            processor.apvts.getParameter (typeId)->convertTo0to1 ((float) (int) SpectralEQBand::Type::Bell));

        auto freqId = HybridKickEQAudioProcessor::getBandFreqParamID (b);
        auto gainId = HybridKickEQAudioProcessor::getBandGainParamID (b);
        processor.apvts.getParameter (freqId)->setValueNotifyingHost (
            processor.apvts.getParameter (freqId)->convertTo0to1 (freq));
        processor.apvts.getParameter (gainId)->setValueNotifyingHost (
            processor.apvts.getParameter (gainId)->convertTo0to1 (gain));

        processor.apvts.getParameter (activeId)->setValueNotifyingHost (1.0f);
        return;
    }
}

void EQCurveComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingBand < 0) return;

    auto freq = xToFreq (e.position.x);
    auto gain = yToGain (e.position.y);

    auto freqId = HybridKickEQAudioProcessor::getBandFreqParamID (draggingBand);
    auto gainId = HybridKickEQAudioProcessor::getBandGainParamID (draggingBand);

    processor.apvts.getParameter (freqId)->setValueNotifyingHost (
        processor.apvts.getParameter (freqId)->convertTo0to1 (freq));

    auto typeId = HybridKickEQAudioProcessor::getBandTypeParamID (draggingBand);
    auto type = (int) processor.apvts.getRawParameterValue (typeId)->load();

    if (type != (int) SpectralEQBand::Type::HighPass)
        processor.apvts.getParameter (gainId)->setValueNotifyingHost (
            processor.apvts.getParameter (gainId)->convertTo0to1 (gain));
}

void EQCurveComponent::mouseUp (const juce::MouseEvent&) { draggingBand = -1; }

void EQCurveComponent::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    auto node = findNodeUnder (e.position);
    if (node < 0) return;

    auto qId = HybridKickEQAudioProcessor::getBandQParamID (node);
    auto* param = processor.apvts.getParameter (qId);
    auto currentQ = processor.apvts.getRawParameterValue (qId)->load();
    auto newQ = juce::jlimit (0.1f, 18.0f, currentQ + wheel.deltaY * 2.0f);
    param->setValueNotifyingHost (param->convertTo0to1 (newQ));
}
