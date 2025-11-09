#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class ModernLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ModernLookAndFeel();
    ~ModernLookAndFeel() override = default;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isEnabled,
                      int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override;

    void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                          float minSliderPos, float maxSliderPos, const juce::Slider::SliderStyle, juce::Slider& slider) override;

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& f) override;

    // Small helper to create subtle shadow
    static void drawShadow(juce::Graphics& g, juce::Rectangle<float> area, float radius);
};
