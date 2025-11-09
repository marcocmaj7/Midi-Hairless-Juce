#include "MidiSerialBridge.h"

MidiSerialBridge::MidiSerialBridge()
    : runningStatus(0)
    , dataExpected(0)
    , attachTime(juce::Time::getCurrentTime())
{
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
    
    // Send to serial port
    if (serialPort.isOpen())
    {
        serialPort.write(message.getRawData(), message.getRawDataSize());
        
        if (onSerialTraffic)
            onSerialTraffic();
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
            midiOutput->sendMessageNow(msg);
            
            if (onMidiSent)
                onMidiSent();
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
