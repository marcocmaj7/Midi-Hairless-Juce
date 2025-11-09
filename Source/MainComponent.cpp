#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : scrollbackSize(500)
    , maxDebugMessages(100)
    , midiInBlinkCounter(0)
    , midiOutBlinkCounter(0)
    , serialBlinkCounter(0)
{
    // Look and Feel
    juce::LookAndFeel::setDefaultLookAndFeel(&modernLnF);

    // Setup labels
    serialLabel.setText("Serial Port:", juce::dontSendNotification);
    serialLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(connectionPanel);
    addAndMakeVisible(serialLabel);
    
    midiInLabel.setText("MIDI In:", juce::dontSendNotification);
    midiInLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(midiInLabel);
    
    midiOutLabel.setText("MIDI Out:", juce::dontSendNotification);
    midiOutLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(midiOutLabel);
    
    // Logs rimossi: nessuna label/status
    
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
    
    // Toggle debug rimosso
    
    // Setup text editors (read-only)
    // Log editor rimosso
    
    // Debug editor rimosso
    
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
    // Hide LED labels as requested (icons convey meaning by position)
    midiInLEDLabel.setVisible(false);
    midiOutLEDLabel.setVisible(false);
    serialLEDLabel.setVisible(false);
    
    // Setup bridge callbacks
    // Nessun collegamento a messaggi di log
    bridge.onMidiReceived = [this]() { midiInBlinkCounter = LED_BLINK_DURATION; };
    bridge.onMidiSent = [this]() { midiOutBlinkCounter = LED_BLINK_DURATION; };
    bridge.onSerialTraffic = [this]() { serialBlinkCounter = LED_BLINK_DURATION; };
    
    // Panels for Velocity & Scale
    addAndMakeVisible(velocityPanel);
    addAndMakeVisible(scalePanel);

    // Per-string velocity & tuning sliders (6) Low E (index0) -> High E (index5)
    static const char* stringNames[6] = { "Low E", "A", "D", "G", "B", "High E" };
    for (int i = 0; i < 6; ++i)
    {
        auto* lbl = new juce::Label();
        lbl->setText(stringNames[i], juce::dontSendNotification);
        lbl->setJustificationType(juce::Justification::centredLeft);
        stringVelocityLabels.add(lbl);
        addAndMakeVisible(lbl);

        auto* vel = new juce::Slider(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
        vel->setRange(1, 10, 1);
        vel->setValue(10, juce::dontSendNotification);
        vel->onValueChange = [this, i]() { onVelocitySliderChanged(i); };
        stringVelocitySliders.add(vel);
        addAndMakeVisible(vel);

        auto* oct = new juce::Slider(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
        oct->setRange(-4, 4, 1);
        oct->setValue(0, juce::dontSendNotification);
        oct->onValueChange = [this, i]() { onOctaveSliderChanged(i); };
        stringOctaveSliders.add(oct);
        addAndMakeVisible(oct);

        auto* semi = new juce::Slider(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
        semi->setRange(-12, 12, 1);
        semi->setValue(0, juce::dontSendNotification);
        semi->onValueChange = [this, i]() { onSemitoneSliderChanged(i); };
        stringSemitoneSliders.add(semi);
        addAndMakeVisible(semi);
    }

    // Column headers
    velHeaderLabel.setText("Velocity", juce::dontSendNotification);
    velHeaderLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(velHeaderLabel);
    octHeaderLabel.setText("Octave", juce::dontSendNotification);
    octHeaderLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(octHeaderLabel);
    semiHeaderLabel.setText("Semitone", juce::dontSendNotification);
    semiHeaderLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(semiHeaderLabel);

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

    // Diatonic mode combo
    diatonicModeLabel.setText("Modo: ", juce::dontSendNotification);
    addAndMakeVisible(diatonicModeLabel);
    diatonicModeCombo.addItemList({"Off","Filtro","Sostituisci (Up)"}, 1);
    diatonicModeCombo.setSelectedId(2, juce::dontSendNotification); // default Filter (id=2 => "Filtro")
    diatonicModeCombo.onChange = [this]() { onDiatonicModeChanged(); };
    addAndMakeVisible(diatonicModeCombo);

    // Initial refresh
    refreshSerialPorts();
    refreshMidiInputs();
    refreshMidiOutputs();

    // Apply initial scale to bridge
    applyScaleToBridge();
    
    // Avvio timer UI (50ms = 20Hz)
    startTimer(50);
    
    setSize(800, 620);

    // Attiva automaticamente il bridge all'avvio
    bridgeToggle.setToggleState(true, juce::dontSendNotification);
    onBridgeToggled();
}

MainComponent::~MainComponent()
{
    bridge.detach();
}

void MainComponent::paint(juce::Graphics& g)
{
    auto bg = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    g.fillAll(bg);
    // Optional subtle vertical gradient overlay for depth
    juce::Colour top = bg.brighter(0.05f);
    juce::Colour bottom = bg.darker(0.05f);
    g.setGradientFill(juce::ColourGradient(top, 0.0f, 0.0f, bottom, 0.0f, (float)getHeight(), false));
    g.fillRect(getLocalBounds());
}

void MainComponent::resized()
{
    using juce::Grid;
    auto outer = getLocalBounds().reduced(12);

    // Panels heights (niente sezione logs)
    auto connectionArea = outer.removeFromTop(150);
    auto midArea = outer.removeFromTop(300);
    auto scaleArea = outer.removeFromTop(110);

    // Connection panel bounds and inner layout
    connectionPanel.setBounds(connectionArea);
    auto connInner = connectionArea.reduced(12);
    {
        Grid grid;
        grid.rowGap = Grid::Px(6);
        grid.columnGap = Grid::Px(8);
        // Columns: Label (120px), Combo (260px), LED (30px), Filler (1fr)
        grid.templateColumns = {
            Grid::TrackInfo(Grid::Px(120)),
            Grid::TrackInfo(Grid::Px(260)),
            Grid::TrackInfo(Grid::Px(30)),
            Grid::TrackInfo(Grid::Fr(1))
        };
        // Rows: Serial, MIDI In, MIDI Out, Toggles
        grid.templateRows = {
            Grid::TrackInfo(Grid::Px(30)),
            Grid::TrackInfo(Grid::Px(30)),
            Grid::TrackInfo(Grid::Px(30)),
            Grid::TrackInfo(Grid::Px(30))
        };

        // Row 1: Serial
    grid.items.add(juce::GridItem(serialLabel).withArea(1, 1));
    grid.items.add(juce::GridItem(serialCombo).withArea(1, 2));
    grid.items.add(juce::GridItem(serialLED).withArea(1, 3).withAlignSelf(juce::GridItem::AlignSelf::center));
    grid.items.add(juce::GridItem().withArea(1, 4)); // filler

        // Row 2: MIDI In
    grid.items.add(juce::GridItem(midiInLabel).withArea(2, 1));
    grid.items.add(juce::GridItem(midiInCombo).withArea(2, 2));
    grid.items.add(juce::GridItem(midiInLED).withArea(2, 3).withAlignSelf(juce::GridItem::AlignSelf::center));
    grid.items.add(juce::GridItem().withArea(2, 4));

        // Row 3: MIDI Out
    grid.items.add(juce::GridItem(midiOutLabel).withArea(3, 1));
    grid.items.add(juce::GridItem(midiOutCombo).withArea(3, 2));
    grid.items.add(juce::GridItem(midiOutLED).withArea(3, 3).withAlignSelf(juce::GridItem::AlignSelf::center));
    grid.items.add(juce::GridItem().withArea(3, 4));

    // Row 4: Solo toggle Bridge
    grid.items.add(juce::GridItem(bridgeToggle).withArea(4, 1));
    grid.items.add(juce::GridItem().withArea(4, 2));
    grid.items.add(juce::GridItem().withArea(4, 3));
    grid.items.add(juce::GridItem().withArea(4, 4));

        grid.performLayout(connInner);
        // Size LEDs to 24x24 and center vertically within their grid cell
        auto placeLed = [](juce::Component& c){ c.setSize(24, 24); };
        placeLed(serialLED);
        placeLed(midiInLED);
        placeLed(midiOutLED);
    }

    // Velocity & Tuning panel
    velocityPanel.setBounds(midArea);
    auto velInner = midArea.reduced(12).withTrimmedTop(26);
    {
        Grid grid;
        grid.rowGap = Grid::Px(4); grid.columnGap = Grid::Px(8);
        grid.templateColumns = {
            Grid::TrackInfo(Grid::Px(90)),   // String name
            Grid::TrackInfo(Grid::Px(170)),  // Velocity slider
            Grid::TrackInfo(Grid::Px(140)),  // Octave slider
            Grid::TrackInfo(Grid::Px(170)),  // Semitone slider
            Grid::TrackInfo(Grid::Fr(1))     // Filler
        };
        // 1 header row + 6 string rows
        grid.templateRows.clear();
        grid.templateRows.add(Grid::TrackInfo(Grid::Px(26))); // header
        for (int i = 0; i < 6; ++i)
            grid.templateRows.add(Grid::TrackInfo(Grid::Px(34)));

        // Header row (blank at column 0, then labels)
        grid.items.add(juce::GridItem().withArea(1,1));
        grid.items.add(juce::GridItem(velHeaderLabel).withArea(1,2));
        grid.items.add(juce::GridItem(octHeaderLabel).withArea(1,3));
        grid.items.add(juce::GridItem(semiHeaderLabel).withArea(1,4));
        grid.items.add(juce::GridItem().withArea(1,5));

        // String rows
        for (int i = 0; i < 6; ++i)
        {
            int row = i + 2;
            grid.items.add(juce::GridItem(*stringVelocityLabels[i]).withArea(row,1));
            grid.items.add(juce::GridItem(*stringVelocitySliders[i]).withArea(row,2));
            grid.items.add(juce::GridItem(*stringOctaveSliders[i]).withArea(row,3));
            grid.items.add(juce::GridItem(*stringSemitoneSliders[i]).withArea(row,4));
            grid.items.add(juce::GridItem().withArea(row,5));
        }
        grid.performLayout(velInner);
    }

    // Scale panel
    scalePanel.setBounds(scaleArea);
    auto scaleInner = scaleArea.reduced(12).withTrimmedTop(26);
    {
        Grid grid;
        grid.rowGap = Grid::Px(6); grid.columnGap = Grid::Px(8);
        grid.templateColumns = {
            Grid::TrackInfo(Grid::Px(140)), // label
            Grid::TrackInfo(Grid::Px(120)), // root
            Grid::TrackInfo(Grid::Px(10)),
            Grid::TrackInfo(Grid::Px(180)), // scale type
            Grid::TrackInfo(Grid::Px(10)),
            Grid::TrackInfo(Grid::Px(120)), // mode label
            Grid::TrackInfo(Grid::Px(160)), // mode combo
            Grid::TrackInfo(Grid::Px(10)),
            Grid::TrackInfo(Grid::Px(200)), // filter toggle
            Grid::TrackInfo(Grid::Fr(1))
        };
        grid.templateRows = { Grid::TrackInfo(Grid::Px(30)) };

        grid.items.add(juce::GridItem(scaleLabel));
        grid.items.add(juce::GridItem(rootNoteCombo));
        grid.items.add(juce::GridItem().withWidth(10));
        grid.items.add(juce::GridItem(scaleTypeCombo));
        grid.items.add(juce::GridItem().withWidth(10));
        diatonicModeLabel.setJustificationType(juce::Justification::centredRight);
        grid.items.add(juce::GridItem(diatonicModeLabel));
        grid.items.add(juce::GridItem(diatonicModeCombo));
        grid.items.add(juce::GridItem().withWidth(10));
        grid.items.add(juce::GridItem(filterEnableToggle));
        grid.items.add(juce::GridItem());
        grid.performLayout(scaleInner);
    }

    // Nessuna area logs
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
    
    // Default to COM1 if available
    if (serialCombo.getSelectedId() == 0)
    {
        int num = serialCombo.getNumItems();
        for (int i = 2; i <= num; ++i)
        {
            auto txt = serialCombo.getItemText(i - 1);
            if (txt.containsIgnoreCase("COM1"))
            {
                serialCombo.setSelectedId(i, juce::dontSendNotification);
                break;
            }
        }
        if (serialCombo.getSelectedId() == 0 && serialCombo.getNumItems() > 0)
            serialCombo.setSelectedId(1, juce::dontSendNotification);
    }
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
    
    // Default to "TriplePlay Express" if available
    if (midiInCombo.getSelectedId() == 0)
    {
        int num = midiInCombo.getNumItems();
        for (int i = 2; i <= num; ++i)
        {
            if (midiInCombo.getItemText(i - 1).equalsIgnoreCase("TriplePlay Express"))
            {
                midiInCombo.setSelectedId(i, juce::dontSendNotification);
                break;
            }
        }
        if (midiInCombo.getSelectedId() == 0 && midiInCombo.getNumItems() > 0)
            midiInCombo.setSelectedId(1, juce::dontSendNotification);
    }
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
    
    // Default to "PythonToReaper" if available
    if (midiOutCombo.getSelectedId() == 0)
    {
        int num = midiOutCombo.getNumItems();
        for (int i = 2; i <= num; ++i)
        {
            if (midiOutCombo.getItemText(i - 1).equalsIgnoreCase("PythonToReaper"))
            {
                midiOutCombo.setSelectedId(i, juce::dontSendNotification);
                break;
            }
        }
        if (midiOutCombo.getSelectedId() == 0 && midiOutCombo.getNumItems() > 0)
            midiOutCombo.setSelectedId(1, juce::dontSendNotification);
    }
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

void MainComponent::onOctaveSliderChanged(int stringIndex)
{
    if (auto* s = stringOctaveSliders[stringIndex])
        bridge.setStringOctaveShift(stringIndex, (int) s->getValue());
}

void MainComponent::onSemitoneSliderChanged(int stringIndex)
{
    if (auto* s = stringSemitoneSliders[stringIndex])
        bridge.setStringSemitoneShift(stringIndex, (int) s->getValue());
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
    bool on = filterEnableToggle.getToggleState();
    bridge.setFilterEnabled(on);
    // Keep mode consistent: if turned off, set mode Off; if on and mode Off, switch to Filter
    if (!on)
    {
        diatonicModeCombo.setSelectedId(1, juce::dontSendNotification); // Off
        bridge.setDiatonicMode(MidiSerialBridge::DiatonicMode::Off);
    }
    else if (diatonicModeCombo.getSelectedId() == 1)
    {
        diatonicModeCombo.setSelectedId(2, juce::dontSendNotification); // Filter
        bridge.setDiatonicMode(MidiSerialBridge::DiatonicMode::Filter);
    }
}

void MainComponent::onDiatonicModeChanged()
{
    int id = diatonicModeCombo.getSelectedId();
    switch (id)
    {
        case 1: // Off
            bridge.setDiatonicMode(MidiSerialBridge::DiatonicMode::Off);
            filterEnableToggle.setToggleState(false, juce::dontSendNotification);
            bridge.setFilterEnabled(false);
            break;
        case 2: // Filter
            bridge.setDiatonicMode(MidiSerialBridge::DiatonicMode::Filter);
            filterEnableToggle.setToggleState(true, juce::dontSendNotification);
            bridge.setFilterEnabled(true);
            break;
        case 3: // ReplaceUp
            bridge.setDiatonicMode(MidiSerialBridge::DiatonicMode::ReplaceUp);
            filterEnableToggle.setToggleState(true, juce::dontSendNotification);
            bridge.setFilterEnabled(true);
            break;
        default:
            break;
    }
}
