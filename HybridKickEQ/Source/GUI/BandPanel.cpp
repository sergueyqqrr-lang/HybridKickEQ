#include "BandPanel.h"
#include "LookAndFeel.h"
#include "../DSP/SpectralEQBand.h"

BandStrip::BandStrip (HybridKickEQAudioProcessor& proc, int bandIndex)
    : processor (proc), index (bandIndex)
{
    using APVTS = juce::AudioProcessorValueTreeState;

    addAndMakeVisible (activeButton);
    activeAttach = std::make_unique<APVTS::ButtonAttachment> (
        processor.apvts, HybridKickEQAudioProcessor::getBandActiveParamID (index), activeButton);

    typeBox.addItemList ({ "High Pass", "Bell", "Low Shelf", "High Shelf" }, 1);
    addAndMakeVisible (typeBox);
    typeAttach = std::make_unique<APVTS::ComboBoxAttachment> (
        processor.apvts, HybridKickEQAudioProcessor::getBandTypeParamID (index), typeBox);

    for (auto* s : { &freqSlider, &gainSlider, &qSlider })
    {
        s->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 16);
        addAndMakeVisible (s);
    }

    auto updateGainEnabled = [this]
    {
        auto typeId = HybridKickEQAudioProcessor::getBandTypeParamID (index);
        auto type = (int) processor.apvts.getRawParameterValue (typeId)->load();
        gainSlider.setEnabled (type != (int) SpectralEQBand::Type::HighPass);
    };
    updateGainEnabled();
    typeBox.onChange = updateGainEnabled;

    freqAttach = std::make_unique<APVTS::SliderAttachment> (
        processor.apvts, HybridKickEQAudioProcessor::getBandFreqParamID (index), freqSlider);
    gainAttach = std::make_unique<APVTS::SliderAttachment> (
        processor.apvts, HybridKickEQAudioProcessor::getBandGainParamID (index), gainSlider);
    qAttach = std::make_unique<APVTS::SliderAttachment> (
        processor.apvts, HybridKickEQAudioProcessor::getBandQParamID (index), qSlider);

    addAndMakeVisible (proportionalButton);
    propAttach = std::make_unique<APVTS::ButtonAttachment> (
        processor.apvts, HybridKickEQAudioProcessor::getBandProportionalParamID (index), proportionalButton);
}

void BandStrip::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    auto colour = juce::Colour::fromHSV ((float) index / (float) HybridKickEQAudioProcessor::numBands, 0.6f, 1.0f, 1.0f);

    g.setColour (KickColours::panel);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (colour.withAlpha (0.5f));
    g.drawRoundedRectangle (bounds, 6.0f, 1.5f);

    g.setColour (colour);
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    auto title = HybridKickEQAudioProcessor::getBandNames()[index];
    g.drawText (title.toUpperCase(), bounds.removeFromTop (16.0f), juce::Justification::centred);
}

void BandStrip::resized()
{
    auto area = getLocalBounds().reduced (6);
    area.removeFromTop (16);

    auto topRow = area.removeFromTop (24);
    activeButton.setBounds (topRow.removeFromLeft (44));
    typeBox.setBounds (topRow.reduced (2, 0));

    auto knobsRow = area.removeFromTop (70);
    auto w = knobsRow.getWidth() / 3;
    freqSlider.setBounds (knobsRow.removeFromLeft (w));
    gainSlider.setBounds (knobsRow.removeFromLeft (w));
    qSlider.setBounds (knobsRow);

    proportionalButton.setBounds (area.removeFromTop (22));
}

BandPanel::BandPanel (HybridKickEQAudioProcessor& proc)
{
    for (int i = 0; i < HybridKickEQAudioProcessor::numBands; ++i)
    {
        auto* strip = new BandStrip (proc, i);
        strips.add (strip);
        content.addAndMakeVisible (strip);
    }

    viewport.setViewedComponent (&content, false);
    viewport.setScrollBarsShown (false, true);
    addAndMakeVisible (viewport);
}

void BandPanel::resized()
{
    viewport.setBounds (getLocalBounds());

    const int stripWidth = 160;
    content.setSize (stripWidth * strips.size(), getHeight());

    for (int i = 0; i < strips.size(); ++i)
        strips[i]->setBounds (i * stripWidth, 0, stripWidth - 4, getHeight());
}
