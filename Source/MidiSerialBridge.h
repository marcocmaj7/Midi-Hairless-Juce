#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_events/juce_events.h>
#include "SerialPortManager.h"
#include <unordered_set>

/**
 * MidiSerialBridge manages the bidirectional bridge between MIDI and Serial ports
 * This is the JUCE equivalent of the Qt Bridge class
 */
class MidiSerialBridge : public juce::Timer,
                         private juce::MidiInputCallback
{
public:
    MidiSerialBridge();
    ~MidiSerialBridge() override;
    
    // Attach to MIDI and Serial ports
    void attach(const juce::String& serialPortName,
                const juce::String& midiInputName,
                const juce::String& midiOutputName);
    
    // Detach from all ports
    void detach();
    
    // Check if currently bridging
    bool isActive() const { return serialPort.isOpen() || midiInput != nullptr || midiOutput != nullptr; }
    
    // Callback types for status updates
    std::function<void(const juce::String&)> onDisplayMessage;
    std::function<void(const juce::String&)> onDebugMessage;
    std::function<void()> onMidiReceived;
    std::function<void()> onMidiSent;
    std::function<void()> onSerialTraffic;

    // Runtime configuration -------------------------------------------------
    // Set per-string velocity scale (index 0..5). Value expected 1..10.
    void setStringVelocityScale(int stringIndex, int scale);
    // Set root note (0=C .. 11=B) and scale type intervals (e.g. major)
    void setScale(int rootNote, const juce::Array<int>& intervals); // intervals are pitch-class offsets from root
    // Enable / disable diatonic filtering
    void setFilterEnabled(bool enabled) { filterEnabled = enabled; }
    bool getFilterEnabled() const { return filterEnabled; }

    enum class DiatonicMode { Off = 0, Filter = 1, ReplaceUp = 2 };
    void setDiatonicMode(DiatonicMode m) { diatonicMode = m; }
    DiatonicMode getDiatonicMode() const { return diatonicMode; }

    // Per-string tuning setters
    void setStringOctaveShift(int stringIndex, int shift);
    void setStringSemitoneShift(int stringIndex, int shift);
    // Map a string (index 0..5) to a MIDI channel (1..16). If unset uses direct index+1.
    void setStringChannel(int stringIndex, int channel);
    int  getStringChannel(int stringIndex) const;

    // Utility to describe current scale
    juce::String getScaleDescription() const;
    
private:
    // Timer callback to poll serial data
    void timerCallback() override;
    
    // MIDI input callback
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;
    
    // Process serial data
    void processSerialData();
    void onDataByte(juce::uint8 byte);
    void onStatusByte(juce::uint8 byte);
    void sendMidiMessage();

    // Message transform helpers
    bool shouldFilterOutNote(int midiNote) const; // returns true if note should be suppressed
    int applyVelocityScaling(int channel, int velocity) const; // channel 0..15
    bool processOutgoingMessage(const juce::MidiMessage& original, juce::MidiMessage& transformed); // returns false if filtered
    
    // Utility functions
    juce::String describeMidiMessage(const juce::MidiMessage& message);
    juce::String describeMidiMessage(const juce::uint8* data, int length);
    juce::String applyTimeStamp(const juce::String& message);
    
    // MIDI message parsing constants
    static constexpr juce::uint8 STATUS_MASK = 0x80;
    static constexpr juce::uint8 CHANNEL_MASK = 0x0F;
    static constexpr juce::uint8 TAG_MASK = 0xF0;
    
    static constexpr juce::uint8 TAG_NOTE_OFF = 0x80;
    static constexpr juce::uint8 TAG_NOTE_ON = 0x90;
    static constexpr juce::uint8 TAG_KEY_PRESSURE = 0xA0;
    static constexpr juce::uint8 TAG_CONTROLLER = 0xB0;
    static constexpr juce::uint8 TAG_PROGRAM = 0xC0;
    static constexpr juce::uint8 TAG_CHANNEL_PRESSURE = 0xD0;
    static constexpr juce::uint8 TAG_PITCH_BEND = 0xE0;
    static constexpr juce::uint8 TAG_SPECIAL = 0xF0;
    
    static constexpr juce::uint8 MSG_SYSEX_START = 0xF0;
    static constexpr juce::uint8 MSG_SYSEX_END = 0xF7;
    static constexpr juce::uint8 MSG_DEBUG = 0xFF;
    
    static bool isVoiceMessage(juce::uint8 tag) { return tag >= 0x80 && tag <= 0xEF; }
    static bool isSysCommonMessage(juce::uint8 tag) { return tag >= 0xF0 && tag <= 0xF7; }
    static bool isRealtimeMessage(juce::uint8 tag) { return tag >= 0xF8 && tag < 0xFF; }
    
    static int getDataLength(int status);
    
    // Member variables
    SerialPortManager serialPort;
    std::unique_ptr<juce::MidiInput> midiInput;
    std::unique_ptr<juce::MidiOutput> midiOutput;
    
    juce::String midiInputName;
    juce::String midiOutputName;
    
    // MIDI parsing state
    int runningStatus;
    int dataExpected;
    juce::MemoryBlock messageData;
    
    juce::Time attachTime;

    // Runtime settings -------------------------------------------------------
    int stringVelocityScale[6]; // 1..10 values, mapped to velocity multiplier
    int octaveShift[6]; // -4..+4
    int semitoneShift[6]; // -12..12
    int channelMap[6]; // 1..16 (0 means use stringIndex+1)
    int rootNotePc; // 0..11
    bool diatonicMask[12]; // allowed pitch classes
    bool filterEnabled { false };
    DiatonicMode diatonicMode { DiatonicMode::Filter };
    std::unordered_set<int> suppressedNotes; // store (channel<<8)|note for which NoteOn was filtered, so we also drop NoteOff
    std::unordered_map<int,int> replacedNotes; // original key -> replaced note
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiSerialBridge)
};
