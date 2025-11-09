#pragma once

#include <juce_core/juce_core.h>

/**
 * SerialPortManager handles serial port enumeration and communication
 * This replaces the qextserialport functionality from the Qt version
 */
class SerialPortManager
{
public:
    struct PortInfo
    {
        juce::String portName;
        juce::String friendlyName;
        juce::String description;
        
        juce::String getDisplayName() const
        {
            return friendlyName.isEmpty() ? portName : friendlyName;
        }
    };
    
    SerialPortManager();
    ~SerialPortManager();
    
    // Get list of available serial ports
    static juce::Array<PortInfo> getAvailablePorts();
    
    // Open a serial port
    bool openPort(const juce::String& portName, int baudRate = 115200);
    
    // Close the current port
    void closePort();
    
    // Check if port is open
    bool isOpen() const { return portHandle != nullptr; }
    
    // Write data to serial port
    int write(const juce::uint8* data, int numBytes);
    
    // Read available data from serial port
    int read(juce::uint8* buffer, int maxBytes);
    
    // Check if data is available
    int bytesAvailable() const;
    
    // Set a callback for when data arrives
    std::function<void()> onDataAvailable;
    
private:
    void* portHandle;
    juce::String currentPortName;
    
#if JUCE_WINDOWS
    void* overlappedRead;
    void* overlappedWrite;
#endif
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SerialPortManager)
};
