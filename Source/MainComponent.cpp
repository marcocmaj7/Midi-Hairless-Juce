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
    
    // Per-string velocity sliders (6)
    static const char* stringNames[6] = { "6: Low E", "5: A", "4: D", "3: G", "2: B", "1: High E" };
    for (int i = 0; i < 6; ++i)
    {
        auto* lbl = new juce::Label();
        lbl->setText(juce::String(stringNames[i]) + " Vel (1-10)", juce::dontSendNotification);
        lbl->setJustificationType(juce::Justification::centredRight);
        stringVelocityLabels.add(lbl);
        addAndMakeVisible(lbl);

        auto* s = new juce::Slider(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
        s->setRange(1, 10, 1);
        s->setValue(10, juce::dontSendNotification);
        s->onValueChange = [this, i]() { onVelocitySliderChanged(i); };
        stringVelocitySliders.add(s);
        addAndMakeVisible(s);
    }

    // Scale UI
    scaleLabel.setText("Scala di Riferimento:", juce::dontSendNotification);
    addAndMakeVisible(scaleLabel);

    rootNoteCombo.addItemList({"Do","Do#","Re","Re#","Mi","Fa","Fa#","Sol","Sol#","La","La#","Si"}, 1);
    rootNoteCombo.onChange = [this]() { onScaleChanged(); };
    addAndMakeVisible(rootNoteCombo);
    rootNoteCombo.setSelectedId(3, juce::dontSendNotification); // default Re (id=3)

    scaleTypeCombo.addItemList({"Maggiore","Minore Naturale","Dorian","Phrygian","Lydian","Mixolydian","Locrian"}, 1);
    scaleTypeCombo.onChange = [this]() { onScaleChanged(); };
    addAndMakeVisible(scaleTypeCombo);
    scaleTypeCombo.setSelectedId(1, juce::dontSendNotification);

    filterEnableToggle.setButtonText("Filtro Diatonico Attivo");
    filterEnableToggle.setToggleState(true, juce::dontSendNotification);
    filterEnableToggle.onClick = [this]() { onFilterToggle(); };
    addAndMakeVisible(filterEnableToggle);

    // Initial refresh
    refreshSerialPorts();
    refreshMidiInputs();
    refreshMidiOutputs();

    // Apply initial scale to bridge
    applyScaleToBridge();
    
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
    
    // Velocity sliders grid (2 columns x 3 rows)
    auto velSection = bounds.removeFromTop(140);
    auto colLeft = velSection.removeFromLeft(velSection.getWidth() / 2).reduced(0, 5);
    auto colRight = velSection.reduced(10, 5);
    for (int i = 0; i < 3; ++i)
    {
        auto row = colLeft.removeFromTop(40);
        stringVelocityLabels[i]->setBounds(row.removeFromLeft(110));
        row.removeFromLeft(5);
        stringVelocitySliders[i]->setBounds(row);
        colLeft.removeFromTop(5);
    }
    for (int i = 3; i < 6; ++i)
    {
        auto row = colRight.removeFromTop(40);
        stringVelocityLabels[i]->setBounds(row.removeFromLeft(110));
        row.removeFromLeft(5);
        stringVelocitySliders[i]->setBounds(row);
        colRight.removeFromTop(5);
    }

    bounds.removeFromTop(5);

    // Scale controls row
    auto scaleRow1 = bounds.removeFromTop(30);
    scaleLabel.setBounds(scaleRow1.removeFromLeft(140));
    scaleRow1.removeFromLeft(5);
    rootNoteCombo.setBounds(scaleRow1.removeFromLeft(120));
    scaleRow1.removeFromLeft(10);
    scaleTypeCombo.setBounds(scaleRow1.removeFromLeft(180));
    scaleRow1.removeFromLeft(10);
    filterEnableToggle.setBounds(scaleRow1.removeFromLeft(200));

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

// ------------------------ New feature handlers ------------------------------
void MainComponent::onVelocitySliderChanged(int stringIndex)
{
    if (auto* s = stringVelocitySliders[stringIndex])
        bridge.setStringVelocityScale(stringIndex, (int) s->getValue());
}

static juce::Array<int> intervalsForScaleType(int scaleTypeId)
{
    // Return interval set (in semitones from root) for various modes
    switch (scaleTypeId)
    {
        case 1: return {0,2,4,5,7,9,11}; // Major (Ionian)
        case 2: return {0,2,3,5,7,8,10}; // Natural Minor (Aeolian)
        case 3: return {0,2,3,5,7,9,10}; // Dorian
        case 4: return {0,1,3,5,7,8,10}; // Phrygian
        case 5: return {0,2,4,6,7,9,11}; // Lydian
        case 6: return {0,2,4,5,7,9,10}; // Mixolydian
        case 7: return {0,1,3,5,6,8,10}; // Locrian
        default: return {0,1,2,3,4,5,6,7,8,9,10,11}; // Chromatic (no filtering)
    }
}

void MainComponent::applyScaleToBridge()
{
    int rootId = rootNoteCombo.getSelectedId();
    int scaleId = scaleTypeCombo.getSelectedId();
    if (rootId == 0) rootId = 1; // default to C
    if (scaleId == 0) scaleId = 1;
    int rootPc = rootId - 1; // 0=C .. 11=B
    auto intervals = intervalsForScaleType(scaleId);
    bridge.setScale(rootPc, intervals);
    bridge.setFilterEnabled(filterEnableToggle.getToggleState());
}

void MainComponent::onScaleChanged()
{
    applyScaleToBridge();
}

void MainComponent::onFilterToggle()
{
    bridge.setFilterEnabled(filterEnableToggle.getToggleState());
}
