#include "SerialPortManager.h"

#if JUCE_WINDOWS
    #include <windows.h>
    #include <setupapi.h>
    #include <devguid.h>
    #pragma comment(lib, "setupapi.lib")
#elif JUCE_MAC
    #include <IOKit/IOKitLib.h>
    #include <IOKit/serial/IOSerialKeys.h>
    #include <IOKit/IOBSD.h>
#elif JUCE_LINUX
    #include <dirent.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <termios.h>
    #include <unistd.h>
#endif

SerialPortManager::SerialPortManager()
    : portHandle(nullptr)
#if JUCE_WINDOWS
    , overlappedRead(nullptr)
    , overlappedWrite(nullptr)
#endif
{
}

SerialPortManager::~SerialPortManager()
{
    closePort();
}

juce::Array<SerialPortManager::PortInfo> SerialPortManager::getAvailablePorts()
{
    juce::Array<PortInfo> ports;
    
#if JUCE_WINDOWS
    // Windows: Use SetupAPI to enumerate serial ports
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT);
    
    if (deviceInfoSet != INVALID_HANDLE_VALUE)
    {
        SP_DEVINFO_DATA deviceInfoData;
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        
        for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++)
        {
            HKEY hKey = SetupDiOpenDevRegKey(deviceInfoSet, &deviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
            
            if (hKey != INVALID_HANDLE_VALUE)
            {
                char portName[256];
                DWORD size = sizeof(portName);
                
                if (RegQueryValueExA(hKey, "PortName", NULL, NULL, (LPBYTE)portName, &size) == ERROR_SUCCESS)
                {
                    PortInfo info;
                    info.portName = juce::String(portName);
                    
                    // Get friendly name
                    char friendlyName[256];
                    if (SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &deviceInfoData, SPDRP_FRIENDLYNAME,
                                                          NULL, (PBYTE)friendlyName, sizeof(friendlyName), NULL))
                    {
                        info.friendlyName = juce::String(friendlyName);
                    }
                    
                    ports.add(info);
                }
                
                RegCloseKey(hKey);
            }
        }
        
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
    }
    
#elif JUCE_MAC
    // macOS: Use IOKit to enumerate serial ports
    kern_return_t kernResult;
    io_iterator_t serialPortIterator;
    
    CFMutableDictionaryRef classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
    if (classesToMatch != NULL)
    {
        CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));
        
        kernResult = IOServiceGetMatchingServices(kIOMasterPortDefault, classesToMatch, &serialPortIterator);
        
        if (kernResult == KERN_SUCCESS)
        {
            io_object_t modemService;
            
            while ((modemService = IOIteratorNext(serialPortIterator)))
            {
                CFTypeRef devicePathAsCFString = IORegistryEntryCreateCFProperty(modemService,
                    CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0);
                
                if (devicePathAsCFString)
                {
                    PortInfo info;
                    
                    char devicePath[256];
                    if (CFStringGetCString((CFStringRef)devicePathAsCFString, devicePath, sizeof(devicePath), kCFStringEncodingUTF8))
                    {
                        info.portName = juce::String(devicePath);
                        info.friendlyName = juce::String(devicePath);
                    }
                    
                    CFRelease(devicePathAsCFString);
                    ports.add(info);
                }
                
                IOObjectRelease(modemService);
            }
            
            IOObjectRelease(serialPortIterator);
        }
    }
    
#elif JUCE_LINUX
    // Linux: Scan /dev for ttyUSB*, ttyACM*, ttyS* devices
    const char* devDir = "/dev";
    DIR* dir = opendir(devDir);
    
    if (dir != nullptr)
    {
        struct dirent* entry;
        
        while ((entry = readdir(dir)) != nullptr)
        {
            juce::String name(entry->d_name);
            
            if (name.startsWith("ttyUSB") || name.startsWith("ttyACM") || name.startsWith("ttyS"))
            {
                PortInfo info;
                info.portName = juce::String("/dev/") + name;
                info.friendlyName = name;
                ports.add(info);
            }
        }
        
        closedir(dir);
    }
#endif
    
    return ports;
}

bool SerialPortManager::openPort(const juce::String& portName, int baudRate)
{
    closePort();
    
#if JUCE_WINDOWS
    juce::String portPath = "\\\\.\\" + portName;
    
    HANDLE handle = CreateFileA(portPath.toRawUTF8(),
                                 GENERIC_READ | GENERIC_WRITE,
                                 0,
                                 0,
                                 OPEN_EXISTING,
                                 FILE_FLAG_OVERLAPPED,
                                 0);
    
    if (handle == INVALID_HANDLE_VALUE)
        return false;
    
    DCB dcb;
    memset(&dcb, 0, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);
    
    if (!GetCommState(handle, &dcb))
    {
        CloseHandle(handle);
        return false;
    }
    
    dcb.BaudRate = baudRate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    
    if (!SetCommState(handle, &dcb))
    {
        CloseHandle(handle);
        return false;
    }
    
    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    SetCommTimeouts(handle, &timeouts);
    
    portHandle = handle;
    
#elif JUCE_MAC || JUCE_LINUX
    int fd = open(portName.toRawUTF8(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    
    if (fd == -1)
        return false;
    
    struct termios options;
    tcgetattr(fd, &options);
    
    // Set baud rate
    speed_t speed = B115200;
    switch (baudRate)
    {
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
    }
    
    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);
    
    // 8N1
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    
    // Raw input
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;
    
    tcsetattr(fd, TCSANOW, &options);
    
    portHandle = reinterpret_cast<void*>(static_cast<intptr_t>(fd));
#endif
    
    currentPortName = portName;
    return true;
}

void SerialPortManager::closePort()
{
    if (portHandle != nullptr)
    {
#if JUCE_WINDOWS
        CloseHandle(static_cast<HANDLE>(portHandle));
#else
        close(static_cast<int>(reinterpret_cast<intptr_t>(portHandle)));
#endif
        portHandle = nullptr;
    }
    
    currentPortName = juce::String();
}

int SerialPortManager::write(const juce::uint8* data, int numBytes)
{
    if (!isOpen())
        return 0;
    
#if JUCE_WINDOWS
    DWORD bytesWritten = 0;
    OVERLAPPED overlapped = {0};
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    if (WriteFile(static_cast<HANDLE>(portHandle), data, numBytes, &bytesWritten, &overlapped))
    {
        CloseHandle(overlapped.hEvent);
        return bytesWritten;
    }
    
    if (GetLastError() == ERROR_IO_PENDING)
    {
        if (GetOverlappedResult(static_cast<HANDLE>(portHandle), &overlapped, &bytesWritten, TRUE))
        {
            CloseHandle(overlapped.hEvent);
            return bytesWritten;
        }
    }
    
    CloseHandle(overlapped.hEvent);
    return 0;
    
#else
    return ::write(static_cast<int>(reinterpret_cast<intptr_t>(portHandle)), data, numBytes);
#endif
}

int SerialPortManager::read(juce::uint8* buffer, int maxBytes)
{
    if (!isOpen())
        return 0;
    
#if JUCE_WINDOWS
    DWORD bytesRead = 0;
    OVERLAPPED overlapped = {0};
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    if (ReadFile(static_cast<HANDLE>(portHandle), buffer, maxBytes, &bytesRead, &overlapped))
    {
        CloseHandle(overlapped.hEvent);
        return bytesRead;
    }
    
    if (GetLastError() == ERROR_IO_PENDING)
    {
        if (GetOverlappedResult(static_cast<HANDLE>(portHandle), &overlapped, &bytesRead, TRUE))
        {
            CloseHandle(overlapped.hEvent);
            return bytesRead;
        }
    }
    
    CloseHandle(overlapped.hEvent);
    return 0;
    
#else
    return ::read(static_cast<int>(reinterpret_cast<intptr_t>(portHandle)), buffer, maxBytes);
#endif
}

int SerialPortManager::bytesAvailable() const
{
    if (!isOpen())
        return 0;
    
#if JUCE_WINDOWS
    COMSTAT comStat;
    DWORD errors;
    
    if (ClearCommError(static_cast<HANDLE>(portHandle), &errors, &comStat))
        return comStat.cbInQue;
    
    return 0;
    
#else
    int bytes = 0;
    ioctl(static_cast<int>(reinterpret_cast<intptr_t>(portHandle)), FIONREAD, &bytes);
    return bytes;
#endif
}
