#include "MainComponent.h"
#include "AppProperties.h"
#include "model/UndoActions.h"
#include "ui/Theme.h"
#include "ui/commands/AppCommands.h"

namespace
{
constexpr const char* kStructuralTxn = "Track structure";
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

    auto savedAudioState = getAppProperties().getUserSettings()->getXmlValue("audioDeviceState");
    audioDeviceManager.initialise(0, 2, savedAudioState.get(), true);
    audioDeviceManager.addChangeListener(this);

    pluginHost.prepare(audioGraph);

    audioDeviceManager.addAudioCallback(&audioPlayer);
    audioPlayer.setProcessor(&audioGraph);

    document.getSequence().addTrack();
    document.getSequence().addListener(this);

    playbackEngine.setSequence(&document.getSequence());
    playbackEngine.addListener(&midiOutput);
    playbackEngine.addListener(&pluginHost);

    pianoRoll.setSequence(&document.getSequence());
    pianoRoll.setUndoManager(&document.getUndoManager());

    pianoRoll.onPlayheadMoved = [this](int tick)
    {
        playbackEngine.setPositionInTicks(tick);
        controllerLane.setPlayheadTick(tick);
        eventList.setPlayheadTick(tick);
        transportBar.updateDisplay();
    };
    pianoRoll.onNotesChanged = [this]() { playbackEngine.rebuildSnapshot(); };
    pianoRoll.onTempoChanged = [this]() { playbackEngine.rebuildSnapshot(); };
    pianoRoll.onNoteSelectionChanged = [this](const auto& selected)
    {
        if (updatingFromEventList)
            return;
        std::set<std::pair<int, int>> noteRefs;
        for (const auto& ref : selected)
            noteRefs.insert({ref.trackIndex, ref.noteIndex});
        eventList.setSelectedNotes(noteRefs);
    };
    pianoRoll.onNotePreview = [this](const MidiNote& note)
    {
        auto ctx = makeTrackContext(pianoRoll.getActiveTrackIndex());
        midiOutput.onNoteOn(ctx, note);
        pluginHost.onNoteOn(ctx, note);
    };
    pianoRoll.onNotePreviewEnd = [this](const MidiNote& note)
    {
        auto ctx = makeTrackContext(pianoRoll.getActiveTrackIndex());
        midiOutput.onNoteOff(ctx, note);
        pluginHost.onNoteOff(ctx, note);
    };
    pianoRoll.onScrollToNote = [this](int startTick, int noteNumber) { scrollNoteIntoView(startTick, noteNumber); };
    pianoRoll.onScrollVertical = [this](int deltaY) { scrollViewVertically(deltaY); };
    pianoRoll.onScrollHorizontal = [this](int deltaX) { scrollViewHorizontally(deltaX); };
    viewport.setViewedComponent(&pianoRoll, false);
    viewport.setScrollBarsShown(true, false);
    viewport.setVerticalScrollBarBottomInset(zoomStripLength);
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

    trackList.setSequence(&document.getSequence());
    trackListViewport.setViewedComponent(&trackList, false);
    trackListViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(trackListViewport);
    trackList.onTrackSelected = [this](int activeIdx, const std::set<int>& selected)
    {
        pianoRoll.setSelectedTracks(activeIdx, selected);
        controllerLane.setSelectedTracks(activeIdx, selected);
        eventList.setSelectedTracks(selected);
    };
    trackList.onMuteSoloChanged = [this]()
    {
        document.getSequence().notifyTracksChanged();
        playbackEngine.rebuildSnapshot();
    };
    trackList.pluginNameForTrack = [this](int trackIndex) { return pluginHost.getPluginName(trackIndex); };
    trackList.onEditorButtonClicked = [this](int trackIndex) { pluginHost.showEditor(trackIndex); };
    trackList.onChannelLabelClicked = [this](int trackIndex)
    {
        int currentCh = document.getSequence().getTrack(trackIndex).getChannel();
        juce::PopupMenu menu;
        menu.addSectionHeader("Channel");
        for (int ch = 1; ch <= 16; ++ch)
        {
            menu.addItem(juce::String(ch), true, ch == currentCh,
                         [this, trackIndex, ch]()
                         {
                             int currentChannel = document.getSequence().getTrack(trackIndex).getChannel();
                             if (currentChannel == ch)
                                 return;
                             document.getUndoManager().beginNewTransaction();
                             document.getUndoManager().perform(new ChannelChangeAction(
                                 &document.getSequence(), trackIndex, currentChannel, ch,
                                 [this](int idx) { playbackEngine.releaseActiveNotesForTrack(idx); }));
                             playbackEngine.rebuildSnapshot();
                         });
        }
        menu.showMenuAsync(juce::PopupMenu::Options{});
    };
    trackList.onAddTrackRequested = [this]()
    {
        document.getUndoManager().beginNewTransaction(kStructuralTxn);
        document.getUndoManager().perform(new TrackAddAction(&document.getSequence(),
                                                             [this](int idx)
                                                             {
                                                                 playbackEngine.releaseActiveNotesForTrack(idx);
                                                                 pluginHost.detachPlugin(idx);
                                                             }));
        trackList.refresh();
        playbackEngine.rebuildSnapshot();
    };
    trackList.onRemoveTrackRequested = [this](int trackIndex)
    {
        if (document.getSequence().getNumTracks() <= 1)
            return;

        bool wasRunning = playbackEngine.suspendForStructuralChange();
        document.getUndoManager().beginNewTransaction(kStructuralTxn);
        document.getUndoManager().perform(new TrackRemoveAction(
            &document.getSequence(), trackIndex, [this](int idx) { pluginHost.detachPlugin(idx); },
            [this](int from, int delta) { pluginHost.renumberTrackIndices(from, delta); }));
        playbackEngine.resumeAfterStructuralChange(wasRunning);

        int newActive = juce::jlimit(0, document.getSequence().getNumTracks() - 1, trackIndex);
        trackList.refresh();
        trackList.setActiveTrackIndex(newActive);
    };
    trackList.onTrackRenamed = [this](int trackIndex, juce::String newName)
    {
        if (trackIndex < 0 || trackIndex >= document.getSequence().getNumTracks())
            return;
        std::string oldName = document.getSequence().getTrack(trackIndex).getName();
        std::string requested = newName.toStdString();
        if (oldName == requested)
            return;
        document.getUndoManager().beginNewTransaction();
        document.getUndoManager().perform(
            new TrackRenameAction(&document.getSequence(), trackIndex, oldName, requested));
    };
    trackList.onPluginLabelClicked = [this](int trackIndex)
    {
        auto types = pluginController.getPluginTypes();

        juce::PopupMenu menu;
        const auto& currentTrack = document.getSequence().getTrack(trackIndex);
        auto currentDest = currentTrack.getOutputDestination();
        int currentRouteTarget = currentTrack.getRouteTargetTrackIndex();

        menu.addSectionHeader("Output");
        for (int i = 0; i < document.getSequence().getNumTracks(); ++i)
        {
            juce::String pluginName = pluginHost.getPluginName(i);
            if (pluginName.isEmpty())
                continue;
            bool isOwn = (i == trackIndex);
            bool ticked =
                currentDest == MidiTrack::OutputDestination::Plugin &&
                (isOwn ? (currentRouteTarget < 0 || currentRouteTarget == trackIndex) : currentRouteTarget == i);
            menu.addItem(pluginName, true, ticked,
                         [this, trackIndex, i, isOwn]()
                         {
                             playbackEngine.releaseActiveNotesForTrack(trackIndex);
                             auto& track = document.getSequence().getTrack(trackIndex);
                             track.setRouteTargetTrackIndex(isOwn ? -1 : i);
                             track.setOutputDestination(MidiTrack::OutputDestination::Plugin);
                             document.getSequence().notifyTracksChanged();
                             playbackEngine.rebuildSnapshot();
                         });
        }

        menu.addItem("MIDI Device", true, currentDest == MidiTrack::OutputDestination::MidiDevice,
                     [this, trackIndex]()
                     {
                         playbackEngine.releaseActiveNotesForTrack(trackIndex);
                         document.getSequence()
                             .getTrack(trackIndex)
                             .setOutputDestination(MidiTrack::OutputDestination::MidiDevice);
                         document.getSequence().notifyTracksChanged();
                         playbackEngine.rebuildSnapshot();
                     });

        menu.addItem(
            "None", true, currentDest == MidiTrack::OutputDestination::None,
            [this, trackIndex]()
            {
                playbackEngine.releaseActiveNotesForTrack(trackIndex);
                document.getSequence().getTrack(trackIndex).setOutputDestination(MidiTrack::OutputDestination::None);
                document.getSequence().notifyTracksChanged();
                playbackEngine.rebuildSnapshot();
            });
        menu.addSeparator();

        menu.addItem("Load Plugin...", true, false,
                     [this, trackIndex]() { pluginController.attachPluginToTrackViaFileChooser(trackIndex); });

        juce::PopupMenu chooseSubmenu;
        juce::KnownPluginList::addToMenu(chooseSubmenu, types, juce::KnownPluginList::sortByManufacturer);
        menu.addSubMenu("Choose Plugin", chooseSubmenu, !types.isEmpty());

        bool hasPlugin = !pluginHost.getPluginName(trackIndex).isEmpty();
        menu.addItem("Detach Plugin", hasPlugin, false,
                     [this, trackIndex]()
                     {
                         if (playbackEngine.isPlaying())
                             stopPlayback();
                         playbackEngine.releaseActiveNotesForTrack(trackIndex);
                         pluginHost.detachPlugin(trackIndex);
                         document.getSequence()
                             .getTrack(trackIndex)
                             .setOutputDestination(MidiTrack::OutputDestination::MidiDevice);
                         document.getSequence().notifyTracksChanged();
                         playbackEngine.rebuildSnapshot();
                     });

        menu.showMenuAsync(juce::PopupMenu::Options{},
                           [this, trackIndex, types](int result)
                           {
                               int index = juce::KnownPluginList::getIndexChosenByMenu(types, result);
                               if (index < 0)
                                   return;
                               pluginController.attachPluginToTrack(trackIndex, types.getReference(index));
                           });
    };

    controllerLane.setSequence(&document.getSequence());
    controllerLane.setUndoManager(&document.getUndoManager());
    controllerLane.setSelectedTracks(0, {0});
    controllerLane.onDataChanged = [this]()
    {
        pianoRoll.repaint();
        eventList.refresh();
    };
    controllerLane.onMouseWheel = [this](const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
    {
        if (e.mods.isCommandDown())
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
    controllerLaneViewport.setHorizontalScrollBarRightInset(zoomStripLength);
    controllerLane.setRightPadding(viewport.getScrollBarThickness());
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

    addAndMakeVisible(trackListDivider);
    trackListDivider.onDragStart = [this]() { trackListWidthOnDragStart = trackListWidth; };
    trackListDivider.onDrag = [this](int deltaX)
    {
        int minW = 80;
        int maxW = juce::jmax(minW, getWidth() - eventListWidth - 200);
        trackListWidth = juce::jlimit(minW, maxW, trackListWidthOnDragStart + deltaX);
        resized();
    };

    addAndMakeVisible(eventListDivider);
    eventListDivider.onDragStart = [this]() { eventListWidthOnDragStart = eventListWidth; };
    eventListDivider.onDrag = [this](int deltaX)
    {
        int minW = 80;
        int maxW = juce::jmax(minW, getWidth() - trackListWidth - 200);
        eventListWidth = juce::jlimit(minW, maxW, eventListWidthOnDragStart - deltaX);
        resized();
    };

    eventList.setSequence(&document.getSequence());
    eventList.setSelectedTracks({0});
    eventList.onEventSelected = [this](int tick)
    {
        playbackEngine.setPositionInTicks(tick);
        pianoRoll.setPlayheadTick(tick);
        transportBar.updateDisplay();
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

    menuBar.setModel(&mainMenuModel);
    addAndMakeVisible(menuBar);

    addAndMakeVisible(transportBar);
    transportBar.onPlayheadMoved = [this](double tick)
    {
        pianoRoll.setPlayheadTick(tick);
        controllerLane.setPlayheadTick(tick);
        eventList.setPlayheadTick(tick);
    };
    transportBar.onScrollToPlayhead = [this](int tick) { scrollToPlayhead(tick); };
    transportBar.onReturnToStart = [this]() { viewport.setViewPosition(0, viewport.getViewPositionY()); };
    transportBar.onPlaybackStateChanged = [this](bool playing)
    {
        if (playing)
            vblankAttachment = std::make_unique<juce::VBlankAttachment>(this, [this]() { onVBlank(); });
        else
            vblankAttachment.reset();
    };
    transportBar.onLoopRegionChanged = [this](bool enabled, int startTick, int endTick)
    {
        pianoRoll.setLoopRegion(enabled, startTick, endTick);
        controllerLane.setLoopRegion(enabled, startTick, endTick);
    };

    pianoRoll.onLoopRegionChanged = [this](int startTick, int endTick)
    {
        playbackEngine.setLoopRange(startTick, endTick);
        bool enabled = playbackEngine.isLoopEnabled();
        controllerLane.setLoopRegion(enabled, startTick, endTick);
    };

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

    transportBar.updateDisplay();

    trackList.setWantsKeyboardFocus(true);
    pianoRoll.setWantsKeyboardFocus(true);
    eventList.setWantsKeyboardFocus(true);
    controllerLane.setWantsKeyboardFocus(true);
    selectToolButton.setWantsKeyboardFocus(true);
    editToolButton.setWantsKeyboardFocus(true);
    addAndMakeVisible(focusBorder);
    juce::Desktop::getInstance().addFocusChangeListener(this);

    commandManager.registerAllCommandsForTarget(this);
    setWantsKeyboardFocus(true);
    setSize(1280, 800);

    int c4Y = PianoRollComponent::gridTopOffset + (127 - 60) * pianoRoll.noteHeight - getHeight() / 2;
    viewport.setViewPosition(0, c4Y);
}

void MainComponent::tracksChanged()
{
    repaint(trackListHeaderBounds);
}

MainComponent::~MainComponent()
{
    document.getSequence().removeListener(this);
    juce::Desktop::getInstance().removeFocusChangeListener(this);
    audioDeviceManager.removeChangeListener(this);
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

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &audioDeviceManager)
    {
        if (auto xml = audioDeviceManager.createStateXml())
            getAppProperties().getUserSettings()->setValue("audioDeviceState", xml.get());
    }
}

void MainComponent::globalFocusChanged(juce::Component* focusedComponent)
{
    if (focusedComponent == nullptr)
        return;

    auto belongsTo = [focusedComponent](const juce::Component& panel)
    { return &panel == focusedComponent || panel.isParentOf(focusedComponent); };

    FocusPanel detected = focusedPanel;
    if (belongsTo(trackList))
        detected = FocusPanel::TrackList;
    else if (belongsTo(pianoRoll) || belongsTo(controllerLane) || belongsTo(quantizeComboBox) ||
             belongsTo(selectToolButton) || belongsTo(editToolButton))
        detected = FocusPanel::PianoRoll;
    else if (belongsTo(eventList))
        detected = FocusPanel::EventList;
    else
        return;

    if (detected == focusedPanel)
        return;

    focusedPanel = detected;
    updateFocusBorder();
}

void MainComponent::updateFocusBorder()
{
    juce::Rectangle<int> target;
    switch (focusedPanel)
    {
    case FocusPanel::TrackList:
        target = trackListPanelBounds;
        break;
    case FocusPanel::PianoRoll:
        target = pianoRollPanelBounds;
        break;
    case FocusPanel::EventList:
        target = eventListPanelBounds;
        break;
    }

    focusBorder.setBounds(target);
    focusBorder.setVisible(!target.isEmpty());
}

void MainComponent::parentHierarchyChanged()
{
    if (auto* topLevel = getTopLevelComponent())
        topLevel->addKeyListener(commandManager.getKeyMappings());
}

void MainComponent::mouseDown(const juce::MouseEvent& e)
{
    if (toolBarBounds.contains(e.getPosition()))
        pianoRoll.grabKeyboardFocus();
}

juce::ApplicationCommandTarget* MainComponent::getNextCommandTarget()
{
    return findFirstTargetParentComponent();
}

void MainComponent::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    AppCommands::getAllCommands(commands);
}

void MainComponent::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    AppCommands::getCommandInfo(commandID, result);

    switch (commandID)
    {
    case AppCommands::undoAction:
        result.setActive(document.getUndoManager().canUndo());
        break;
    case AppCommands::redoAction:
        result.setActive(document.getUndoManager().canRedo());
        break;
    case AppCommands::cutAction:
    case AppCommands::copyAction:
        result.setActive(pianoRoll.hasSelection());
        break;
    case AppCommands::pasteAction:
        result.setActive(pianoRoll.hasClipboardContent());
        break;
    case AppCommands::selectAllAction:
        result.setActive(focusedPanel == FocusPanel::PianoRoll && pianoRoll.hasNotesInActiveTrack());
        break;
    case AppCommands::moveNotesUp:
    case AppCommands::moveNotesDown:
        result.setActive(focusedPanel == FocusPanel::PianoRoll && pianoRoll.hasSelectedNotes());
        break;
    case AppCommands::moveSelectionPrev:
    case AppCommands::moveSelectionNext:
        result.setActive(focusedPanel == FocusPanel::PianoRoll && pianoRoll.hasNotesInActiveTrack());
        break;
    case AppCommands::scrollViewUp:
    case AppCommands::scrollViewDown:
    case AppCommands::scrollViewLeft:
    case AppCommands::scrollViewRight:
        result.setActive(focusedPanel == FocusPanel::PianoRoll);
        break;
    default:
        break;
    }
}

bool MainComponent::perform(const InvocationInfo& info)
{
    switch (info.commandID)
    {
    case AppCommands::newFile_:
        newFile();
        return true;
    case AppCommands::openFile:
        loadFile();
        return true;
    case AppCommands::saveFile_:
        saveFile();
        return true;
    case AppCommands::quitApp:
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
        return true;
    case AppCommands::togglePlay:
        transportBar.togglePlay();
        return true;
    case AppCommands::returnToStart:
        transportBar.returnToStart();
        return true;
    case AppCommands::prevBar:
    {
        int currentTick = static_cast<int>(playbackEngine.getCurrentTick());
        auto bbt = document.getSequence().tickToBarBeatTick(currentTick);
        int targetBar = juce::jmax(1, bbt.bar - 1);
        transportBar.jumpToTick(document.getSequence().barStartToTick(targetBar));
        return true;
    }
    case AppCommands::nextBar:
    {
        int currentTick = static_cast<int>(playbackEngine.getCurrentTick());
        auto bbt = document.getSequence().tickToBarBeatTick(currentTick);
        transportBar.jumpToTick(document.getSequence().barStartToTick(bbt.bar + 1));
        return true;
    }
    case AppCommands::switchToEditTool:
        setActiveTool(PianoRollComponent::EditMode::Edit);
        return true;
    case AppCommands::switchToSelectTool:
        setActiveTool(PianoRollComponent::EditMode::Select);
        return true;
    case AppCommands::undoAction:
    {
        const bool structural = (document.getUndoManager().getUndoDescription() == juce::String(kStructuralTxn));
        bool wasRunning = false;
        if (structural)
            wasRunning = playbackEngine.suspendForStructuralChange();
        document.getUndoManager().undo();
        if (structural)
            playbackEngine.resumeAfterStructuralChange(wasRunning);
        else
            playbackEngine.rebuildSnapshot();
        trackList.refresh();
        pianoRoll.setSelectedNotes({});
        return true;
    }
    case AppCommands::redoAction:
    {
        const bool structural = (document.getUndoManager().getRedoDescription() == juce::String(kStructuralTxn));
        bool wasRunning = false;
        if (structural)
            wasRunning = playbackEngine.suspendForStructuralChange();
        document.getUndoManager().redo();
        if (structural)
            playbackEngine.resumeAfterStructuralChange(wasRunning);
        else
            playbackEngine.rebuildSnapshot();
        trackList.refresh();
        pianoRoll.setSelectedNotes({});
        return true;
    }
    case AppCommands::cutAction:
        pianoRoll.cutSelection();
        return true;
    case AppCommands::copyAction:
        pianoRoll.copySelection();
        return true;
    case AppCommands::pasteAction:
        pianoRoll.paste(static_cast<int>(playbackEngine.getCurrentTick()));
        return true;
    case AppCommands::selectAllAction:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRoll.selectAllNotes();
        return true;
    case AppCommands::moveNotesUp:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRoll.nudgeSelectedNotesPitch(1);
        return true;
    case AppCommands::moveNotesDown:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRoll.nudgeSelectedNotesPitch(-1);
        return true;
    case AppCommands::moveSelectionPrev:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRoll.moveSelectionToAdjacentNote(-1);
        return true;
    case AppCommands::moveSelectionNext:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRoll.moveSelectionToAdjacentNote(1);
        return true;
    case AppCommands::scrollViewUp:
        if (focusedPanel == FocusPanel::PianoRoll)
            scrollViewVertically(-PianoRollComponent::defaultNoteHeight);
        return true;
    case AppCommands::scrollViewDown:
        if (focusedPanel == FocusPanel::PianoRoll)
            scrollViewVertically(PianoRollComponent::defaultNoteHeight);
        return true;
    case AppCommands::scrollViewLeft:
        if (focusedPanel == FocusPanel::PianoRoll)
            scrollViewHorizontally(-PianoRollComponent::defaultBeatWidth);
        return true;
    case AppCommands::scrollViewRight:
        if (focusedPanel == FocusPanel::PianoRoll)
            scrollViewHorizontally(PianoRollComponent::defaultBeatWidth);
        return true;
    case AppCommands::zoomInHorizontal:
        zoomHorizontal(1.15f, viewport.getViewWidth() / 2);
        return true;
    case AppCommands::zoomOutHorizontal:
        zoomHorizontal(1.0f / 1.15f, viewport.getViewWidth() / 2);
        return true;
    case AppCommands::zoomInVertical:
        zoomVertical(1.15f, viewport.getViewHeight() / 2);
        return true;
    case AppCommands::zoomOutVertical:
        zoomVertical(1.0f / 1.15f, viewport.getViewHeight() / 2);
        return true;
    case AppCommands::zoomReset:
        setHorizontalZoom(PianoRollComponent::defaultBeatWidth, viewport.getViewWidth() / 2);
        setVerticalZoom(PianoRollComponent::defaultNoteHeight, viewport.getViewHeight() / 2);
        return true;
    case AppCommands::toggleLoop:
        transportBar.toggleLoop();
        return true;
    default:
        return false;
    }
}

void MainComponent::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(surface::surface2);
    g.fillRect(toolBarBounds);
    g.setColour(border::strong);
    g.drawHorizontalLine(toolBarBounds.getBottom() - 1, static_cast<float>(toolBarBounds.getX()),
                         static_cast<float>(toolBarBounds.getRight()));
    g.drawVerticalLine(toolBarSeparatorX, static_cast<float>(toolBarBounds.getY() + 8),
                       static_cast<float>(toolBarBounds.getBottom() - 8));

    if (!trackListHeaderBounds.isEmpty())
    {
        g.setColour(surface::bg2);
        g.fillRect(trackListHeaderBounds);
        g.setColour(border::soft);
        g.drawHorizontalLine(trackListHeaderBounds.getBottom() - 1, static_cast<float>(trackListHeaderBounds.getX()),
                             static_cast<float>(trackListHeaderBounds.getRight()));
        int numTracks = document.getSequence().getNumTracks();
        g.setColour(text::t3);
        g.setFont(font::sans(font::sizeXS));
        g.drawText(juce::String::fromUTF8("TRACKS \xc2\xb7 ") + juce::String(numTracks),
                   trackListHeaderBounds.reduced(12, 0), juce::Justification::centredLeft);
    }

    if (fileDragOver)
    {
        g.setColour(surface::press);
        g.fillRect(getLocalBounds());
        g.setColour(accent::base);
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
            pluginHost.detachAllPlugins();
            if (document.loadFrom(file))
            {
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
    transportBar.setBounds(area.removeFromBottom(transportBarHeight));

    int clampedTrackListW = juce::jlimit(80, juce::jmax(80, area.getWidth() - eventListWidth - 200), trackListWidth);
    auto trackListColumn = area.removeFromLeft(clampedTrackListW);
    trackListPanelBounds = trackListColumn;
    trackListHeaderBounds = trackListColumn.removeFromTop(toolBarHeight);
    trackListViewport.setBounds(trackListColumn);
    trackList.setSize(trackListViewport.getMaximumVisibleWidth(), trackList.getHeight());
    trackListDivider.setBounds(area.removeFromLeft(dividerThickness));
    int clampedEventListW = juce::jlimit(80, juce::jmax(80, area.getWidth() - 200), eventListWidth);
    auto eventListColumn = area.removeFromRight(clampedEventListW);
    eventListPanelBounds = eventListColumn;
    eventList.setBounds(eventListColumn);
    eventListDivider.setBounds(area.removeFromRight(dividerThickness));
    pianoRollPanelBounds = area;

    auto toolBarArea = area.removeFromTop(toolBarHeight);
    toolBarBounds = toolBarArea;
    {
        const int btnSize = 28;
        const int pad = 4;
        auto toolBtnArea =
            toolBarArea.withTrimmedLeft(pad * 2).withSizeKeepingCentre(toolBarArea.getWidth() - pad * 2, btnSize);
        selectToolButton.setBounds(toolBtnArea.removeFromLeft(btnSize));
        toolBtnArea.removeFromLeft(pad);
        editToolButton.setBounds(toolBtnArea.removeFromLeft(btnSize));
        toolBtnArea.removeFromLeft(pad * 2);
        toolBarSeparatorX = toolBtnArea.getX();
        toolBtnArea.removeFromLeft(pad * 2);
        quantizeComboBox.setBounds(toolBtnArea.removeFromLeft(70).withSizeKeepingCentre(70, 26));
    }

    int clampedEditorH = juce::jlimit(0, area.getHeight() - 100, controllerLaneHeight);
    auto editorArea = area.removeFromBottom(clampedEditorH);
    auto divArea = area.removeFromBottom(dividerThickness);

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

    updateFocusBorder();
}

void MainComponent::onVBlank()
{
    double tick = playbackEngine.getCurrentTick();
    pianoRoll.setPlayheadTick(tick);
    controllerLane.setPlayheadTick(tick);
    eventList.setPlayheadTick(tick);
    transportBar.updateDisplay();
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
        controllerLane.setContentBeats(pianoRoll.getContentBeats());

        viewport.setViewPosition(desiredX, viewport.getViewPositionY());
    }
}

void MainComponent::scrollNoteIntoView(int startTick, int noteNumber)
{
    int noteX = pianoRoll.tickToX(startTick);
    int noteY = pianoRoll.noteToY(noteNumber);
    int noteH = pianoRoll.getNoteHeight();

    int viewX = viewport.getViewPositionX();
    int viewW = viewport.getViewWidth();
    int viewY = viewport.getViewPositionY();
    int viewH = viewport.getViewHeight();

    int newX = viewX;
    if (noteX < viewX + PianoRollComponent::keyboardWidth)
        newX = noteX - PianoRollComponent::keyboardWidth;
    else if (noteX > viewX + viewW - 20)
        newX = noteX - viewW + 20;

    newX = juce::jmax(0, newX);
    while (newX + viewW > pianoRoll.getWidth())
        pianoRoll.extendContent();
    controllerLane.setContentBeats(pianoRoll.getContentBeats());

    int newY = viewY;
    if (noteY < viewY + PianoRollComponent::gridTopOffset)
        newY = noteY - PianoRollComponent::gridTopOffset;
    else if (noteY + noteH > viewY + viewH)
        newY = noteY + noteH - viewH;

    newY = juce::jmax(0, newY);

    if (newX != viewX || newY != viewY)
        viewport.setViewPosition(newX, newY);
}

void MainComponent::scrollViewVertically(int deltaY)
{
    int newY = viewport.getViewPositionY() + deltaY;
    newY = juce::jlimit(0, juce::jmax(0, pianoRoll.getHeight() - viewport.getViewHeight()), newY);
    viewport.setViewPosition(viewport.getViewPositionX(), newY);
}

void MainComponent::scrollViewHorizontally(int deltaX)
{
    int newX = viewport.getViewPositionX() + deltaX;
    newX = juce::jlimit(0, juce::jmax(0, pianoRoll.getWidth() - viewport.getViewWidth()), newX);
    viewport.setViewPosition(newX, viewport.getViewPositionY());
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
    pluginHost.detachAllPlugins();
    document.newDocument();
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
                                 if (file != juce::File{} && document.saveTo(file))
                                     updateTitleBar();
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
                                 pluginHost.detachAllPlugins();
                                 if (document.loadFrom(file))
                                 {
                                     onSequenceLoaded();
                                     updateTitleBar();
                                 }
                             });
}

void MainComponent::showAudioSettings()
{
    auto* selector = new juce::AudioDeviceSelectorComponent(audioDeviceManager, 0, 0, 2, 2, false, false, true, false);
    selector->setSize(500, 450);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(selector);
    options.dialogTitle = "Audio Settings";
    options.dialogBackgroundColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.launchAsync();
}

void MainComponent::stopPlayback()
{
    playbackEngine.stop();
    midiOutput.reset();
    transportBar.setPlaying(false);
    vblankAttachment.reset();
}

PlaybackTrackContext MainComponent::makeTrackContext(int trackIndex) const
{
    PlaybackTrackContext ctx;
    ctx.trackIndex = trackIndex;
    if (trackIndex < 0 || trackIndex >= document.getSequence().getNumTracks())
        return ctx;
    const auto& track = document.getSequence().getTrack(trackIndex);
    ctx.channel = track.getChannel();
    ctx.destination = track.getOutputDestination();
    const int rt = track.getRouteTargetTrackIndex();
    ctx.routeTarget = (rt >= 0 && rt < document.getSequence().getNumTracks()) ? rt : trackIndex;
    return ctx;
}

void MainComponent::onSequenceLoaded()
{
    playbackEngine.setPositionInTicks(0);
    playbackEngine.setLoopEnabled(false);
    playbackEngine.setLoopRange(0, 0);
    transportBar.setLoopActive(false);
    pianoRoll.setLoopRegion(false, 0, 0);
    controllerLane.setLoopRegion(false, 0, 0);

    std::set<int> allTracks;
    for (int i = 0; i < document.getSequence().getNumTracks(); ++i)
        allTracks.insert(i);

    pianoRoll.setSequence(&document.getSequence());
    pianoRoll.setSelectedTracks(0, allTracks);
    pianoRoll.setPlayheadTick(0);
    transportBar.updateDisplay();

    trackList.setSequence(&document.getSequence());

    controllerLane.setSequence(&document.getSequence());
    controllerLane.setContentBeats(pianoRoll.getContentBeats());
    controllerLane.setSelectedTracks(0, allTracks);
    controllerLane.setPlayheadTick(0);

    eventList.setSequence(&document.getSequence());
    eventList.setSelectedTracks(allTracks);
    eventList.setPlayheadTick(0);

    int c4Y = PianoRollComponent::gridTopOffset + (127 - 60) * pianoRoll.noteHeight - getHeight() / 2;
    viewport.setViewPosition(0, c4Y);
    repaint(trackListHeaderBounds);

    playbackEngine.rebuildSnapshot();
    document.getSequence().notifySequenceReset();
}

void MainComponent::updateTitleBar()
{
    if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
    {
        auto appName = juce::JUCEApplication::getInstance()->getApplicationName();
        if (document.getCurrentFile() != juce::File{})
            window->setName(document.getCurrentFile().getFileName() + " - " + appName);
        else
            window->setName(appName);
    }
}
