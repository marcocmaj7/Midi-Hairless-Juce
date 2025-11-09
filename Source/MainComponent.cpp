#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : scrollbackSize(500)
    , maxDebugMessages(100)
    , midiInBlinkCounter(0)
    , midiOutBlinkCounter(0)
    , serialBlinkCounter(0)
{
    // Setup labels
    serialLabel.setText("Serial Port:", juce::dontSendNotification);
    serialLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(serialLabel);
    
    midiInLabel.setText("MIDI In:", juce::dontSendNotification);
    midiInLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(midiInLabel);
    
    midiOutLabel.setText("MIDI Out:", juce::dontSendNotification);
    midiOutLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(midiOutLabel);
    
    statusLabel.setText("Status Messages:", juce::dontSendNotification);
    addAndMakeVisible(statusLabel);
    
    debugLabel.setText("Debug Messages:", juce::dontSendNotification);
    addAndMakeVisible(debugLabel);
    
    // Setup combo boxes
    addAndMakeVisible(serialCombo);
    addAndMakeVisible(midiInCombo);
    addAndMakeVisible(midiOutCombo);
    
    serialCombo.onChange = [this] { onConnectionChanged(); };
    midiInCombo.onChange = [this] { onConnectionChanged(); };
    midiOutCombo.onChange = [this] { onConnectionChanged(); };
    
    // Setup toggle buttons
    bridgeToggle.setButtonText("Bridge Active");
    bridgeToggle.onClick = [this] { onBridgeToggled(); };
    addAndMakeVisible(bridgeToggle);
    
    debugToggle.setButtonText("Show Debug Messages");
    debugToggle.onClick = [this] { onDebugToggled(); };
    addAndMakeVisible(debugToggle);
    
    // Setup text editors (read-only)
    messageList.setMultiLine(true);
    messageList.setReadOnly(true);
    messageList.setScrollbarsShown(true);
    messageList.setCaretVisible(false);
    messageList.setPopupMenuEnabled(true);
    addAndMakeVisible(messageList);
    
    debugList.setMultiLine(true);
    debugList.setReadOnly(true);
    debugList.setScrollbarsShown(true);
    debugList.setCaretVisible(false);
    debugList.setPopupMenuEnabled(true);
    debugList.setVisible(false);
    addAndMakeVisible(debugList);
    
    // Setup LEDs
    midiInLEDLabel.setText("MIDI In", juce::dontSendNotification);
    midiInLEDLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(midiInLED);
    addAndMakeVisible(midiInLEDLabel);
    
    midiOutLEDLabel.setText("MIDI Out", juce::dontSendNotification);
    midiOutLEDLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(midiOutLED);
    addAndMakeVisible(midiOutLEDLabel);
    
    serialLEDLabel.setText("Serial", juce::dontSendNotification);
    serialLEDLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(serialLED);
    addAndMakeVisible(serialLEDLabel);
    
    // Setup bridge callbacks
    bridge.onDisplayMessage = [this](const juce::String& msg) { addMessage(msg); };
    bridge.onDebugMessage = [this](const juce::String& msg) { addDebugMessage(msg); };
    bridge.onMidiReceived = [this]() { midiInBlinkCounter = LED_BLINK_DURATION; };
    bridge.onMidiSent = [this]() { midiOutBlinkCounter = LED_BLINK_DURATION; };
    bridge.onSerialTraffic = [this]() { serialBlinkCounter = LED_BLINK_DURATION; };
    
    // Initial refresh
    refreshSerialPorts();
    refreshMidiInputs();
    refreshMidiOutputs();
    
    // Start timer for UI updates (50ms = 20Hz)
    startTimer(50);
    
    setSize(700, 600);
}

MainComponent::~MainComponent()
{
    bridge.detach();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Top section - Port selection
    auto topSection = bounds.removeFromTop(120);
    
    // Serial port row
    auto serialRow = topSection.removeFromTop(30);
    serialLabel.setBounds(serialRow.removeFromLeft(100));
    serialRow.removeFromLeft(5);
    serialCombo.setBounds(serialRow);
    
    topSection.removeFromTop(5);
    
    // MIDI In row
    auto midiInRow = topSection.removeFromTop(30);
    midiInLabel.setBounds(midiInRow.removeFromLeft(100));
    midiInRow.removeFromLeft(5);
    midiInCombo.setBounds(midiInRow);
    
    topSection.removeFromTop(5);
    
    // MIDI Out row
    auto midiOutRow = topSection.removeFromTop(30);
    midiOutLabel.setBounds(midiOutRow.removeFromLeft(100));
    midiOutRow.removeFromLeft(5);
    midiOutCombo.setBounds(midiOutRow);
    
    topSection.removeFromTop(5);
    
    // Toggle buttons row
    auto toggleRow = topSection.removeFromTop(30);
    bridgeToggle.setBounds(toggleRow.removeFromLeft(150));
    toggleRow.removeFromLeft(10);
    debugToggle.setBounds(toggleRow.removeFromLeft(200));
    
    bounds.removeFromTop(10);
    
    // LED indicators
    auto ledSection = bounds.removeFromTop(60);
    int ledWidth = ledSection.getWidth() / 3;
    
    auto midiInLEDArea = ledSection.removeFromLeft(ledWidth);
    midiInLED.setBounds(midiInLEDArea.removeFromTop(30).withSizeKeepingCentre(24, 24));
    midiInLEDLabel.setBounds(midiInLEDArea);
    
    auto midiOutLEDArea = ledSection.removeFromLeft(ledWidth);
    midiOutLED.setBounds(midiOutLEDArea.removeFromTop(30).withSizeKeepingCentre(24, 24));
    midiOutLEDLabel.setBounds(midiOutLEDArea);
    
    auto serialLEDArea = ledSection.removeFromLeft(ledWidth);
    serialLED.setBounds(serialLEDArea.removeFromTop(30).withSizeKeepingCentre(24, 24));
    serialLEDLabel.setBounds(serialLEDArea);
    
    bounds.removeFromTop(10);
    
    // Message lists
    statusLabel.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(5);
    
    if (debugToggle.getToggleState())
    {
        // Show both message lists
        auto halfHeight = bounds.getHeight() / 2;
        messageList.setBounds(bounds.removeFromTop(halfHeight - 15));
        bounds.removeFromTop(5);
        debugLabel.setBounds(bounds.removeFromTop(20));
        bounds.removeFromTop(5);
        debugList.setBounds(bounds);
    }
    else
    {
        // Show only status messages
        messageList.setBounds(bounds);
    }
}

void MainComponent::timerCallback()
{
    // Update LED blink counters
    if (midiInBlinkCounter > 0)
    {
        midiInLED.setOn(true);
        midiInBlinkCounter--;
    }
    else
    {
        midiInLED.setOn(false);
    }
    
    if (midiOutBlinkCounter > 0)
    {
        midiOutLED.setOn(true);
        midiOutBlinkCounter--;
    }
    else
    {
        midiOutLED.setOn(false);
    }
    
    if (serialBlinkCounter > 0)
    {
        serialLED.setOn(true);
        serialBlinkCounter--;
    }
    else
    {
        serialLED.setOn(false);
    }
}

void MainComponent::refreshSerialPorts()
{
    auto currentSelection = serialCombo.getText();
    serialCombo.clear();
    
    serialCombo.addItem("(Not Connected)", 1);
    
    auto ports = SerialPortManager::getAvailablePorts();
    int id = 2;
    
    for (auto& port : ports)
    {
        serialCombo.addItem(port.getDisplayName(), id++);
        
        if (port.getDisplayName() == currentSelection)
            serialCombo.setSelectedId(id - 1, juce::dontSendNotification);
    }
    
    if (serialCombo.getSelectedId() == 0 && serialCombo.getNumItems() > 0)
        serialCombo.setSelectedId(1, juce::dontSendNotification);
}

void MainComponent::refreshMidiInputs()
{
    auto currentSelection = midiInCombo.getText();
    midiInCombo.clear();
    
    midiInCombo.addItem("(Not Connected)", 1);
    
    auto devices = juce::MidiInput::getAvailableDevices();
    int id = 2;
    
    for (auto& device : devices)
    {
        midiInCombo.addItem(device.name, id++);
        
        if (device.name == currentSelection)
            midiInCombo.setSelectedId(id - 1, juce::dontSendNotification);
    }
    
    if (midiInCombo.getSelectedId() == 0 && midiInCombo.getNumItems() > 0)
        midiInCombo.setSelectedId(1, juce::dontSendNotification);
}

void MainComponent::refreshMidiOutputs()
{
    auto currentSelection = midiOutCombo.getText();
    midiOutCombo.clear();
    
    midiOutCombo.addItem("(Not Connected)", 1);
    
    auto devices = juce::MidiOutput::getAvailableDevices();
    int id = 2;
    
    for (auto& device : devices)
    {
        midiOutCombo.addItem(device.name, id++);
        
        if (device.name == currentSelection)
            midiOutCombo.setSelectedId(id - 1, juce::dontSendNotification);
    }
    
    if (midiOutCombo.getSelectedId() == 0 && midiOutCombo.getNumItems() > 0)
        midiOutCombo.setSelectedId(1, juce::dontSendNotification);
}

void MainComponent::onBridgeToggled()
{
    if (bridgeToggle.getToggleState())
    {
        // Start bridging
        juce::String serialPort = serialCombo.getSelectedId() > 1 ? serialCombo.getText() : juce::String();
        juce::String midiIn = midiInCombo.getSelectedId() > 1 ? midiInCombo.getText() : juce::String();
        juce::String midiOut = midiOutCombo.getSelectedId() > 1 ? midiOutCombo.getText() : juce::String();
        
        // Get actual port names for serial
        if (serialPort.isNotEmpty())
        {
            auto ports = SerialPortManager::getAvailablePorts();
            for (auto& port : ports)
            {
                if (port.getDisplayName() == serialPort)
                {
                    serialPort = port.portName;
                    break;
                }
            }
        }
        
        bridge.attach(serialPort, midiIn, midiOut);
    }
    else
    {
        // Stop bridging
        bridge.detach();
    }
}

void MainComponent::onDebugToggled()
{
    debugList.setVisible(debugToggle.getToggleState());
    debugLabel.setVisible(debugToggle.getToggleState());
    resized();
}

void MainComponent::onConnectionChanged()
{
    // If bridge is active, restart it with new settings
    if (bridgeToggle.getToggleState())
    {
        bridge.detach();
        
        juce::String serialPort = serialCombo.getSelectedId() > 1 ? serialCombo.getText() : juce::String();
        juce::String midiIn = midiInCombo.getSelectedId() > 1 ? midiInCombo.getText() : juce::String();
        juce::String midiOut = midiOutCombo.getSelectedId() > 1 ? midiOutCombo.getText() : juce::String();
        
        // Get actual port names for serial
        if (serialPort.isNotEmpty())
        {
            auto ports = SerialPortManager::getAvailablePorts();
            for (auto& port : ports)
            {
                if (port.getDisplayName() == serialPort)
                {
                    serialPort = port.portName;
                    break;
                }
            }
        }
        
        bridge.attach(serialPort, midiIn, midiOut);
    }
}

void MainComponent::addMessage(const juce::String& message)
{
    messageList.moveCaretToEnd();
    messageList.insertTextAtCaret(message + "\n");
    
    // Limit scrollback
    auto text = messageList.getText();
    auto lines = juce::StringArray::fromLines(text);
    
    if (lines.size() > scrollbackSize)
    {
        lines.removeRange(0, lines.size() - scrollbackSize);
        messageList.setText(lines.joinIntoString("\n"));
    }
    
    messageList.moveCaretToEnd();
}

void MainComponent::addDebugMessage(const juce::String& message)
{
    if (!debugToggle.getToggleState())
        return;
    
    debugList.moveCaretToEnd();
    debugList.insertTextAtCaret(message + "\n");
    
    // Limit scrollback
    auto text = debugList.getText();
    auto lines = juce::StringArray::fromLines(text);
    
    if (lines.size() > maxDebugMessages)
    {
        lines.removeRange(0, lines.size() - maxDebugMessages);
        debugList.setText(lines.joinIntoString("\n"));
    }
    
    debugList.moveCaretToEnd();
}
