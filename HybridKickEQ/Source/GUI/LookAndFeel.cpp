#include "LookAndFeel.h"

KickLookAndFeel::KickLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, KickColours::background);
    setColour (juce::Slider::thumbColourId, KickColours::accent);
    setColour (juce::Slider::trackColourId, KickColours::accent);
    setColour (juce::Slider::backgroundColourId, KickColours::gridLine);
    setColour (juce::ComboBox::backgroundColourId, KickColours::panel);
    setColour (juce::ComboBox::textColourId, KickColours::text);
    setColour (juce::ComboBox::outlineColourId, KickColours::gridLine);
    setColour (juce::Label::textColourId, KickColours::text);
    setColour (juce::TextButton::buttonColourId, KickColours::panel);
    setColour (juce::TextButton::textColourOffId, KickColours::textDim);
    setColour (juce::TextButton::textColourOnId, KickColours::accent);
    setColour (juce::ToggleButton::textColourId, KickColours::text);
}

void KickLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                         juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centre = bounds.getCentre();
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = juce::jmax (2.0f, radius * 0.11f);
    auto arcRadius = radius - lineW * 0.5f;

    juce::Path backgroundArc;
    backgroundArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                  rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (KickColours::gridLine);
    g.strokePath (backgroundArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
    g.setColour (slider.isEnabled() ? KickColours::accent : KickColours::textDim);
    g.strokePath (valueArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    auto knobRadius = radius * 0.62f;
    g.setColour (KickColours::panel);
    g.fillEllipse (centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);
    g.setColour (KickColours::gridLineMain);
    g.drawEllipse (centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, 1.0f);

    juce::Path pointer;
    auto pointerLength = knobRadius * 0.75f;
    pointer.addRectangle (-1.5f, -knobRadius, 3.0f, pointerLength);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre));
    g.setColour (KickColours::accent);
    g.fillPath (pointer);
}

void KickLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                         bool highlighted, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    auto on = button.getToggleState();

    g.setColour (on ? KickColours::accent.withAlpha (0.18f) : KickColours::panel);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (on ? KickColours::accent : (highlighted ? KickColours::gridLineMain : KickColours::gridLine));
    g.drawRoundedRectangle (bounds, 4.0f, 1.2f);

    g.setColour (on ? KickColours::accent : KickColours::textDim);
    g.setFont (juce::Font (12.0f, juce::Font::bold));
    g.drawText (button.getButtonText(), bounds, juce::Justification::centred);
}

void KickLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                             const juce::Colour&, bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    auto baseColour = KickColours::panel;
    if (down) baseColour = KickColours::accentDim.withAlpha (0.3f);
    else if (highlighted) baseColour = KickColours::gridLine;

    g.setColour (baseColour);
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (KickColours::gridLineMain);
    g.drawRoundedRectangle (bounds, 5.0f, 1.0f);
}

juce::Font KickLookAndFeel::getComboBoxFont (juce::ComboBox&) { return juce::Font (13.0f); }
juce::Font KickLookAndFeel::getLabelFont (juce::Label&) { return juce::Font (13.0f); }
