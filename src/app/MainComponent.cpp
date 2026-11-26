#include "app/MainComponent.h"
#include "model/grid/GridDivision.h"
#include "settings/AppProperties.h"
#include "ui/commands/AppCommands.h"
#include "ui/theme/Theme.h"
#include "ui/widgets/NoTabFocusTraverser.h"
#include <ranges>
#include <set>
#include <utility>

MainComponent::MainComponent()
{
    document.newDocument();
    document.addChangeListener(this);
    document.onWillReplaceSequence = [this] { stopPlayback(); };
    audio.getPluginHost().setSequence(&document.getSequence());
    document.pluginStateSource = [this](TrackId trackId) { return audio.getPluginHost().getPluginState(trackId); };
    audio.getPluginHost().onPluginStateChanged = [this] { document.markChanged(); };
    document.onWillSave = [this] { audio.getPluginHost().flushPendingStateChanges(); };

    audio.getPlaybackEngine().setSequence(&document.getSequence());

    pianoRollPanel.setSequence(&document.getSequence());

    pianoRollPanel.getPianoRoll().onNotePreview = [this](const MidiNote& note)
    {
        auto ctx = makeTrackContext(pianoRollPanel.getPianoRoll().getActiveTrackIndex());
        audio.getMidiOutput().onNoteOn(ctx, note);
        audio.getPluginHost().onNoteOn(ctx, note);
    };
    pianoRollPanel.getPianoRoll().onNotePreviewEnd = [this](const MidiNote& note)
    {
        auto ctx = makeTrackContext(pianoRollPanel.getPianoRoll().getActiveTrackIndex());
        audio.getMidiOutput().onNoteOff(ctx, note);
        audio.getPluginHost().onNoteOff(ctx, note);
    };
    addAndMakeVisible(pianoRollPanel);

    trackListPanel.setSequence(&document.getSequence());
    addAndMakeVisible(trackListPanel);
    trackListPanel.getTrackList().pluginNameForTrack = [this](int trackIndex)
    { return audio.getPluginHost().getPluginName(document.getSequence().getTrack(trackIndex).getId()); };
    trackListPanel.getTrackList().onEditorButtonClicked = [this](int trackIndex)
    { editorController.showEditor(document.getSequence().getTrack(trackIndex).getId()); };
    trackListPanel.getTrackList().onPluginLabelClicked = [this](int trackIndex)
    { trackOutput.showOutputMenu(trackIndex); };

    addAndMakeVisible(trackListDivider);
    trackListDivider.onDragStart = [this]() { trackListWidthOnDragStart = trackListWidth; };
    trackListDivider.onDrag = [this](int deltaX)
    {
        int minW = 80;
        int maxW = juce::jmax(minW, getWidth() - 2 * panelGap - eventListWidth - 200);
        trackListWidth = juce::jlimit(minW, maxW, trackListWidthOnDragStart + deltaX);
        resized();
    };

    addAndMakeVisible(eventListDivider);
    eventListDivider.onDragStart = [this]() { eventListWidthOnDragStart = eventListWidth; };
    eventListDivider.onDrag = [this](int deltaX)
    {
        int minW = 80;
        int maxW = juce::jmax(minW, getWidth() - 2 * panelGap - trackListWidth - 200);
        eventListWidth = juce::jlimit(minW, maxW, eventListWidthOnDragStart - deltaX);
        resized();
    };

    eventList.setSequence(&document.getSequence());
    eventList.setSelectedTracks({0});
    addAndMakeVisible(eventList);

    menuBar.setModel(&mainMenuModel);
    addAndMakeVisible(menuBar);

    addAndMakeVisible(transportBar);
    transportBar.onRecordToggled = [this] { toggleRecording(); };
    recorder.onRecordingChanged = [this](bool recording) { transportBar.setRecordActive(recording); };
    pianoRollPanel.getPianoRoll().setRecordingLayer(&recorder, &recorder);
    pianoRollPanel.getControllerLane().setRecordingLayer(&recorder, &recorder);
    eventList.setRecordingLayer(&recorder);
    recorder.onLayerChanged = [this]
    {
        pianoRollPanel.getPianoRoll().recordingLayerChanged();
        pianoRollPanel.getControllerLane().recordingLayerChanged();
        eventList.recordingLayerChanged();
    };
    recorder.onCommitted = [this](const auto& committed)
    { pianoRollPanel.getPianoRoll().recordedNotesCommitted(committed); };
    transportBar.setMetronomeActive(audio.getPlaybackEngine().isMetronomeEnabled());

    pianoRollPanel.onGridDivisionChanged = [this](GridDivision) { updateInputQuantize(); };
    inputQuantizeEnabled = getAppProperties().getUserSettings()->getBoolValue("inputQuantizeEnabled", false);
    updateInputQuantize();

    transportBar.updateDisplay();

    eventList.setWantsKeyboardFocus(true);
    addAndMakeVisible(trackListFrame);
    addAndMakeVisible(pianoRollFrame);
    addAndMakeVisible(eventListFrame);
    updateFocusBorder();
    juce::Desktop::getInstance().addFocusChangeListener(this);

    commandManager.registerAllCommandsForTarget(this);
    setWantsKeyboardFocus(true);
    setSize(1280, 800);

    pianoRollPanel.scrollToMiddleC(getHeight() / 2);

    // Asks once the main window is showing.
    juce::MessageManager::callAsync(
        [safeThis = juce::Component::SafePointer<MainComponent>(this)]
        {
            if (safeThis != nullptr)
                safeThis->recovery.offer();
        });
}

MainComponent::~MainComponent()
{
    // Windows destroys owned windows along with their owner, so these must go before the main window does.
    metronomeSettings.closeWindow();
    recovery.closeWindow();
    editorController.closeAllEditors();
    document.removeChangeListener(this);
    juce::Desktop::getInstance().removeFocusChangeListener(this);
    audio.detachInputs();
    menuBar.setModel(nullptr);
    playheadLinks.stopFollowing();
    audio.getPlaybackEngine().stop();
    pianoRollPanel.getPianoRoll().setRecordingLayer(nullptr, nullptr);
    pianoRollPanel.getControllerLane().setRecordingLayer(nullptr, nullptr);
    eventList.setRecordingLayer(nullptr);
    audio.shutDown();
}

void MainComponent::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(surface::bg2);
    for (auto* frame : {&trackListFrame, &pianoRollFrame, &eventListFrame})
        g.fillRect(frame->getBounds());
}

void MainComponent::paintOverChildren(juce::Graphics& g)
{
    if (!fileDragOver)
        return;

    using namespace calliope::theme;
    const float outset = panelGap * 0.5f;
    auto area = trackListFrame.getBounds().getUnion(eventListFrame.getBounds()).toFloat().expanded(outset);
    const float cornerRadius = radius::r3 + outset;
    g.setColour(surface::press);
    g.fillRoundedRectangle(area, cornerRadius);
    g.setColour(accent::base);
    g.drawRoundedRectangle(area.reduced(1.0f), cornerRadius - 1.0f, 2.0f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    menuBar.setBounds(area.removeFromTop(menuBarHeight));
    transportBar.setBounds(area.removeFromBottom(transportBarHeight));
    area.reduce(panelGap, panelGap);

    int clampedTrackListW = juce::jlimit(80, juce::jmax(80, area.getWidth() - eventListWidth - 200), trackListWidth);
    auto trackListColumn = area.removeFromLeft(clampedTrackListW);
    trackListFrame.setBounds(trackListColumn);
    trackListColumn.reduce(panelPadding, panelPadding);
    trackListPanel.setBounds(trackListColumn);
    trackListDivider.setBounds(area.removeFromLeft(panelGap));
    int clampedEventListW = juce::jlimit(80, juce::jmax(80, area.getWidth() - 200), eventListWidth);
    auto eventListColumn = area.removeFromRight(clampedEventListW);
    eventListFrame.setBounds(eventListColumn);
    eventListColumn.reduce(panelPadding, panelPadding);
    eventList.setBounds(eventListColumn);
    eventListDivider.setBounds(area.removeFromRight(panelGap));
    pianoRollFrame.setBounds(area);
    area.reduce(panelPadding, panelPadding);
    pianoRollPanel.setBounds(area);
}

void MainComponent::parentHierarchyChanged()
{
    if (auto* topLevel = getTopLevelComponent())
        topLevel->addKeyListener(commandManager.getKeyMappings());
    updateTitleBar();
}

std::unique_ptr<juce::ComponentTraverser> MainComponent::createKeyboardFocusTraverser()
{
    return std::make_unique<NoTabFocusTraverser>();
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &document)
    {
        updateTitleBar();
    }
}

void MainComponent::globalFocusChanged(juce::Component* focusedComponent)
{
    if (focusedComponent == nullptr)
        return;

    auto belongsTo = [focusedComponent](const juce::Component& panel)
    { return &panel == focusedComponent || panel.isParentOf(focusedComponent); };

    FocusPanel detected = focusedPanel;
    if (trackListPanel.claimsFocus(focusedComponent))
    {
        detected = FocusPanel::TrackList;
    }
    else if (pianoRollPanel.claimsFocus(focusedComponent))
    {
        detected = FocusPanel::PianoRoll;
    }
    else if (belongsTo(eventList))
    {
        detected = FocusPanel::EventList;
    }
    else
    {
        return;
    }

    if (detected == focusedPanel)
        return;

    focusedPanel = detected;
    updateFocusBorder();
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
    case AppCommands::undo:
        result.setActive(document.getHistory().canUndo());
        break;
    case AppCommands::redo:
        result.setActive(document.getHistory().canRedo());
        break;
    case AppCommands::cut:
    case AppCommands::copy:
        result.setActive(pianoRollPanel.canCutOrCopy());
        break;
    case AppCommands::paste:
        result.setActive(pianoRollPanel.canPaste());
        break;
    case AppCommands::selectAll:
        result.setActive(focusedPanel == FocusPanel::PianoRoll && pianoRollPanel.canSelectAll());
        break;
    case AppCommands::moveNotesUp:
    case AppCommands::moveNotesDown:
    case AppCommands::quantizeNotes:
        result.setActive(focusedPanel == FocusPanel::PianoRoll && pianoRollPanel.hasSelectedNotes());
        break;
    case AppCommands::moveSelectionPrev:
    case AppCommands::moveSelectionNext:
        result.setActive(focusedPanel == FocusPanel::PianoRoll && pianoRollPanel.hasNotesInActiveTrack());
        break;
    case AppCommands::scrollViewUp:
    case AppCommands::scrollViewDown:
    case AppCommands::scrollViewLeft:
    case AppCommands::scrollViewRight:
        result.setActive(focusedPanel == FocusPanel::PianoRoll);
        break;
    case AppCommands::toggleInputQuantize:
        result.setTicked(inputQuantizeEnabled);
        break;
    case AppCommands::toggleCountIn:
        result.setTicked(metronomeSettings.isCountInEnabled());
        break;
    case AppCommands::insertSilence:
    case AppCommands::deleteTime:
        result.setActive(!recorder.isRecording());
        break;
    default:
        break;
    }
}

bool MainComponent::perform(const InvocationInfo& info)
{
    switch (info.commandID)
    {
    case AppCommands::newFile:
        fileOperations.newFile();
        return true;
    case AppCommands::openFile:
        fileOperations.open();
        return true;
    case AppCommands::saveFile:
        fileOperations.save();
        return true;
    case AppCommands::saveFileAs:
        fileOperations.saveAs();
        return true;
    case AppCommands::importMidi:
        fileOperations.importMidi();
        return true;
    case AppCommands::exportMidi:
        fileOperations.exportMidi();
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
        int currentTick = static_cast<int>(audio.getPlaybackEngine().getCurrentTick());
        auto bbt = document.getSequence().getTimeline().tickToBarBeatTick(currentTick);
        int targetBar = juce::jmax(1, bbt.bar - 1);
        transportBar.jumpToTick(document.getSequence().getTimeline().barStartToTick(targetBar));
        return true;
    }
    case AppCommands::nextBar:
    {
        int currentTick = static_cast<int>(audio.getPlaybackEngine().getCurrentTick());
        auto bbt = document.getSequence().getTimeline().tickToBarBeatTick(currentTick);
        transportBar.jumpToTick(document.getSequence().getTimeline().barStartToTick(bbt.bar + 1));
        return true;
    }
    case AppCommands::switchToEditTool:
        pianoRollPanel.setActiveTool(PianoRollComponent::EditMode::Edit);
        return true;
    case AppCommands::switchToSelectTool:
        pianoRollPanel.setActiveTool(PianoRollComponent::EditMode::Select);
        return true;
    case AppCommands::toggleSnap:
        pianoRollPanel.toggleSnap();
        return true;
    case AppCommands::undo:
        document.getHistory().undo();
        trackListPanel.getTrackList().refresh();
        pianoRollPanel.deselectNotes();
        return true;
    case AppCommands::redo:
        document.getHistory().redo();
        trackListPanel.getTrackList().refresh();
        pianoRollPanel.deselectNotes();
        return true;
    case AppCommands::cut:
        pianoRollPanel.cut();
        return true;
    case AppCommands::copy:
        pianoRollPanel.copy();
        return true;
    case AppCommands::paste:
    {
        pianoRollPanel.paste(static_cast<int>(audio.getPlaybackEngine().getCurrentTick()));
        return true;
    }
    case AppCommands::selectAll:
        if (focusedPanel != FocusPanel::PianoRoll)
            return true;
        pianoRollPanel.selectAll();
        return true;
    case AppCommands::moveNotesUp:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRollPanel.nudgeSelectedNotesPitch(1);
        return true;
    case AppCommands::moveNotesDown:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRollPanel.nudgeSelectedNotesPitch(-1);
        return true;
    case AppCommands::moveSelectionPrev:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRollPanel.moveSelectionToAdjacentNote(-1);
        return true;
    case AppCommands::moveSelectionNext:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRollPanel.moveSelectionToAdjacentNote(1);
        return true;
    case AppCommands::scrollViewUp:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRollPanel.scrollViewUp();
        return true;
    case AppCommands::scrollViewDown:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRollPanel.scrollViewDown();
        return true;
    case AppCommands::scrollViewLeft:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRollPanel.scrollViewLeft();
        return true;
    case AppCommands::scrollViewRight:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRollPanel.scrollViewRight();
        return true;
    case AppCommands::zoomInHorizontal:
        pianoRollPanel.zoomInHorizontally();
        return true;
    case AppCommands::zoomOutHorizontal:
        pianoRollPanel.zoomOutHorizontally();
        return true;
    case AppCommands::zoomInVertical:
        pianoRollPanel.zoomInVertically();
        return true;
    case AppCommands::zoomOutVertical:
        pianoRollPanel.zoomOutVertically();
        return true;
    case AppCommands::zoomReset:
        pianoRollPanel.resetZoom();
        return true;
    case AppCommands::toggleLoop:
        transportBar.toggleLoop();
        return true;
    case AppCommands::toggleMetronome:
        transportBar.toggleMetronome();
        return true;
    case AppCommands::toggleRecord:
        transportBar.toggleRecord();
        return true;
    case AppCommands::quantizeNotes:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRollPanel.quantizeSelectedNotes();
        return true;
    case AppCommands::toggleInputQuantize:
        toggleInputQuantize();
        return true;
    case AppCommands::toggleCountIn:
        metronomeSettings.toggleCountIn();
        return true;
    case AppCommands::showMetronomeSettings:
        metronomeSettings.showWindow();
        return true;
    case AppCommands::insertSilence:
        timeEdits.insertSilence();
        return true;
    case AppCommands::deleteTime:
        timeEdits.deleteTime();
        return true;
    default:
        return false;
    }
}

bool MainComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    return FileOperations::canOpenAny(files);
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

    fileOperations.openDropped(files);
}

void MainComponent::saveIfNeededThen(std::function<void()> next)
{
    fileOperations.saveIfNeededThen(std::move(next));
}

void MainComponent::stopAutoSaveAndDiscardRecovery()
{
    recovery.stopAndDiscard();
}

void MainComponent::toggleInputQuantize()
{
    inputQuantizeEnabled = !inputQuantizeEnabled;
    getAppProperties().getUserSettings()->setValue("inputQuantizeEnabled", inputQuantizeEnabled);
    updateInputQuantize();
}

void MainComponent::updateInputQuantize()
{
    recorder.setInputQuantize(inputQuantizeEnabled ? std::optional(pianoRollPanel.getGridDivision()) : std::nullopt);
}

void MainComponent::updateFocusBorder()
{
    trackListFrame.setFocused(focusedPanel == FocusPanel::TrackList);
    pianoRollFrame.setFocused(focusedPanel == FocusPanel::PianoRoll);
    eventListFrame.setFocused(focusedPanel == FocusPanel::EventList);
}

void MainComponent::stopPlayback()
{
    audio.getPlaybackEngine().stop();
    audio.getMidiOutput().reset();
    transportBar.setPlaying(false);
    playheadLinks.stopFollowing();
}

void MainComponent::toggleRecording()
{
    if (recorder.isRecording())
        recorder.finish();
    else
        recorder.start();
}

void MainComponent::onSequenceLoaded()
{
    audio.getPlaybackEngine().setPositionInTicks(0);
    audio.getPlaybackEngine().setLoopEnabled(false);
    audio.getPlaybackEngine().setLoopRange(0, 0);
    transportBar.setLoopActive(false);

    const auto allTracks = std::views::iota(0, document.getSequence().getNumTracks()) | std::ranges::to<std::set>();

    pianoRollPanel.setSequence(&document.getSequence());
    pianoRollPanel.setSelectedTracks(0, allTracks);
    pianoRollPanel.setPlayheadTick(0);
    pianoRollPanel.setLoopRegion(false, 0, 0);
    transportBar.updateDisplay();

    trackListPanel.setSequence(&document.getSequence());

    eventList.setSequence(&document.getSequence());
    eventList.setSelectedTracks(allTracks);
    eventList.setPlayheadTick(0);

    pianoRollPanel.scrollToMiddleC(getHeight() / 2);
}

void MainComponent::updateTitleBar()
{
    if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
    {
        auto appName = juce::JUCEApplication::getInstance()->getApplicationName();
        auto marker = juce::String(document.hasChangedSinceSaved() ? "*" : "");
        window->setName(marker + document.getDocumentTitle() + " - " + appName);
    }
}

PlaybackTrackContext MainComponent::makeTrackContext(int trackIndex) const
{
    if (trackIndex < 0 || trackIndex >= document.getSequence().getNumTracks())
        return {};
    return makePlaybackTrackContext(document.getSequence(), trackIndex);
}
