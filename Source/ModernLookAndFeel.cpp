#include "ModernLookAndFeel.h"

ModernLookAndFeel::ModernLookAndFeel()
{
    using namespace juce;
    setColour(ResizableWindow::backgroundColourId, Colour(0xff1e1f25));
    setColour(ScrollBar::thumbColourId, Colour(0xff4b4f5a));

    setColour(TextEditor::backgroundColourId, Colour(0xff2a2c34));
    setColour(TextEditor::outlineColourId, Colour(0x223b3f4a));
    setColour(TextEditor::textColourId, Colours::whitesmoke);

    setColour(Slider::trackColourId, Colour(0xff5d86ff));
    setColour(Slider::thumbColourId, Colour(0xffb3c7ff));

    setColour(ComboBox::backgroundColourId, Colour(0xff2a2c34));
    setColour(ComboBox::textColourId, Colours::whitesmoke);
    setColour(ComboBox::outlineColourId, Colour(0x223b3f4a));

    setColour(Label::textColourId, Colours::gainsboro);
    setColour(ToggleButton::textColourId, Colours::gainsboro);
    setColour(TextButton::buttonColourId, Colour(0xff3a3e47));
    setColour(TextButton::textColourOffId, Colours::whitesmoke);
}

void ModernLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto base = backgroundColour.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);
    auto col = base;
    if (shouldDrawButtonAsDown) col = base.brighter(0.2f);
    else if (shouldDrawButtonAsHighlighted) col = base.brighter(0.1f);

    g.setColour(col);
    g.fillRoundedRectangle(bounds, 6.0f);
}

void ModernLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isEnabled,
                      int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box)
{
    juce::ignoreUnused(isEnabled, buttonX, buttonY, buttonW, buttonH);
    auto r = juce::Rectangle<int>(0, 0, width, height).toFloat();
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, 1.0f);
}

void ModernLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor)
{
    auto r = juce::Rectangle<int>(0, 0, width, height).toFloat();
    g.setColour(textEditor.findColour(juce::TextEditor::outlineColourId));
    g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, 1.0f);
}

void ModernLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                          float minSliderPos, float maxSliderPos, const juce::Slider::SliderStyle, juce::Slider& slider)
{
    juce::ignoreUnused(minSliderPos, maxSliderPos);
    auto r = juce::Rectangle<int>(x, y, width, height).toFloat();
    auto track = r.reduced(2.0f);
    track.setHeight(6.0f);
    track.setCentre(track.getCentreX(), r.getCentreY());

    drawShadow(g, track.expanded(0, 4), 3.0f);
    g.setColour(slider.findColour(juce::Slider::trackColourId).withAlpha(0.45f));
    g.fillRoundedRectangle(track, 3.0f);

    // Thumb
    g.setColour(slider.findColour(juce::Slider::thumbColourId));
    float thumbX = (float) x + sliderPos;
    auto thumb = juce::Rectangle<float>(0, 0, 14, 14).withCentre({ thumbX, r.getCentreY() });
    g.fillEllipse(thumb);
}

juce::Typeface::Ptr ModernLookAndFeel::getTypefaceForFont(const juce::Font& f)
{
    // Use default system typeface; place-holder to allow future custom fonts
    return juce::LookAndFeel_V4::getTypefaceForFont(f);
}

void ModernLookAndFeel::drawShadow(juce::Graphics& g, juce::Rectangle<float> area, float radius)
{
    juce::Colour shadowCol = juce::Colour(0x33000000);
    g.setColour(shadowCol);
    g.fillRoundedRectangle(area.translated(0, 2), radius);
}
