#include "MidiSerialBridge.h"

MidiSerialBridge::MidiSerialBridge()
    : runningStatus(0)
    , dataExpected(0)
    , attachTime(juce::Time::getCurrentTime())
{
    // Defaults: all velocity scales at 10 (unity)
    for (int i = 0; i < 6; ++i)
    {
        stringVelocityScale[i] = 10;
        octaveShift[i] = 0;
        semitoneShift[i] = 0;
        channelMap[i] = i + 1; // default: strings map to channels 1..6
    }
    rootNotePc = 0; // C
    for (int i = 0; i < 12; ++i)
        diatonicMask[i] = true; // initially allow all notes (chromatic)
}

MidiSerialBridge::~MidiSerialBridge()
{
    detach();
}

void MidiSerialBridge::attach(const juce::String& serialPortName,
                               const juce::String& midiInputName,
                               const juce::String& midiOutputName)
{
    detach();
    
    attachTime = juce::Time::getCurrentTime();
    
    // Open serial port
    if (serialPortName.isNotEmpty())
    {
        if (onDisplayMessage)
            onDisplayMessage("Opening serial port '" + serialPortName + "'...");
        
        if (serialPort.openPort(serialPortName, 115200))
        {
            if (onDisplayMessage)
                onDisplayMessage("Serial port opened successfully");
            
            // Start timer to poll serial data (20ms intervals)
            startTimer(20);
        }
        else
        {
            if (onDisplayMessage)
                onDisplayMessage("Failed to open serial port '" + serialPortName + "'");
        }
    }
    
    // Open MIDI input
    if (midiInputName.isNotEmpty())
    {
        auto devices = juce::MidiInput::getAvailableDevices();
        
        for (auto& device : devices)
        {
            if (device.name == midiInputName)
            {
                if (onDisplayMessage)
                    onDisplayMessage("Opening MIDI Input '" + midiInputName + "'...");
                
                midiInput = juce::MidiInput::openDevice(device.identifier, this);
                
                if (midiInput != nullptr)
                {
                    midiInput->start();
                    this->midiInputName = midiInputName;
                    
                    if (onDisplayMessage)
                        onDisplayMessage("MIDI Input opened successfully");
                }
                else
                {
                    if (onDisplayMessage)
                        onDisplayMessage("Failed to open MIDI Input");
                }
                
                break;
            }
        }
    }
    
    // Open MIDI output
    if (midiOutputName.isNotEmpty())
    {
        auto devices = juce::MidiOutput::getAvailableDevices();
        
        for (auto& device : devices)
        {
            if (device.name == midiOutputName)
            {
                if (onDisplayMessage)
                    onDisplayMessage("Opening MIDI Output '" + midiOutputName + "'...");
                
                midiOutput = juce::MidiOutput::openDevice(device.identifier);
                
                if (midiOutput != nullptr)
                {
                    this->midiOutputName = midiOutputName;
                    
                    if (onDisplayMessage)
                        onDisplayMessage("MIDI Output opened successfully");
                }
                else
                {
                    if (onDisplayMessage)
                        onDisplayMessage("Failed to open MIDI Output");
                }
                
                break;
            }
        }
    }
}

void MidiSerialBridge::detach()
{
    stopTimer();
    
    if (onDisplayMessage)
        onDisplayMessage(applyTimeStamp("Closing MIDI<->Serial bridge..."));
    
    if (midiInput != nullptr)
    {
        midiInput->stop();
        midiInput.reset();
    }
    
    midiOutput.reset();
    serialPort.closePort();
    
    runningStatus = 0;
    dataExpected = 0;
    messageData.reset();
}

void MidiSerialBridge::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    juce::ignoreUnused(source);
    
    if (onDebugMessage)
        onDebugMessage(applyTimeStamp("MIDI In: " + describeMidiMessage(message)));
    
    if (onMidiReceived)
        onMidiReceived();
    
    juce::MidiMessage transformed(message);
    if (! processOutgoingMessage(message, transformed))
        return; // filtered out

    // Send to serial port
    if (serialPort.isOpen())
    {
        serialPort.write(transformed.getRawData(), transformed.getRawDataSize());
        if (onSerialTraffic)
            onSerialTraffic();
    }

    // Send to MIDI output (loopback to DAW)
    if (midiOutput != nullptr)
    {
        midiOutput->sendMessageNow(transformed);
        if (onMidiSent)
            onMidiSent();
    }
}

void MidiSerialBridge::timerCallback()
{
    // Poll serial port for data
    if (serialPort.bytesAvailable() > 0)
    {
        processSerialData();
        
        if (onSerialTraffic)
            onSerialTraffic();
    }
}

void MidiSerialBridge::processSerialData()
{
    juce::uint8 buffer[1024];
    int bytesRead = serialPort.read(buffer, sizeof(buffer));
    
    for (int i = 0; i < bytesRead; ++i)
    {
        juce::uint8 nextByte = buffer[i];
        
        if (nextByte & STATUS_MASK)
            onStatusByte(nextByte);
        else
            onDataByte(nextByte);
        
        if (dataExpected == 0)
            sendMidiMessage();
    }
}

void MidiSerialBridge::onStatusByte(juce::uint8 byte)
{
    // Handle SysEx end
    if (byte == MSG_SYSEX_END && messageData.getSize() > 0)
    {
        const juce::uint8* data = static_cast<const juce::uint8*>(messageData.getData());
        if (data[0] == MSG_SYSEX_START)
        {
            messageData.append(&byte, 1);
            sendMidiMessage();
            return;
        }
    }
    
    // If we were expecting more data, send incomplete message with warning
    if (dataExpected > 0)
    {
        if (onDisplayMessage)
        {
            juce::String msg = juce::String::formatted(
                "Warning: got a status byte when we were expecting %d more data bytes, "
                "sending possibly incomplete MIDI message 0x%02X",
                dataExpected,
                static_cast<const juce::uint8*>(messageData.getData())[0]);
            onDisplayMessage(applyTimeStamp(msg));
        }
        sendMidiMessage();
    }
    
    // Update running status
    if (isVoiceMessage(byte))
        runningStatus = byte;
    if (isSysCommonMessage(byte))
        runningStatus = 0;
    
    dataExpected = getDataLength(byte);
    
    if (dataExpected == -2) // UNKNOWN_MIDI
    {
        if (onDisplayMessage)
        {
            juce::String msg = juce::String::formatted("Warning: got unexpected status byte 0x%02X", byte);
            onDisplayMessage(applyTimeStamp(msg));
        }
        dataExpected = 0;
    }
    
    messageData.reset();
    messageData.append(&byte, 1);
}

void MidiSerialBridge::onDataByte(juce::uint8 byte)
{
    // Handle running status
    if (dataExpected == 0 && runningStatus != 0)
    {
        onStatusByte(runningStatus);
    }
    
    if (dataExpected == 0)
    {
        if (onDisplayMessage)
        {
            juce::String msg = juce::String::formatted("Error: got unexpected data byte 0x%02X", byte);
            onDisplayMessage(applyTimeStamp(msg));
        }
        return;
    }
    
    messageData.append(&byte, 1);
    dataExpected--;
    
    // Handle debug message length
    const juce::uint8* data = static_cast<const juce::uint8*>(messageData.getData());
    if (data[0] == MSG_DEBUG && dataExpected == 0 && messageData.getSize() == 4)
    {
        dataExpected += data[3]; // Add message length
    }
}

void MidiSerialBridge::sendMidiMessage()
{
    if (messageData.getSize() == 0)
        return;
    
    const juce::uint8* data = static_cast<const juce::uint8*>(messageData.getData());
    
    // Handle debug messages
    if (data[0] == MSG_DEBUG && messageData.getSize() > 4)
    {
        juce::String debugMsg = juce::String::fromUTF8(
            reinterpret_cast<const char*>(data + 4),
            data[3]);
        
        if (onDisplayMessage)
            onDisplayMessage(applyTimeStamp("Serial Says: " + debugMsg));
    }
    else
    {
        // Regular MIDI message
        if (onDebugMessage)
            onDebugMessage(applyTimeStamp("Serial In: " + describeMidiMessage(data, static_cast<int>(messageData.getSize()))));
        
        // Send to MIDI output
        if (midiOutput != nullptr)
        {
            juce::MidiMessage msg(data, static_cast<int>(messageData.getSize()));
            juce::MidiMessage transformed(msg);
            if (processOutgoingMessage(msg, transformed))
            {
                midiOutput->sendMessageNow(transformed);
                if (onMidiSent)
                    onMidiSent();
            }
        }
    }
    
    messageData.reset();
    dataExpected = 0;
}

int MidiSerialBridge::getDataLength(int status)
{
    juce::uint8 channel;
    
    switch (status & TAG_MASK)
    {
        case TAG_PROGRAM:
        case TAG_CHANNEL_PRESSURE:
            return 1;
            
        case TAG_NOTE_ON:
        case TAG_NOTE_OFF:
        case TAG_KEY_PRESSURE:
        case TAG_CONTROLLER:
        case TAG_PITCH_BEND:
            return 2;
            
        case TAG_SPECIAL:
            if (status == MSG_DEBUG)
                return 3; // Debug messages: { 0xFF, 0, 0, <len>, Msg }
            
            if (status == MSG_SYSEX_START)
                return 65535; // SysEx messages go until MSG_SYSEX_END
            
            channel = status & CHANNEL_MASK;
            if (channel < 3)
                return 2;
            else if (channel < 6)
                return 1;
            return 0;
            
        default:
            return -2; // UNKNOWN_MIDI
    }
}

juce::String MidiSerialBridge::describeMidiMessage(const juce::MidiMessage& message)
{
    return describeMidiMessage(message.getRawData(), message.getRawDataSize());
}

juce::String MidiSerialBridge::describeMidiMessage(const juce::uint8* data, int length)
{
    if (length == 0)
        return "Empty message";
    
    juce::uint8 msg = data[0];
    juce::uint8 tag = msg & TAG_MASK;
    juce::uint8 channel = msg & CHANNEL_MASK;
    
    juce::String desc;
    
    switch (tag)
    {
        case TAG_PROGRAM:
            if (length >= 2)
                return juce::String::formatted("Ch %d: Program change %d", channel + 1, data[1]);
            break;
            
        case TAG_CHANNEL_PRESSURE:
            if (length >= 2)
                return juce::String::formatted("Ch %d: Pressure change %d", channel + 1, data[1]);
            break;
            
        case TAG_NOTE_ON:
            if (length >= 3)
                return juce::String::formatted("Ch %d: Note %d on  velocity %d", channel + 1, data[1], data[2]);
            break;
            
        case TAG_NOTE_OFF:
            if (length >= 3)
                return juce::String::formatted("Ch %d: Note %d off velocity %d", channel + 1, data[1], data[2]);
            break;
            
        case TAG_KEY_PRESSURE:
            if (length >= 3)
                return juce::String::formatted("Ch %d: Note %d pressure %d", channel + 1, data[1], data[2]);
            break;
            
        case TAG_CONTROLLER:
            if (length >= 3)
                return juce::String::formatted("Ch %d: Controller %d value %d", channel + 1, data[1], data[2]);
            break;
            
        case TAG_PITCH_BEND:
            if (length >= 3)
            {
                int bend = data[1] | (data[2] << 7);
                return juce::String::formatted("Ch %d: Pitch bend %d", channel + 1, bend);
            }
            break;
            
        case TAG_SPECIAL:
            if (msg == MSG_SYSEX_START)
            {
                juce::String res = "SysEx Message: ";
                for (int i = 1; i < length; i++)
                {
                    if (data[i] == MSG_SYSEX_END)
                        break;
                    res += juce::String::formatted("0x%02X ", data[i]);
                }
                return res;
            }
            
            if (channel < 3 && length >= 3)
                return juce::String::formatted("System Message #%d: %d %d", channel, data[1], data[2]);
            else if (channel < 6 && length >= 2)
                return juce::String::formatted("System Message #%d: %d", channel, data[1]);
            else
                return juce::String::formatted("System Message #%d", channel);
            break;
    }
    
    // Fallback: hex dump
    juce::String hex = "Unknown MIDI message:";
    for (int i = 0; i < length; i++)
        hex += juce::String::formatted(" %02X", data[i]);
    
    return hex;
}

juce::String MidiSerialBridge::applyTimeStamp(const juce::String& message)
{
    double seconds = (juce::Time::getCurrentTime() - attachTime).inSeconds();
    return juce::String::formatted("+%.1f - ", seconds) + message;
}

// ---------------------- Runtime configuration -------------------------------
void MidiSerialBridge::setStringVelocityScale(int stringIndex, int scale)
{
    if (stringIndex < 0 || stringIndex >= 6) return;
    scale = juce::jlimit(1, 10, scale);
    stringVelocityScale[stringIndex] = scale;
}

void MidiSerialBridge::setScale(int rootNote, const juce::Array<int>& intervals)
{
    rootNotePc = ((rootNote % 12) + 12) % 12;
    for (int i = 0; i < 12; ++i) diatonicMask[i] = false;
    for (int interval : intervals)
    {
        int pc = ((rootNotePc + interval) % 12 + 12) % 12;
        diatonicMask[pc] = true;
    }
}

void MidiSerialBridge::setStringOctaveShift(int stringIndex, int shift)
{
    if (stringIndex < 0 || stringIndex >= 6) return;
    shift = juce::jlimit(-4, 4, shift);
    octaveShift[stringIndex] = shift;
}

void MidiSerialBridge::setStringSemitoneShift(int stringIndex, int shift)
{
    if (stringIndex < 0 || stringIndex >= 6) return;
    shift = juce::jlimit(-12, 12, shift);
    semitoneShift[stringIndex] = shift;
}

juce::String MidiSerialBridge::getScaleDescription() const
{
    static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    juce::String allowed;
    for (int i = 0; i < 12; ++i)
        if (diatonicMask[i]) allowed << names[i] << " ";
    return juce::String(names[rootNotePc]) + " scale: " + allowed.trim();
}

// ---------------------- Processing helpers ---------------------------------
bool MidiSerialBridge::shouldFilterOutNote(int midiNote) const
{
    if (! filterEnabled) return false;
    int pc = ((midiNote % 12) + 12) % 12;
    return ! diatonicMask[pc];
}

int MidiSerialBridge::applyVelocityScaling(int channel, int velocity) const
{
    // Assume channels 0..5 map to guitar strings lowE..highE or vice versa.
    // We cannot know pickup ordering; user can adjust scales accordingly.
    if (channel >= 0 && channel < 6)
    {
        float factor = stringVelocityScale[channel] / 10.0f; // 1..10 -> 0.1..1.0
        int scaled = (int)std::round(velocity * factor);
        return juce::jlimit(1, 127, scaled); // avoid zero (interpreted as NoteOff)
    }
    return velocity;
}

bool MidiSerialBridge::processOutgoingMessage(const juce::MidiMessage& original, juce::MidiMessage& transformed)
{
    const int msgChannel0 = original.getChannel() - 1;
    // Resolve string index from configured channel map
    int stringIndex = -1;
    if (msgChannel0 >= 0)
    {
        int msgChannel1 = msgChannel0 + 1;
        for (int i = 0; i < 6; ++i)
        {
            if (channelMap[i] == msgChannel1)
            {
                stringIndex = i;
                break;
            }
        }
        // Fallback to direct mapping if not found
        if (stringIndex < 0 && msgChannel0 >= 0 && msgChannel0 < 6)
            stringIndex = msgChannel0;
    }

    if (original.isNoteOn())
    {
        int channel = msgChannel0; // 0-based
        int note = original.getNoteNumber();
        // Apply per-string tuning (octave and semitone)
        if (stringIndex >= 0)
        {
            note += (octaveShift[stringIndex] * 12) + semitoneShift[stringIndex];
            note = juce::jlimit(0, 127, note);
        }

        if (shouldFilterOutNote(note))
        {
            if (diatonicMode == DiatonicMode::ReplaceUp)
            {
                // Replace with next higher diatonic pitch class within MIDI range
                int nn = note;
                for (int step = 1; step <= 12; ++step)
                {
                    int candidate = note + step;
                    if (candidate > 127) break;
                    int pc = ((candidate % 12) + 12) % 12;
                    if (diatonicMask[pc]) { nn = candidate; break; }
                }
                if (nn != note)
                {
                    replacedNotes[(channel << 8) | note] = nn;
                    note = nn;
                }
                else
                {
                    // fallback: drop if no replacement found
                    suppressedNotes.insert((channel << 8) | note);
                    return false;
                }
            }
            else
            {
                // Mark suppressed so matching NoteOff also filtered
                suppressedNotes.insert((channel << 8) | note);
                return false;
            }
        }
        int vel = applyVelocityScaling(stringIndex >= 0 ? stringIndex : channel, (int) original.getVelocity());
        transformed = juce::MidiMessage::noteOn(channel + 1, note, (juce::uint8) vel);
        transformed.setTimeStamp(original.getTimeStamp());
        return true;
    }
    else if (original.isNoteOff())
    {
        int channel = msgChannel0;
        int note = original.getNoteNumber();
        // Account for per-string tuning shifts for matching
        if (stringIndex >= 0)
        {
            int shifted = note + (octaveShift[stringIndex] * 12) + semitoneShift[stringIndex];
            shifted = juce::jlimit(0, 127, shifted);
            // If we had replaced this note-on, map back
            auto keyOrig = (channel << 8) | note;
            auto it = replacedNotes.find(keyOrig);
            if (it != replacedNotes.end())
            {
                note = it->second; // send note-off for the replaced note
                replacedNotes.erase(it);
            }
            else
            {
                note = shifted;
            }
        }
        int key = (channel << 8) | note;
        if (suppressedNotes.find(key) != suppressedNotes.end())
        {
            // Drop matching note-off
            suppressedNotes.erase(key);
            return false;
        }
        transformed = juce::MidiMessage::noteOff(channel + 1, note);
        transformed.setTimeStamp(original.getTimeStamp());
        return true;
    }
    else
    {
        // Non-note messages pass through unchanged
        transformed = original;
        return true;
    }
}

void MidiSerialBridge::setStringChannel(int stringIndex, int channel)
{
    if (stringIndex < 0 || stringIndex >= 6) return;
    if (channel < 1 || channel > 16) channel = stringIndex + 1;
    channelMap[stringIndex] = channel;
}

int MidiSerialBridge::getStringChannel(int stringIndex) const
{
    if (stringIndex < 0 || stringIndex >= 6) return 0;
    return channelMap[stringIndex];
}
