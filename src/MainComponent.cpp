#include "MainComponent.h"
#include "io/MidiFileIO.h"

void MainComponent::TransportButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    bool hover = isMouseOver();
    float alpha = hover ? 1.0f : 0.65f;

    if (type == ReturnToStart)
    {
        g.setColour(juce::Colours::white.withAlpha(alpha * 0.7f));
        auto h = bounds.getHeight() * 0.45f;
        auto w = h * 0.55f;
        auto cx = bounds.getCentreX();
        auto cy = bounds.getCentreY();
        auto barW = bounds.getWidth() * 0.08f;
        g.fillRoundedRectangle(cx - w * 0.45f - barW, cy - h / 2, barW, h, 1.0f);
        juce::Path path;
        path.addTriangle(cx + w * 0.55f, cy - h / 2, cx + w * 0.55f, cy + h / 2, cx - w * 0.4f, cy);
        g.fillPath(path);
    }
    else if (type == Stop)
    {
        g.setColour(juce::Colours::white.withAlpha(alpha * 0.7f));
        auto size = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.42f;
        g.fillRoundedRectangle(bounds.withSizeKeepingCentre(size, size), 2.0f);
    }
    else if (type == Play)
    {
        auto colour = active ? juce::Colour(0xff00c853) : juce::Colours::white;
        g.setColour(colour.withAlpha(active ? 1.0f : alpha));
        auto h = bounds.getHeight() * 0.5f;
        auto w = h * 0.85f;
        auto cx = bounds.getCentreX();
        auto cy = bounds.getCentreY();
        juce::Path path;
        path.addTriangle(cx - w * 0.38f, cy - h / 2, cx - w * 0.38f, cy + h / 2, cx + w * 0.62f, cy);
        g.fillPath(path);
    }
}

void MainComponent::TransportButton::mouseUp(const juce::MouseEvent& e)
{
    if (active)
        return;
    if (getLocalBounds().contains(e.getPosition()) && onClick)
        onClick();
}

void MainComponent::ToolButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    bool hover = isMouseOver();

    if (active)
    {
        g.setColour(juce::Colour(60, 100, 200).withAlpha(0.4f));
        g.fillRoundedRectangle(bounds.reduced(1.0f), 4.0f);
    }
    else if (hover)
    {
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.fillRoundedRectangle(bounds.reduced(1.0f), 4.0f);
    }

    auto iconColour = active ? juce::Colour(130, 170, 255) : juce::Colours::white.withAlpha(hover ? 0.85f : 0.55f);
    g.setColour(iconColour);

    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();

    if (type == EditTool)
    {
        float size = bounds.getHeight() * 0.38f;
        juce::Path path;
        path.addLineSegment({cx - size * 0.7f, cy + size * 0.7f, cx + size * 0.3f, cy - size * 0.7f}, size * 0.28f);
        g.fillPath(path);
        juce::Path tip;
        tip.addTriangle(cx - size * 0.7f, cy + size * 0.7f, cx - size * 0.45f, cy + size * 0.55f, cx - size * 0.55f,
                        cy + size * 0.45f);
        g.fillPath(tip);
    }
    else if (type == SelectTool)
    {
        float size = bounds.getHeight() * 0.42f;
        juce::Path arrow;
        arrow.startNewSubPath(cx - size * 0.3f, cy - size * 0.75f);
        arrow.lineTo(cx - size * 0.3f, cy + size * 0.75f);
        arrow.lineTo(cx + size * 0.05f, cy + size * 0.35f);
        arrow.lineTo(cx + size * 0.5f, cy + size * 0.35f);
        arrow.closeSubPath();
        g.fillPath(arrow);
    }
}

void MainComponent::ToolButton::mouseUp(const juce::MouseEvent& e)
{
    if (active)
        return;
    if (getLocalBounds().contains(e.getPosition()) && onClick)
        onClick();
}

void MainComponent::setActiveTool(PianoRollComponent::EditMode mode)
{
    editToolButton.setActive(mode == PianoRollComponent::EditMode::Edit);
    selectToolButton.setActive(mode == PianoRollComponent::EditMode::Select);
    pianoRoll.setEditMode(mode);
}

MainComponent::MainComponent()
{
    midiOutput.open();

    sequence.addTrack();

    playbackEngine.setSequence(&sequence);
    playbackEngine.addListener(&midiOutput);

    pianoRoll.setSequence(&sequence);
    pianoRoll.onPlayheadMoved = [this](int tick)
    {
        playbackEngine.setPositionInTicks(tick);
        eventList.setPlayheadTick(tick);
        updateTransportDisplay();
    };
    pianoRoll.onNotesChanged = [this]()
    {
        trackList.refresh();
        eventList.refresh();
    };
    pianoRoll.onNoteSelectionChanged = [this](const auto& selected)
    {
        if (updatingFromEventList)
            return;
        std::set<std::pair<int, int>> noteRefs;
        for (const auto& ref : selected)
            noteRefs.insert({ref.trackIndex, ref.noteIndex});
        eventList.setSelectedNotes(noteRefs);
    };
    viewport.setViewedComponent(&pianoRoll, false);
    viewport.setScrollBarsShown(true, true);
    viewport.onReachedEnd = [this]() { pianoRoll.extendContent(); };
    viewport.onVisibleAreaChanged = [this]() { pianoRoll.updateSize(); };
    addAndMakeVisible(viewport);

    trackList.setSequence(&sequence);
    trackListViewport.setViewedComponent(&trackList, false);
    trackListViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(trackListViewport);
    trackList.onTrackSelected = [this](int activeIdx, const std::set<int>& selected)
    {
        pianoRoll.setSelectedTracks(activeIdx, selected);
        eventList.setSelectedTracks(selected);
    };
    trackList.onMuteSoloChanged = []() {};

    eventList.setSequence(&sequence);
    eventList.setSelectedTracks({0});
    eventList.onEventSelected = [this](int tick)
    {
        playbackEngine.setPositionInTicks(tick);
        pianoRoll.setPlayheadTick(tick);
        updateTransportDisplay();
        scrollToPlayhead(tick);
    };
    eventList.onNoteSelectionFromList = [this](const auto& noteRefs)
    {
        updatingFromEventList = true;
        std::set<PianoRollComponent::NoteRef> notes;
        for (const auto& [trackIdx, noteIdx] : noteRefs)
            notes.insert({trackIdx, noteIdx});
        pianoRoll.setSelectedNotes(notes);
        updatingFromEventList = false;
    };
    addAndMakeVisible(eventList);

    menuBar.setModel(this);
    addAndMakeVisible(menuBar);

    addAndMakeVisible(returnToStartButton);
    returnToStartButton.onClick = [this]()
    {
        playbackEngine.setPositionInTicks(0);
        pianoRoll.setPlayheadTick(0);
        eventList.setPlayheadTick(0);
        updateTransportDisplay();
        viewport.setViewPosition(0, viewport.getViewPositionY());
    };

    addAndMakeVisible(playButton);
    playButton.onClick = [this]()
    {
        if (playbackEngine.isPlaying())
        {
            playbackEngine.stop();
            playButton.setActive(false);
            vblankAttachment.reset();
            pianoRoll.setPlayheadTick(playbackEngine.getCurrentTick());
            eventList.setPlayheadTick(playbackEngine.getCurrentTick());
            updateTransportDisplay();
        }
        else
        {
            playbackEngine.play();
            playButton.setActive(true);
            vblankAttachment = std::make_unique<juce::VBlankAttachment>(this, [this]() { onVBlank(); });
        }
    };

    addAndMakeVisible(stopButton);
    stopButton.onClick = [this]()
    {
        playbackEngine.stop();
        playButton.setActive(false);
        vblankAttachment.reset();
        pianoRoll.setPlayheadTick(playbackEngine.getCurrentTick());
        eventList.setPlayheadTick(playbackEngine.getCurrentTick());
        updateTransportDisplay();
    };

    auto headerColour = juce::Colour(0xff8888aa);
    auto headerFont = juce::Font(juce::FontOptions(12.0f));

    for (auto* label : {&positionHeaderLabel, &timeSigHeaderLabel, &keyHeaderLabel, &tempoHeaderLabel})
    {
        addAndMakeVisible(label);
        label->setFont(headerFont);
        label->setColour(juce::Label::textColourId, headerColour);
        label->setJustificationType(juce::Justification::centred);
    }

    addAndMakeVisible(positionLabel);
    positionLabel.setFont(juce::Font(juce::FontOptions(28.0f)));
    positionLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    positionLabel.setJustificationType(juce::Justification::centred);

    for (auto* label : {&timeSigValueLabel, &keyValueLabel, &tempoValueLabel})
    {
        addAndMakeVisible(label);
        label->setFont(juce::Font(juce::FontOptions(24.0f)));
        label->setColour(juce::Label::textColourId, juce::Colours::white);
        label->setJustificationType(juce::Justification::centred);
    }

    addAndMakeVisible(editToolButton);
    editToolButton.onClick = [this]() { setActiveTool(PianoRollComponent::EditMode::Edit); };

    addAndMakeVisible(selectToolButton);
    selectToolButton.setActive(true);
    selectToolButton.onClick = [this]() { setActiveTool(PianoRollComponent::EditMode::Select); };

    updateTransportDisplay();

    commandManager.registerAllCommandsForTarget(this);
    setApplicationCommandManagerToWatch(&commandManager);
    setWantsKeyboardFocus(true);
    setSize(1280, 800);

    int c4Y = PianoRollComponent::gridTopOffset + (127 - 60) * PianoRollComponent::noteHeight - getHeight() / 2;
    viewport.setViewPosition(0, c4Y);
}

MainComponent::~MainComponent()
{
    menuBar.setModel(nullptr);
    vblankAttachment.reset();
    playbackEngine.stop();
    playbackEngine.removeListener(&midiOutput);
    midiOutput.close();
}

void MainComponent::parentHierarchyChanged()
{
    if (auto* topLevel = getTopLevelComponent())
        topLevel->addKeyListener(commandManager.getKeyMappings());
}

juce::StringArray MainComponent::getMenuBarNames()
{
    return {"File"};
}

juce::PopupMenu MainComponent::getMenuForIndex(int menuIndex, const juce::String&)
{
    juce::PopupMenu menu;
    if (menuIndex == 0)
    {
        juce::PopupMenu::Item open;
        open.itemID = CommandID::openFile;
        open.text = "Open...";
        open.shortcutKeyDescription = "Ctrl+O";
        open.action = [this]() { commandManager.invokeDirectly(CommandID::openFile, true); };
        menu.addItem(open);

        juce::PopupMenu::Item save;
        save.itemID = CommandID::saveFile_;
        save.text = "Save...";
        save.shortcutKeyDescription = "Ctrl+S";
        save.action = [this]() { commandManager.invokeDirectly(CommandID::saveFile_, true); };
        menu.addItem(save);

        menu.addSeparator();

        juce::PopupMenu::Item quit;
        quit.itemID = CommandID::quitApp;
        quit.text = "Exit";
        quit.action = [this]() { commandManager.invokeDirectly(CommandID::quitApp, true); };
        menu.addItem(quit);
    }
    return menu;
}

void MainComponent::menuItemSelected(int, int) {}

juce::ApplicationCommandTarget* MainComponent::getNextCommandTarget()
{
    return findFirstTargetParentComponent();
}

void MainComponent::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    commands.addArray({CommandID::openFile, CommandID::saveFile_, CommandID::quitApp, CommandID::togglePlay,
                       CommandID::returnToStart, CommandID::prevBar, CommandID::nextBar, CommandID::switchToEditTool,
                       CommandID::switchToSelectTool});
}

void MainComponent::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    switch (commandID)
    {
    case CommandID::openFile:
        result.setInfo("Open...", "", "File", 0);
        result.addDefaultKeypress('O', juce::ModifierKeys::ctrlModifier);
        break;
    case CommandID::saveFile_:
        result.setInfo("Save...", "", "File", 0);
        result.addDefaultKeypress('S', juce::ModifierKeys::ctrlModifier);
        break;
    case CommandID::quitApp:
        result.setInfo("Exit", "", "File", 0);
        break;
    case CommandID::togglePlay:
        result.setInfo("Play/Stop", "", "Transport", 0);
        result.addDefaultKeypress(juce::KeyPress::spaceKey, 0);
        break;
    case CommandID::returnToStart:
        result.setInfo("Return to Start", "", "Transport", 0);
        result.addDefaultKeypress(',', juce::ModifierKeys::ctrlModifier);
        break;
    case CommandID::prevBar:
        result.setInfo("Previous Bar", "", "Transport", 0);
        result.addDefaultKeypress(',', 0);
        break;
    case CommandID::nextBar:
        result.setInfo("Next Bar", "", "Transport", 0);
        result.addDefaultKeypress('.', 0);
        break;
    case CommandID::switchToSelectTool:
        result.setInfo("Select Tool", "", "Tools", 0);
        result.addDefaultKeypress('1', 0);
        break;
    case CommandID::switchToEditTool:
        result.setInfo("Edit Tool", "", "Tools", 0);
        result.addDefaultKeypress('2', 0);
        break;
    default:
        break;
    }
}

bool MainComponent::perform(const InvocationInfo& info)
{
    switch (info.commandID)
    {
    case CommandID::openFile:
        loadFile();
        return true;
    case CommandID::saveFile_:
        saveFile();
        return true;
    case CommandID::quitApp:
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
        return true;
    case CommandID::togglePlay:
        playButton.onClick();
        return true;
    case CommandID::returnToStart:
        playbackEngine.setPositionInTicks(0);
        pianoRoll.setPlayheadTick(0);
        eventList.setPlayheadTick(0);
        updateTransportDisplay();
        viewport.setViewPosition(0, viewport.getViewPositionY());
        return true;
    case CommandID::prevBar:
    {
        int currentTick = static_cast<int>(playbackEngine.getCurrentTick());
        auto bbt = sequence.tickToBarBeatTick(currentTick);
        int targetBar = juce::jmax(1, bbt.bar - 1);
        int newTick = sequence.barStartToTick(targetBar);
        playbackEngine.setPositionInTicks(newTick);
        pianoRoll.setPlayheadTick(newTick);
        eventList.setPlayheadTick(newTick);
        updateTransportDisplay();
        scrollToPlayhead(newTick);
        return true;
    }
    case CommandID::nextBar:
    {
        int currentTick = static_cast<int>(playbackEngine.getCurrentTick());
        auto bbt = sequence.tickToBarBeatTick(currentTick);
        int newTick = sequence.barStartToTick(bbt.bar + 1);
        playbackEngine.setPositionInTicks(newTick);
        pianoRoll.setPlayheadTick(newTick);
        eventList.setPlayheadTick(newTick);
        updateTransportDisplay();
        scrollToPlayhead(newTick);
        return true;
    }
    case CommandID::switchToEditTool:
        setActiveTool(PianoRollComponent::EditMode::Edit);
        return true;
    case CommandID::switchToSelectTool:
        setActiveTool(PianoRollComponent::EditMode::Select);
        return true;
    default:
        return false;
    }
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colour(0xff1c1c2c));
    g.fillRect(0, menuBarHeight, getWidth(), transportBarHeight);

    int toolBarTop = menuBarHeight + transportBarHeight;
    g.setColour(juce::Colour(38, 38, 48));
    g.fillRect(0, toolBarTop, getWidth(), toolBarHeight);
    g.setColour(juce::Colour(55, 55, 65));
    g.drawHorizontalLine(toolBarTop + toolBarHeight - 1, 0.0f, static_cast<float>(getWidth()));

    if (fileDragOver)
    {
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.fillRect(getLocalBounds());
        g.setColour(juce::Colours::dodgerblue);
        g.drawRect(getLocalBounds(), 2);
    }
}

bool MainComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& file : files)
        if (file.endsWithIgnoreCase(".mid") || file.endsWithIgnoreCase(".midi"))
            return true;
    return false;
}

void MainComponent::fileDragEnter(const juce::StringArray&, int, int)
{
    fileDragOver = true;
    repaint();
}

void MainComponent::fileDragExit(const juce::StringArray&)
{
    fileDragOver = false;
    repaint();
}

void MainComponent::filesDropped(const juce::StringArray& files, int, int)
{
    fileDragOver = false;
    repaint();

    for (auto& path : files)
    {
        if (path.endsWithIgnoreCase(".mid") || path.endsWithIgnoreCase(".midi"))
        {
            juce::File file(path);
            stopPlayback();
            if (MidiFileIO::load(sequence, file))
                onSequenceLoaded();
            break;
        }
    }
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    menuBar.setBounds(area.removeFromTop(menuBarHeight));
    auto transportArea = area.removeFromTop(transportBarHeight);
    auto toolbar = transportArea;

    const int posW = 190;
    const int btnW = 128;
    const int tsW = 55;
    const int keyW = 55;
    const int tempoW = 90;
    const int g1 = 28, g2 = 36, g3 = 36, g4 = 36;
    const int contentWidth = posW + g1 + btnW + g2 + tsW + g3 + keyW + g4 + tempoW;

    auto content = toolbar.withSizeKeepingCentre(contentWidth, transportBarHeight);

    const int topPad = 8;
    const int headerH = 18;

    auto layoutSection = [&](int width, juce::Label& header, juce::Label& value)
    {
        auto section = content.removeFromLeft(width);
        section.removeFromTop(topPad);
        header.setBounds(section.removeFromTop(headerH));
        value.setBounds(section);
    };

    layoutSection(posW, positionHeaderLabel, positionLabel);
    content.removeFromLeft(g1);

    auto btnSection = content.removeFromLeft(btnW);
    auto btnArea = btnSection.withSizeKeepingCentre(128, 40);
    returnToStartButton.setBounds(btnArea.removeFromLeft(40));
    btnArea.removeFromLeft(4);
    stopButton.setBounds(btnArea.removeFromLeft(40));
    btnArea.removeFromLeft(4);
    playButton.setBounds(btnArea.removeFromLeft(40));
    content.removeFromLeft(g2);

    layoutSection(tsW, timeSigHeaderLabel, timeSigValueLabel);
    content.removeFromLeft(g3);

    layoutSection(keyW, keyHeaderLabel, keyValueLabel);
    content.removeFromLeft(g4);

    layoutSection(tempoW, tempoHeaderLabel, tempoValueLabel);

    auto toolBarArea = area.removeFromTop(toolBarHeight);
    {
        const int btnSize = 28;
        const int pad = 4;
        auto toolBtnArea = toolBarArea.withTrimmedLeft(pad).withSizeKeepingCentre(toolBarArea.getWidth(), btnSize);
        auto left = toolBtnArea.removeFromLeft(btnSize + pad);
        left.removeFromLeft(pad);
        selectToolButton.setBounds(left.removeFromLeft(btnSize));
        editToolButton.setBounds(toolBtnArea.removeFromLeft(btnSize));
    }

    trackListViewport.setBounds(area.removeFromLeft(trackListWidth));
    trackList.setSize(trackListViewport.getMaximumVisibleWidth(), trackList.getHeight());
    eventList.setBounds(area.removeFromRight(eventListWidth));
    viewport.setBounds(area);
}

void MainComponent::onVBlank()
{
    double tick = playbackEngine.getCurrentTick();
    pianoRoll.setPlayheadTick(tick);
    eventList.setPlayheadTick(tick);
    updateTransportDisplay();
    scrollToPlayhead(static_cast<int>(tick));
}

void MainComponent::scrollToPlayhead(int tick)
{
    int playheadX = pianoRoll.tickToX(tick);
    int viewX = viewport.getViewPositionX();
    int viewRight = viewX + viewport.getViewWidth();

    if (playheadX < viewX + PianoRollComponent::keyboardWidth || playheadX > viewRight - 20)
    {
        int desiredX = playheadX - PianoRollComponent::keyboardWidth;
        while (desiredX + viewport.getViewWidth() > pianoRoll.getWidth())
            pianoRoll.extendContent();

        viewport.setViewPosition(desiredX, viewport.getViewPositionY());
    }
}

void MainComponent::updateTransportDisplay()
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());

    auto bbt = sequence.tickToBarBeatTick(tick);
    positionLabel.setText(juce::String(bbt.bar).paddedLeft('0', 3) + "." + juce::String(bbt.beat).paddedLeft('0', 2) +
                              "." + juce::String(bbt.tick).paddedLeft('0', 4),
                          juce::dontSendNotification);

    auto ts = sequence.getTimeSignatureAt(tick);
    timeSigValueLabel.setText(juce::String(ts.numerator) + "/" + juce::String(ts.denominator),
                              juce::dontSendNotification);

    if (sequence.getKeySignatureChanges().empty())
        keyValueLabel.setText("--", juce::dontSendNotification);
    else
    {
        auto ks = sequence.getKeySignatureAt(tick);
        keyValueLabel.setText(MidiSequence::keySignatureToString(ks.sharpsOrFlats, ks.isMinor),
                              juce::dontSendNotification);
    }

    double tempo = sequence.getTempoAt(tick);
    tempoValueLabel.setText(juce::String(tempo, 2), juce::dontSendNotification);
}

void MainComponent::saveFile()
{
    fileChooser = std::make_unique<juce::FileChooser>("Save MIDI File", juce::File{}, "*.mid");
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                             [this](const juce::FileChooser& fc)
                             {
                                 auto file = fc.getResult();
                                 if (file != juce::File{})
                                     MidiFileIO::save(sequence, file);
                             });
}

void MainComponent::loadFile()
{
    fileChooser = std::make_unique<juce::FileChooser>("Open MIDI File", juce::File{}, "*.mid");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [this](const juce::FileChooser& fc)
                             {
                                 auto file = fc.getResult();
                                 if (file == juce::File{})
                                     return;
                                 stopPlayback();
                                 if (MidiFileIO::load(sequence, file))
                                     onSequenceLoaded();
                             });
}

void MainComponent::stopPlayback()
{
    playbackEngine.stop();
    midiOutput.reset();
    playButton.setActive(false);
    vblankAttachment.reset();
}

void MainComponent::onSequenceLoaded()
{
    playbackEngine.setPositionInTicks(0);

    pianoRoll.setSequence(&sequence);
    {
        std::set<int> allTracks;
        for (int i = 0; i < sequence.getNumTracks(); ++i)
            allTracks.insert(i);
        pianoRoll.setSelectedTracks(0, allTracks);
    }
    pianoRoll.setPlayheadTick(0);
    updateTransportDisplay();

    trackList.setSequence(&sequence);

    eventList.setSequence(&sequence);
    {
        std::set<int> allTracks;
        for (int i = 0; i < sequence.getNumTracks(); ++i)
            allTracks.insert(i);
        eventList.setSelectedTracks(allTracks);
    }

    int c4Y = PianoRollComponent::gridTopOffset + (127 - 60) * PianoRollComponent::noteHeight - getHeight() / 2;
    viewport.setViewPosition(0, c4Y);
}
