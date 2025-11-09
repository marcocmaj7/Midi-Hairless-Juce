#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "MidiSerialBridge.h"
#include "ModernLookAndFeel.h"

//==============================================================================
class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    
    void refreshSerialPorts();
    void refreshMidiInputs();
    void refreshMidiOutputs();
    
    void onBridgeToggled();
    void onDebugToggled();
    void onConnectionChanged();

    // New feature handlers
    void onVelocitySliderChanged(int stringIndex);
    void onScaleChanged();
    void onFilterToggle();
    
    void addMessage(const juce::String& message);
    void addDebugMessage(const juce::String& message);
    
    void updateLED(juce::Component& led, bool on);
    
    // UI Components
    juce::Label serialLabel;
    juce::ComboBox serialCombo;
    
    juce::Label midiInLabel;
    juce::ComboBox midiInCombo;
    
    juce::Label midiOutLabel;
    juce::ComboBox midiOutCombo;
    
    juce::ToggleButton bridgeToggle;
    juce::ToggleButton debugToggle;
    
    juce::Label statusLabel;
    juce::TextEditor messageList;
    
    juce::Label debugLabel;
    juce::TextEditor debugList;
    
    // LED indicators
    class LEDComponent : public juce::Component
    {
    public:
        LEDComponent() : isOn(false) {}
        
        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            
            if (isOn)
            {
                g.setGradientFill(juce::ColourGradient(
                    juce::Colours::lime.brighter(0.5f), bounds.getCentre(),
                    juce::Colours::green, bounds.getBottomRight(), true));
            }
            else
            {
                g.setColour(juce::Colours::darkgrey);
            }
            
            g.fillEllipse(bounds.reduced(2));
            
            g.setColour(juce::Colours::black);
            g.drawEllipse(bounds.reduced(2), 1.0f);
        }
        
        void setOn(bool shouldBeOn)
        {
            if (isOn != shouldBeOn)
            {
                isOn = shouldBeOn;
                repaint();
            }
        }
        
    private:
        bool isOn;
    };
    
    LEDComponent midiInLED;
    LEDComponent midiOutLED;
    LEDComponent serialLED;
    
    juce::Label midiInLEDLabel;
    juce::Label midiOutLEDLabel;
    juce::Label serialLEDLabel;
    
    // Bridge
    MidiSerialBridge bridge;
    
    // Settings
    int scrollbackSize;
    int maxDebugMessages;
    
    // LED blink timers
    int midiInBlinkCounter;
    int midiOutBlinkCounter;
    int serialBlinkCounter;
    
    static constexpr int LED_BLINK_DURATION = 3; // Timer ticks

    // Per-string velocity sliders (6 strings)
    juce::OwnedArray<juce::Slider> stringVelocitySliders;
    juce::OwnedArray<juce::Label> stringVelocityLabels;

    // Scale selection UI
    juce::ComboBox rootNoteCombo; // C..B
    juce::ComboBox scaleTypeCombo; // Major, Minor (extendable)
    juce::ToggleButton filterEnableToggle; // enable diatonic filtering
    juce::Label scaleLabel;

    void initialiseScaleUI();
    void applyScaleToBridge();

    // Modern look and feel instance
    ModernLookAndFeel modernLnF;
    juce::Font headingFont { "Rubik", 18.0f, juce::Font::bold };

    // Background panel component
    class CardPanel : public juce::Component
    {
    public:
        CardPanel(const juce::String& title, juce::Font heading)
            : headingText(title), headingFont(heading),
              sh(juce::DropShadow(juce::Colours::black.withAlpha(0.35f), 12, juce::Point<int>(0, 2)))
        {
            sh.setOwner(this);
        }

        void paint(juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat();
            g.setColour(juce::Colour(0xFF242730));
            g.fillRoundedRectangle(r, 10.0f);
            g.setColour(juce::Colour(0x332e3440));
            g.drawRoundedRectangle(r, 10.0f, 1.0f);

            g.setColour(juce::Colours::whitesmoke);
            g.setFont(headingFont);
            auto header = r.removeFromTop(26);
            g.drawText(headingText, header, juce::Justification::centredLeft, true);
            // subtle divider under the title
            g.setColour(juce::Colour(0x223b3f4a));
            g.fillRect(header.removeFromBottom(1));
        }

    private:
        juce::String headingText;
        juce::Font headingFont;
        juce::DropShadower sh;
    };

    CardPanel connectionPanel { "Connessioni", headingFont };
    CardPanel velocityPanel { "Velocity Corde", headingFont };
    CardPanel scalePanel { "Scala & Filtro", headingFont };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
