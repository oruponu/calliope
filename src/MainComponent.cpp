#include "MainComponent.h"
#include "AppProperties.h"
#include "io/MidiFileIO.h"

void MainComponent::Divider::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(50, 50, 65));
    g.fillRect(getLocalBounds());
    g.setColour(juce::Colour(80, 80, 95));
    auto cy = getHeight() / 2.0f;
    auto cx = getWidth() / 2.0f;
    for (float dx : {-12.0f, -4.0f, 4.0f, 12.0f})
        g.fillEllipse(cx + dx - 1.0f, cy - 1.0f, 2.0f, 2.0f);
}

void MainComponent::Divider::mouseDown(const juce::MouseEvent&)
{
    if (onDragStart)
        onDragStart();
}

void MainComponent::Divider::mouseDrag(const juce::MouseEvent& e)
{
    if (onDrag)
        onDrag(e.getDistanceFromDragStartY());
}

void MainComponent::ZoomStrip::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(30, 30, 42));
    g.fillRect(getLocalBounds());

    auto drawSymbol = [&](const juce::Rectangle<int>& bounds, bool isMinus)
    {
        bool hover = bounds.contains(getMouseXYRelative());
        float alpha = hover ? 0.9f : 0.5f;
        g.setColour(juce::Colours::white.withAlpha(alpha));
        auto cx = bounds.getCentreX();
        auto cy = bounds.getCentreY();
        int halfLen = 4;
        g.drawHorizontalLine(cy, static_cast<float>(cx - halfLen), static_cast<float>(cx + halfLen + 1));
        if (!isMinus)
            g.drawVerticalLine(cx, static_cast<float>(cy - halfLen), static_cast<float>(cy + halfLen + 1));
    };
    drawSymbol(minusBounds, true);
    drawSymbol(plusBounds, false);
}

void MainComponent::ZoomStrip::resized()
{
    auto area = getLocalBounds();
    if (orientation == Horizontal)
    {
        int btnSize = area.getHeight();
        minusBounds = area.removeFromLeft(btnSize);
        plusBounds = area.removeFromRight(btnSize);
    }
    else
    {
        int btnSize = area.getWidth();
        minusBounds = area.removeFromTop(btnSize);
        plusBounds = area.removeFromBottom(btnSize);
    }
    slider.setBounds(area);
}

void MainComponent::ZoomStrip::mouseUp(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();
    if (minusBounds.contains(pos) && onZoomOut)
        onZoomOut();
    else if (plusBounds.contains(pos) && onZoomIn)
        onZoomIn();
}

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
    else if (type == Loop)
    {
        auto colour = active ? juce::Colour(80, 160, 255) : juce::Colours::white;
        g.setColour(colour.withAlpha(active ? 1.0f : alpha * 0.7f));
        auto cx = bounds.getCentreX();
        auto cy = bounds.getCentreY();
        float s = bounds.getHeight() * 0.2f;
        float hw = s * 0.6f;
        float hh = s * 0.75f;
        float gap = hw * 0.8f;
        float a = hh * 0.8f;

        juce::Path rp;
        rp.startNewSubPath(cx + gap, cy - hh);
        rp.lineTo(cx + hw, cy - hh);
        rp.quadraticTo(cx + hw + hh, cy - hh, cx + hw + hh, cy);
        rp.quadraticTo(cx + hw + hh, cy + hh, cx + hw, cy + hh);
        rp.lineTo(cx + gap, cy + hh);
        g.strokePath(rp, juce::PathStrokeType(1.5f));

        juce::Path lp;
        lp.startNewSubPath(cx - gap, cy + hh);
        lp.lineTo(cx - hw, cy + hh);
        lp.quadraticTo(cx - hw - hh, cy + hh, cx - hw - hh, cy);
        lp.quadraticTo(cx - hw - hh, cy - hh, cx - hw, cy - hh);
        lp.lineTo(cx - gap, cy - hh);
        g.strokePath(lp, juce::PathStrokeType(1.5f));

        juce::Path la;
        la.addTriangle(cx + gap - a, cy + hh, cx + gap, cy + hh - a, cx + gap, cy + hh + a);
        g.fillPath(la);

        juce::Path ra;
        ra.addTriangle(cx - gap + a, cy - hh, cx - gap, cy - hh - a, cx - gap, cy - hh + a);
        g.fillPath(ra);
    }
}

void MainComponent::TransportButton::mouseUp(const juce::MouseEvent& e)
{
    if (active && type != Loop)
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
    if (!midiOutput.open(getAppProperties().getUserSettings()->getValue("midiOutputDeviceId")))
        midiOutput.open();

    audioDeviceManager.initialiseWithDefaultDevices(0, 2);

    pluginHost.prepare(audioGraph);

    audioDeviceManager.addAudioCallback(&audioPlayer);
    audioPlayer.setProcessor(&audioGraph);

    sequence.addTrack();

    playbackEngine.setSequence(&sequence);
    playbackEngine.addListener(&midiOutput);
    playbackEngine.addListener(&pluginHost);

    pianoRoll.setSequence(&sequence);
    pianoRoll.setUndoManager(&undoManager);
    pianoRoll.onPlayheadMoved = [this](int tick)
    {
        playbackEngine.setPositionInTicks(tick);
        controllerLane.setPlayheadTick(tick);
        eventList.setPlayheadTick(tick);
        updateTransportDisplay();
    };
    pianoRoll.onNotesChanged = [this]()
    {
        trackList.refresh();
        controllerLane.repaint();
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
    pianoRoll.onNotePreview = [this](const MidiNote& note) { midiOutput.onNoteOn(0, note); };
    pianoRoll.onNotePreviewEnd = [this](const MidiNote& note) { midiOutput.onNoteOff(0, note); };
    viewport.setViewedComponent(&pianoRoll, false);
    viewport.setScrollBarsShown(true, false);
    viewport.onReachedEnd = [this]()
    {
        pianoRoll.extendContent();
        controllerLane.setContentBeats(pianoRoll.getContentBeats());
    };
    viewport.onVisibleAreaChanged = [this]()
    {
        pianoRoll.updateSize();
        controllerLane.updateSize();
        if (!syncingScroll)
        {
            syncingScroll = true;
            controllerLaneViewport.setViewPosition(viewport.getViewPositionX(), 0);
            syncingScroll = false;
        }
    };
    viewport.onZoom = [this](const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
    {
        float factor = wheel.deltaY > 0 ? 1.15f : (1.0f / 1.15f);
        if (e.mods.isShiftDown())
            zoomVertical(factor, e.getPosition().y);
        else
            zoomHorizontal(factor, e.getPosition().x);
    };
    pianoRoll.onRulerWheel = [this](const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
    {
        float factor = wheel.deltaY > 0 ? 1.15f : (1.0f / 1.15f);
        auto relEvent = e.getEventRelativeTo(&viewport);
        zoomHorizontal(factor, relEvent.getPosition().x);
    };
    pianoRoll.onRulerDrag = [this](const juce::MouseEvent& e, int deltaY)
    {
        float factor = std::pow(1.01f, static_cast<float>(deltaY));
        auto relEvent = e.getEventRelativeTo(&viewport);
        zoomHorizontal(factor, relEvent.getPosition().x);
    };
    addAndMakeVisible(viewport);

    trackList.setSequence(&sequence);
    trackListViewport.setViewedComponent(&trackList, false);
    trackListViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(trackListViewport);
    trackList.onTrackSelected = [this](int activeIdx, const std::set<int>& selected)
    {
        pianoRoll.setSelectedTracks(activeIdx, selected);
        controllerLane.setSelectedTracks(activeIdx, selected);
        eventList.setSelectedTracks(selected);
    };
    trackList.onMuteSoloChanged = []() {};

    controllerLane.setSequence(&sequence);
    controllerLane.setUndoManager(&undoManager);
    controllerLane.setSelectedTracks(0, {0});
    controllerLane.onDataChanged = [this]()
    {
        pianoRoll.repaint();
        eventList.refresh();
    };
    controllerLane.onMouseWheel = [this](const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
    {
        if (e.mods.isCtrlDown())
        {
            float factor = w.deltaY > 0 ? 1.15f : (1.0f / 1.15f);
            auto relEvent = e.getEventRelativeTo(&controllerLaneViewport);
            zoomHorizontal(factor, relEvent.getPosition().x);
            return;
        }
        int scrollSpeed = 600;
        int newX = viewport.getViewPositionX() - juce::roundToInt(w.deltaX * scrollSpeed);
        int newY = viewport.getViewPositionY() - juce::roundToInt(w.deltaY * scrollSpeed);
        newX = juce::jlimit(0, juce::jmax(0, pianoRoll.getWidth() - viewport.getViewWidth()), newX);
        newY = juce::jlimit(0, juce::jmax(0, pianoRoll.getHeight() - viewport.getViewHeight()), newY);
        viewport.setViewPosition(newX, newY);
    };
    controllerLaneViewport.setViewedComponent(&controllerLane, false);
    controllerLaneViewport.setScrollBarsShown(false, true);
    controllerLaneViewport.onVisibleAreaChanged = [this]()
    {
        if (!syncingScroll)
        {
            syncingScroll = true;
            viewport.setViewPosition(controllerLaneViewport.getViewPositionX(), viewport.getViewPositionY());
            syncingScroll = false;
        }
    };
    controllerLaneViewport.onReachedEnd = [this]()
    {
        pianoRoll.extendContent();
        controllerLane.setContentBeats(pianoRoll.getContentBeats());
    };
    addAndMakeVisible(controllerLaneViewport);

    horizontalZoomStrip.slider.setRange(PianoRollComponent::minBeatWidth, PianoRollComponent::maxBeatWidth, 1);
    horizontalZoomStrip.slider.setSkewFactorFromMidPoint(PianoRollComponent::defaultBeatWidth);
    horizontalZoomStrip.slider.setValue(pianoRoll.getBeatWidth(), juce::dontSendNotification);
    horizontalZoomStrip.slider.setDoubleClickReturnValue(true, PianoRollComponent::defaultBeatWidth);
    horizontalZoomStrip.slider.onValueChange = [this]()
    { setHorizontalZoom(static_cast<int>(horizontalZoomStrip.slider.getValue()), viewport.getViewWidth() / 2); };
    horizontalZoomStrip.onZoomIn = [this]() { zoomHorizontal(1.15f, viewport.getViewWidth() / 2); };
    horizontalZoomStrip.onZoomOut = [this]() { zoomHorizontal(1.0f / 1.15f, viewport.getViewWidth() / 2); };
    addAndMakeVisible(horizontalZoomStrip);

    verticalZoomStrip.slider.setRange(PianoRollComponent::minNoteHeight, PianoRollComponent::maxNoteHeight, 1);
    verticalZoomStrip.slider.setSkewFactorFromMidPoint(PianoRollComponent::defaultNoteHeight);
    verticalZoomStrip.slider.setValue(pianoRoll.getNoteHeight(), juce::dontSendNotification);
    verticalZoomStrip.slider.setDoubleClickReturnValue(true, PianoRollComponent::defaultNoteHeight);
    verticalZoomStrip.slider.onValueChange = [this]()
    { setVerticalZoom(static_cast<int>(verticalZoomStrip.slider.getValue()), viewport.getViewHeight() / 2); };
    verticalZoomStrip.onZoomIn = [this]() { zoomVertical(1.15f, viewport.getViewHeight() / 2); };
    verticalZoomStrip.onZoomOut = [this]() { zoomVertical(1.0f / 1.15f, viewport.getViewHeight() / 2); };
    addAndMakeVisible(verticalZoomStrip);

    addAndMakeVisible(controllerLaneDivider);
    controllerLaneDivider.onDragStart = [this]() { controllerLaneHeightOnDragStart = controllerLaneHeight; };
    controllerLaneDivider.onDrag = [this](int deltaY)
    {
        int minH = 60;
        int maxH = getHeight() - menuBarHeight - transportBarHeight - toolBarHeight - 150;
        controllerLaneHeight = juce::jlimit(minH, maxH, controllerLaneHeightOnDragStart - deltaY);
        resized();
    };

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
        controllerLane.setPlayheadTick(0);
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
            controllerLane.setPlayheadTick(playbackEngine.getCurrentTick());
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
        controllerLane.setPlayheadTick(playbackEngine.getCurrentTick());
        eventList.setPlayheadTick(playbackEngine.getCurrentTick());
        updateTransportDisplay();
    };

    addAndMakeVisible(loopButton);
    loopButton.onClick = [this]()
    {
        bool newState = !playbackEngine.isLoopEnabled();
        playbackEngine.setLoopEnabled(newState);
        loopButton.setActive(newState);
        pianoRoll.setLoopRegion(newState, playbackEngine.getLoopStartTick(), playbackEngine.getLoopEndTick());
        controllerLane.setLoopRegion(newState, playbackEngine.getLoopStartTick(), playbackEngine.getLoopEndTick());
    };

    pianoRoll.onLoopRegionChanged = [this](int startTick, int endTick)
    {
        playbackEngine.setLoopRange(startTick, endTick);
        bool enabled = playbackEngine.isLoopEnabled();
        controllerLane.setLoopRegion(enabled, startTick, endTick);
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

    addAndMakeVisible(quantizeComboBox);
    quantizeComboBox.addItem("1/1", 1);
    quantizeComboBox.addItem("1/2", 2);
    quantizeComboBox.addItem("1/4", 4);
    quantizeComboBox.addItem("1/8", 8);
    quantizeComboBox.addItem("1/16", 16);
    quantizeComboBox.addItem("1/32", 32);
    quantizeComboBox.setSelectedId(4, juce::dontSendNotification);
    quantizeComboBox.onChange = [this]()
    {
        int denom = quantizeComboBox.getSelectedId();
        pianoRoll.setQuantizeDenominator(denom);
        controllerLane.setQuantizeDenominator(denom);
    };

    updateTransportDisplay();

    commandManager.registerAllCommandsForTarget(this);
    setApplicationCommandManagerToWatch(&commandManager);
    setWantsKeyboardFocus(true);
    setSize(1280, 800);

    int c4Y = PianoRollComponent::gridTopOffset + (127 - 60) * pianoRoll.noteHeight - getHeight() / 2;
    viewport.setViewPosition(0, c4Y);
}

MainComponent::~MainComponent()
{
    menuBar.setModel(nullptr);
    vblankAttachment.reset();
    playbackEngine.stop();
    playbackEngine.removeListener(&pluginHost);
    playbackEngine.removeListener(&midiOutput);
    midiOutput.close();
    audioPlayer.setProcessor(nullptr);
    audioDeviceManager.removeAudioCallback(&audioPlayer);
    audioDeviceManager.closeAudioDevice();
}

void MainComponent::parentHierarchyChanged()
{
    if (auto* topLevel = getTopLevelComponent())
        topLevel->addKeyListener(commandManager.getKeyMappings());
}

juce::StringArray MainComponent::getMenuBarNames()
{
    return {"File", "Edit", "View", "Plugins", "Settings"};
}

juce::PopupMenu MainComponent::getMenuForIndex(int menuIndex, const juce::String&)
{
    juce::PopupMenu menu;
    if (menuIndex == 0)
    {
        juce::PopupMenu::Item newItem;
        newItem.itemID = CommandID::newFile_;
        newItem.text = "New";
        newItem.shortcutKeyDescription = "Ctrl+N";
        newItem.action = [this]() { commandManager.invokeDirectly(CommandID::newFile_, true); };
        menu.addItem(newItem);

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
    else if (menuIndex == 1)
    {
        menu.addCommandItem(&commandManager, CommandID::undoAction);
        menu.addCommandItem(&commandManager, CommandID::redoAction);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandID::cutAction);
        menu.addCommandItem(&commandManager, CommandID::copyAction);
        menu.addCommandItem(&commandManager, CommandID::pasteAction);
    }
    else if (menuIndex == 2)
    {
        menu.addCommandItem(&commandManager, CommandID::zoomInHorizontal);
        menu.addCommandItem(&commandManager, CommandID::zoomOutHorizontal);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandID::zoomInVertical);
        menu.addCommandItem(&commandManager, CommandID::zoomOutVertical);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandID::zoomReset);
    }
    else if (menuIndex == 3)
    {
        juce::PopupMenu::Item loadPluginItem;
        loadPluginItem.itemID = CommandID::loadPlugin_;
        loadPluginItem.text = "Load Plugin...";
        loadPluginItem.action = [this]() { loadPlugin(); };
        menu.addItem(loadPluginItem);
    }
    else if (menuIndex == 4)
    {
        juce::PopupMenu midiOutputMenu;
        auto devices = juce::MidiOutput::getAvailableDevices();
        auto currentId = midiOutput.getCurrentDeviceIdentifier();

        if (devices.isEmpty())
        {
            midiOutputMenu.addItem(juce::PopupMenu::Item("(No devices available)").setEnabled(false));
        }
        else
        {
            for (const auto& device : devices)
            {
                bool isCurrent = (device.identifier == currentId);
                midiOutputMenu.addItem(juce::PopupMenu::Item(device.name)
                                           .setTicked(isCurrent)
                                           .setAction(
                                               [this, id = device.identifier]()
                                               {
                                                   if (midiOutput.open(id))
                                                       getAppProperties().getUserSettings()->setValue(
                                                           "midiOutputDeviceId", id);
                                               }));
            }
        }

        menu.addSubMenu("MIDI Output", midiOutputMenu);
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
    commands.addArray({CommandID::newFile_,          CommandID::openFile,
                       CommandID::saveFile_,         CommandID::quitApp,
                       CommandID::togglePlay,        CommandID::returnToStart,
                       CommandID::prevBar,           CommandID::nextBar,
                       CommandID::switchToEditTool,  CommandID::switchToSelectTool,
                       CommandID::undoAction,        CommandID::redoAction,
                       CommandID::cutAction,         CommandID::copyAction,
                       CommandID::pasteAction,       CommandID::zoomInHorizontal,
                       CommandID::zoomOutHorizontal, CommandID::zoomInVertical,
                       CommandID::zoomOutVertical,   CommandID::zoomReset,
                       CommandID::toggleLoop});
}

void MainComponent::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    switch (commandID)
    {
    case CommandID::newFile_:
        result.setInfo("New", "", "File", 0);
        result.addDefaultKeypress('N', juce::ModifierKeys::ctrlModifier);
        break;
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
    case CommandID::undoAction:
        result.setInfo("Undo", "", "Edit", 0);
        result.addDefaultKeypress('Z', juce::ModifierKeys::ctrlModifier);
        result.setActive(undoManager.canUndo());
        break;
    case CommandID::redoAction:
        result.setInfo("Redo", "", "Edit", 0);
        result.addDefaultKeypress('Y', juce::ModifierKeys::ctrlModifier);
        result.setActive(undoManager.canRedo());
        break;
    case CommandID::cutAction:
        result.setInfo("Cut", "", "Edit", 0);
        result.addDefaultKeypress('X', juce::ModifierKeys::ctrlModifier);
        result.setActive(pianoRoll.hasSelectedNotes());
        break;
    case CommandID::copyAction:
        result.setInfo("Copy", "", "Edit", 0);
        result.addDefaultKeypress('C', juce::ModifierKeys::ctrlModifier);
        result.setActive(pianoRoll.hasSelectedNotes());
        break;
    case CommandID::pasteAction:
        result.setInfo("Paste", "", "Edit", 0);
        result.addDefaultKeypress('V', juce::ModifierKeys::ctrlModifier);
        result.setActive(pianoRoll.hasClipboardNotes());
        break;
    case CommandID::zoomInHorizontal:
        result.setInfo("Zoom In (Horizontal)", "", "View", 0);
        result.addDefaultKeypress('=', juce::ModifierKeys::ctrlModifier);
        break;
    case CommandID::zoomOutHorizontal:
        result.setInfo("Zoom Out (Horizontal)", "", "View", 0);
        result.addDefaultKeypress('-', juce::ModifierKeys::ctrlModifier);
        break;
    case CommandID::zoomInVertical:
        result.setInfo("Zoom In (Vertical)", "", "View", 0);
        result.addDefaultKeypress('=', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier);
        break;
    case CommandID::zoomOutVertical:
        result.setInfo("Zoom Out (Vertical)", "", "View", 0);
        result.addDefaultKeypress('-', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier);
        break;
    case CommandID::zoomReset:
        result.setInfo("Reset Zoom", "", "View", 0);
        result.addDefaultKeypress('0', juce::ModifierKeys::ctrlModifier);
        break;
    case CommandID::toggleLoop:
        result.setInfo("Toggle Loop", "", "Transport", 0);
        result.addDefaultKeypress('/', 0);
        break;
    default:
        break;
    }
}

bool MainComponent::perform(const InvocationInfo& info)
{
    switch (info.commandID)
    {
    case CommandID::newFile_:
        newFile();
        return true;
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
        controllerLane.setPlayheadTick(0);
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
        controllerLane.setPlayheadTick(newTick);
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
        controllerLane.setPlayheadTick(newTick);
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
    case CommandID::undoAction:
        undoManager.undo();
        refreshAllViews();
        return true;
    case CommandID::redoAction:
        undoManager.redo();
        refreshAllViews();
        return true;
    case CommandID::cutAction:
        pianoRoll.cutSelectedNotes();
        return true;
    case CommandID::copyAction:
        pianoRoll.copySelectedNotes();
        return true;
    case CommandID::pasteAction:
        pianoRoll.pasteNotes(static_cast<int>(playbackEngine.getCurrentTick()));
        return true;
    case CommandID::zoomInHorizontal:
        zoomHorizontal(1.15f, viewport.getViewWidth() / 2);
        return true;
    case CommandID::zoomOutHorizontal:
        zoomHorizontal(1.0f / 1.15f, viewport.getViewWidth() / 2);
        return true;
    case CommandID::zoomInVertical:
        zoomVertical(1.15f, viewport.getViewHeight() / 2);
        return true;
    case CommandID::zoomOutVertical:
        zoomVertical(1.0f / 1.15f, viewport.getViewHeight() / 2);
        return true;
    case CommandID::zoomReset:
        setHorizontalZoom(PianoRollComponent::defaultBeatWidth, viewport.getViewWidth() / 2);
        setVerticalZoom(PianoRollComponent::defaultNoteHeight, viewport.getViewHeight() / 2);
        return true;
    case CommandID::toggleLoop:
        loopButton.onClick();
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
            {
                currentFile = file;
                undoManager.clearUndoHistory();
                onSequenceLoaded();
                updateTitleBar();
            }
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
    const int btnW = 172;
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
    auto btnArea = btnSection.withSizeKeepingCentre(btnW, 40);
    returnToStartButton.setBounds(btnArea.removeFromLeft(40));
    btnArea.removeFromLeft(4);
    stopButton.setBounds(btnArea.removeFromLeft(40));
    btnArea.removeFromLeft(4);
    playButton.setBounds(btnArea.removeFromLeft(40));
    btnArea.removeFromLeft(4);
    loopButton.setBounds(btnArea.removeFromLeft(40));
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
        toolBtnArea.removeFromLeft(pad * 2);
        quantizeComboBox.setBounds(toolBtnArea.removeFromLeft(70));
    }

    trackListViewport.setBounds(area.removeFromLeft(trackListWidth));
    trackList.setSize(trackListViewport.getMaximumVisibleWidth(), trackList.getHeight());
    eventList.setBounds(area.removeFromRight(eventListWidth));

    int clampedEditorH = juce::jlimit(0, area.getHeight() - 100, controllerLaneHeight);
    auto editorArea = area.removeFromBottom(clampedEditorH);
    auto divArea = area.removeFromBottom(dividerHeight);

    viewport.setBounds(area);
    pianoRoll.updateSize();
    controllerLaneDivider.setBounds(divArea);
    controllerLaneViewport.setBounds(editorArea);
    controllerLane.setSize(std::max(controllerLane.getWidth(), editorArea.getWidth()),
                           controllerLaneViewport.getMaximumVisibleHeight());
    controllerLane.updateSize();

    int sbThickness = viewport.getScrollBarThickness();
    verticalZoomStrip.setBounds(viewport.getRight() - sbThickness, viewport.getBottom() - zoomStripLength, sbThickness,
                                zoomStripLength);
    horizontalZoomStrip.setBounds(controllerLaneViewport.getRight() - zoomStripLength,
                                  controllerLaneViewport.getBottom() - sbThickness, zoomStripLength, sbThickness);
}

void MainComponent::onVBlank()
{
    double tick = playbackEngine.getCurrentTick();
    pianoRoll.setPlayheadTick(tick);
    controllerLane.setPlayheadTick(tick);
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

void MainComponent::refreshAllViews()
{
    pianoRoll.setSelectedNotes({});
    pianoRoll.repaint();
    controllerLane.repaint();
    trackList.refresh();
    eventList.refresh();
}

void MainComponent::setHorizontalZoom(int newBeatWidth, int anchorXInViewport)
{
    newBeatWidth = juce::jlimit(PianoRollComponent::minBeatWidth, PianoRollComponent::maxBeatWidth, newBeatWidth);
    if (newBeatWidth == pianoRoll.getBeatWidth())
        return;

    int tickAtAnchor = pianoRoll.xToTick(viewport.getViewPositionX() + anchorXInViewport);
    pianoRoll.setBeatWidth(newBeatWidth);
    controllerLane.setBeatWidth(newBeatWidth);
    horizontalZoomStrip.slider.setValue(newBeatWidth, juce::dontSendNotification);

    int newContentX = pianoRoll.tickToX(tickAtAnchor);
    viewport.setViewPosition(juce::jmax(0, newContentX - anchorXInViewport), viewport.getViewPositionY());
}

void MainComponent::setVerticalZoom(int newNoteHeight, int anchorYInViewport)
{
    newNoteHeight = juce::jlimit(PianoRollComponent::minNoteHeight, PianoRollComponent::maxNoteHeight, newNoteHeight);
    if (newNoteHeight == pianoRoll.getNoteHeight())
        return;

    int noteAtAnchor = pianoRoll.yToNote(viewport.getViewPositionY() + anchorYInViewport);
    pianoRoll.setNoteHeight(newNoteHeight);
    verticalZoomStrip.slider.setValue(newNoteHeight, juce::dontSendNotification);

    int newContentY = pianoRoll.noteToY(noteAtAnchor);
    viewport.setViewPosition(viewport.getViewPositionX(), juce::jmax(0, newContentY - anchorYInViewport));
}

void MainComponent::zoomHorizontal(float factor, int anchorXInViewport)
{
    setHorizontalZoom(juce::roundToInt(pianoRoll.getBeatWidth() * factor), anchorXInViewport);
}

void MainComponent::zoomVertical(float factor, int anchorYInViewport)
{
    setVerticalZoom(juce::roundToInt(pianoRoll.getNoteHeight() * factor), anchorYInViewport);
}

void MainComponent::newFile()
{
    stopPlayback();
    sequence.clear();
    sequence.addTrack();
    currentFile = juce::File{};
    undoManager.clearUndoHistory();
    onSequenceLoaded();
    updateTitleBar();
}

void MainComponent::saveFile()
{
    fileChooser = std::make_unique<juce::FileChooser>("Save MIDI File", juce::File{}, "*.mid");
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                             [this](const juce::FileChooser& fc)
                             {
                                 auto file = fc.getResult();
                                 if (file != juce::File{})
                                 {
                                     MidiFileIO::save(sequence, file);
                                     currentFile = file;
                                     updateTitleBar();
                                 }
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
                                 {
                                     currentFile = file;
                                     undoManager.clearUndoHistory();
                                     onSequenceLoaded();
                                     updateTitleBar();
                                 }
                             });
}

void MainComponent::loadPlugin()
{
    fileChooser = std::make_unique<juce::FileChooser>("Load Plugin", juce::File{}, "*.vst3");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [this](const juce::FileChooser& fc)
                             {
                                 auto file = fc.getResult();
                                 if (file == juce::File{})
                                     return;
                                 pluginHost.loadPlugin(file);
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
    playbackEngine.setLoopEnabled(false);
    playbackEngine.setLoopRange(0, 0);
    loopButton.setActive(false);
    pianoRoll.setLoopRegion(false, 0, 0);
    controllerLane.setLoopRegion(false, 0, 0);

    std::set<int> allTracks;
    for (int i = 0; i < sequence.getNumTracks(); ++i)
        allTracks.insert(i);

    pianoRoll.setSequence(&sequence);
    pianoRoll.setSelectedTracks(0, allTracks);
    pianoRoll.setPlayheadTick(0);
    updateTransportDisplay();

    trackList.setSequence(&sequence);

    controllerLane.setSequence(&sequence);
    controllerLane.setContentBeats(pianoRoll.getContentBeats());
    controllerLane.setSelectedTracks(0, allTracks);

    eventList.setSequence(&sequence);
    eventList.setSelectedTracks(allTracks);

    int c4Y = PianoRollComponent::gridTopOffset + (127 - 60) * pianoRoll.noteHeight - getHeight() / 2;
    viewport.setViewPosition(0, c4Y);
}

void MainComponent::updateTitleBar()
{
    if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
    {
        auto appName = juce::JUCEApplication::getInstance()->getApplicationName();
        if (currentFile != juce::File{})
            window->setName(currentFile.getFileName() + " - " + appName);
        else
            window->setName(appName);
    }
}
