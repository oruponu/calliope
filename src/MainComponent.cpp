#include "MainComponent.h"
#include "AppProperties.h"
#include "io/MidiFileIO.h"
#include "model/UndoActions.h"
#include "ui/Theme.h"

namespace
{
constexpr const char* kStructuralTxn = "Track structure";
}

void MainComponent::Divider::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    g.setColour(border::strong);
    g.fillRect(getLocalBounds());
    g.setColour(text::t4);
    auto cy = getHeight() / 2.0f;
    auto cx = getWidth() / 2.0f;
    for (float d : {-12.0f, -4.0f, 4.0f, 12.0f})
    {
        if (orientation == Horizontal)
            g.fillEllipse(cx + d - 1.0f, cy - 1.0f, 2.0f, 2.0f);
        else
            g.fillEllipse(cx - 1.0f, cy + d - 1.0f, 2.0f, 2.0f);
    }
}

void MainComponent::Divider::mouseDown(const juce::MouseEvent&)
{
    if (onDragStart)
        onDragStart();
}

void MainComponent::Divider::mouseDrag(const juce::MouseEvent& e)
{
    if (!onDrag)
        return;
    onDrag(orientation == Horizontal ? e.getDistanceFromDragStartY() : e.getDistanceFromDragStartX());
}

void MainComponent::ZoomStrip::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    g.setColour(surface::bg2);
    g.fillRect(getLocalBounds());

    auto drawSymbol = [&](const juce::Rectangle<int>& bounds, bool isMinus)
    {
        bool hover = bounds.contains(getMouseXYRelative());
        float alpha = hover ? 0.9f : 0.5f;
        g.setColour(text::t1.withAlpha(alpha));
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

void MainComponent::FocusBorder::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    g.setColour(text::t2);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), radius::r2, 1.0f);
}

void MainComponent::TransportButton::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    auto bounds = getLocalBounds().toFloat();
    bool hover = isMouseOver();

    juce::Colour boxColour, boxBorder, iconColour;
    if (type == Play && active)
    {
        boxColour = accent::base;
        boxBorder = accent::base;
        iconColour = surface::bg;
    }
    else if (type == Loop && active)
    {
        boxColour = accent::soft;
        boxBorder = accent::dim;
        iconColour = accent::strong;
    }
    else
    {
        boxColour = hover ? surface::surface3 : surface::surface2;
        boxBorder = hover ? border::strong : border::normal;
        iconColour = hover ? text::t1 : text::t2;
    }

    g.setColour(boxColour);
    g.fillRoundedRectangle(bounds, radius::r2);
    g.setColour(boxBorder);
    g.drawRoundedRectangle(bounds.reduced(0.5f), radius::r2, 1.0f);

    g.setColour(iconColour);

    if (type == ReturnToStart)
    {
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
        auto size = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.42f;
        g.fillRoundedRectangle(bounds.withSizeKeepingCentre(size, size), 2.0f);
    }
    else if (type == Play)
    {
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
    using namespace calliope::theme;
    auto bounds = getLocalBounds().toFloat();
    bool hover = isMouseOver();

    juce::Colour boxColour, boxBorder, iconColour;
    if (active)
    {
        boxColour = accent::soft;
        boxBorder = accent::dim;
        iconColour = accent::strong;
    }
    else
    {
        boxColour = hover ? surface::surface3 : surface::surface2;
        boxBorder = hover ? border::strong : border::normal;
        iconColour = hover ? text::t1 : text::t2;
    }

    g.setColour(boxColour);
    g.fillRoundedRectangle(bounds, radius::r2);
    g.setColour(boxBorder);
    g.drawRoundedRectangle(bounds.reduced(0.5f), radius::r2, 1.0f);

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

    if (auto xml = getAppProperties().getUserSettings()->getXmlValue("knownPluginList"))
        knownPluginList.recreateFromXml(*xml);
    knownPluginList.addChangeListener(this);

    auto savedAudioState = getAppProperties().getUserSettings()->getXmlValue("audioDeviceState");
    audioDeviceManager.initialise(0, 2, savedAudioState.get(), true);
    audioDeviceManager.addChangeListener(this);

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
        playbackEngine.rebuildSnapshot();
    };
    pianoRoll.onTempoChanged = [this]()
    {
        updateTransportDisplay();
        controllerLane.repaint();
        eventList.refresh();
        playbackEngine.rebuildSnapshot();
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
    trackList.onMuteSoloChanged = [this]() { playbackEngine.rebuildSnapshot(); };
    trackList.pluginNameForTrack = [this](int trackIndex) { return pluginHost.getPluginName(trackIndex); };
    trackList.onEditorButtonClicked = [this](int trackIndex) { pluginHost.showEditor(trackIndex); };
    trackList.onChannelLabelClicked = [this](int trackIndex)
    {
        int currentCh = sequence.getTrack(trackIndex).getChannel();
        juce::PopupMenu menu;
        menu.addSectionHeader("Channel");
        for (int ch = 1; ch <= 16; ++ch)
        {
            menu.addItem(juce::String(ch), true, ch == currentCh,
                         [this, trackIndex, ch]()
                         {
                             int currentChannel = sequence.getTrack(trackIndex).getChannel();
                             if (currentChannel == ch)
                                 return;
                             undoManager.beginNewTransaction();
                             undoManager.perform(
                                 new ChannelChangeAction(&sequence, trackIndex, currentChannel, ch, [this](int idx)
                                                         { playbackEngine.releaseActiveNotesForTrack(idx); }));
                             playbackEngine.rebuildSnapshot();
                             trackList.repaint();
                             pianoRoll.repaint();
                             eventList.refresh();
                         });
        }
        menu.showMenuAsync(juce::PopupMenu::Options{});
    };
    trackList.onAddTrackRequested = [this]()
    {
        undoManager.beginNewTransaction(kStructuralTxn);
        undoManager.perform(new TrackAddAction(&sequence,
                                               [this](int idx)
                                               {
                                                   playbackEngine.releaseActiveNotesForTrack(idx);
                                                   pluginHost.detachPlugin(idx);
                                               }));
        trackList.refresh();
        pianoRoll.repaint();
        controllerLane.repaint();
        eventList.refresh();
        repaint(trackListHeaderBounds);
        playbackEngine.rebuildSnapshot();
    };
    trackList.onRemoveTrackRequested = [this](int trackIndex)
    {
        if (sequence.getNumTracks() <= 1)
            return;

        bool wasRunning = playbackEngine.suspendForStructuralChange();
        undoManager.beginNewTransaction(kStructuralTxn);
        undoManager.perform(new TrackRemoveAction(
            &sequence, trackIndex, [this](int idx) { pluginHost.detachPlugin(idx); },
            [this](int from, int delta) { pluginHost.renumberTrackIndices(from, delta); }));
        playbackEngine.resumeAfterStructuralChange(wasRunning);

        int newActive = juce::jlimit(0, sequence.getNumTracks() - 1, trackIndex);
        trackList.refresh();
        trackList.setActiveTrackIndex(newActive);
        repaint(trackListHeaderBounds);
    };
    trackList.onTrackRenamed = [this](int trackIndex, juce::String newName)
    {
        if (trackIndex < 0 || trackIndex >= sequence.getNumTracks())
            return;
        std::string oldName = sequence.getTrack(trackIndex).getName();
        std::string requested = newName.toStdString();
        if (oldName == requested)
            return;
        undoManager.beginNewTransaction();
        undoManager.perform(new TrackRenameAction(&sequence, trackIndex, oldName, requested));
        trackList.repaint();
    };
    trackList.onPluginLabelClicked = [this](int trackIndex)
    {
        auto types = knownPluginList.getTypes();

        juce::PopupMenu menu;
        const auto& currentTrack = sequence.getTrack(trackIndex);
        auto currentDest = currentTrack.getOutputDestination();
        int currentRouteTarget = currentTrack.getRouteTargetTrackIndex();

        menu.addSectionHeader("Output");
        for (int i = 0; i < sequence.getNumTracks(); ++i)
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
                             auto& track = sequence.getTrack(trackIndex);
                             track.setRouteTargetTrackIndex(isOwn ? -1 : i);
                             track.setOutputDestination(MidiTrack::OutputDestination::Plugin);
                             playbackEngine.rebuildSnapshot();
                             trackList.repaint();
                         });
        }

        menu.addItem("MIDI Device", true, currentDest == MidiTrack::OutputDestination::MidiDevice,
                     [this, trackIndex]()
                     {
                         playbackEngine.releaseActiveNotesForTrack(trackIndex);
                         sequence.getTrack(trackIndex).setOutputDestination(MidiTrack::OutputDestination::MidiDevice);
                         playbackEngine.rebuildSnapshot();
                         trackList.repaint();
                     });

        menu.addItem("None", true, currentDest == MidiTrack::OutputDestination::None,
                     [this, trackIndex]()
                     {
                         playbackEngine.releaseActiveNotesForTrack(trackIndex);
                         sequence.getTrack(trackIndex).setOutputDestination(MidiTrack::OutputDestination::None);
                         playbackEngine.rebuildSnapshot();
                         trackList.repaint();
                     });
        menu.addSeparator();

        menu.addItem("Load Plugin...", true, false,
                     [this, trackIndex]()
                     {
                         fileChooser = std::make_unique<juce::FileChooser>("Load Plugin", juce::File{}, "*.vst3");
                         fileChooser->launchAsync(
                             juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [this, trackIndex](const juce::FileChooser& fc)
                             {
                                 auto file = fc.getResult();
                                 if (file == juce::File{})
                                     return;
                                 if (playbackEngine.isPlaying())
                                     stopPlayback();
                                 if (pluginHost.attachPlugin(trackIndex, file))
                                 {
                                     auto& track = sequence.getTrack(trackIndex);
                                     track.setRouteTargetTrackIndex(-1);
                                     track.setOutputDestination(MidiTrack::OutputDestination::Plugin);
                                     playbackEngine.rebuildSnapshot();
                                 }
                                 trackList.repaint();
                             });
                     });

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
                         sequence.getTrack(trackIndex).setOutputDestination(MidiTrack::OutputDestination::MidiDevice);
                         playbackEngine.rebuildSnapshot();
                         trackList.repaint();
                     });

        menu.showMenuAsync(juce::PopupMenu::Options{},
                           [this, trackIndex, types](int result)
                           {
                               int index = juce::KnownPluginList::getIndexChosenByMenu(types, result);
                               if (index < 0)
                                   return;
                               if (playbackEngine.isPlaying())
                                   stopPlayback();
                               if (pluginHost.attachPlugin(trackIndex, types.getReference(index)))
                               {
                                   auto& track = sequence.getTrack(trackIndex);
                                   track.setRouteTargetTrackIndex(-1);
                                   track.setOutputDestination(MidiTrack::OutputDestination::Plugin);
                                   playbackEngine.rebuildSnapshot();
                               }
                               trackList.repaint();
                           });
    };

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

    using namespace calliope::theme;
    auto headerColour = text::t2;
    auto headerFont = font::sans(font::sizeXS);

    for (auto* label : {&positionHeaderLabel, &timeSigHeaderLabel, &keyHeaderLabel, &tempoHeaderLabel})
    {
        addAndMakeVisible(label);
        label->setFont(headerFont);
        label->setColour(juce::Label::textColourId, headerColour);
        label->setJustificationType(juce::Justification::centred);
    }

    for (auto* sep : {&positionDot1, &positionDot2, &timeSigSlashLabel})
    {
        addAndMakeVisible(sep);
        sep->setFont(font::mono(font::sizeDisplay).boldened());
        sep->setColour(juce::Label::textColourId, text::t1);
        sep->setJustificationType(juce::Justification::centred);
        sep->setBorderSize(juce::BorderSize<int>(0));
        sep->setMinimumHorizontalScale(1.0f);
    }

    struct PositionField
    {
        WheelLabel& label;
        int maxLength;
        PositionUnit unit;
    };
    for (auto field : {PositionField{positionBarLabel, 3, PositionUnit::Bar},
                       PositionField{positionBeatLabel, 2, PositionUnit::Beat},
                       PositionField{positionTickLabel, 4, PositionUnit::Tick}})
    {
        auto& label = field.label;
        addAndMakeVisible(label);
        label.setFont(font::mono(font::sizeDisplay).boldened());
        label.setColour(juce::Label::textColourId, text::t1);
        label.setJustificationType(juce::Justification::centred);
        label.setBorderSize(juce::BorderSize<int>(0));
        label.setMinimumHorizontalScale(1.0f);
        label.setEditable(true);
        int maxLength = field.maxLength;
        label.onEditorShow = [&label, maxLength]()
        {
            if (auto* editor = label.getCurrentTextEditor())
            {
                editor->setInputRestrictions(maxLength, "0123456789");
                editor->setJustification(juce::Justification::centred);
                editor->selectAll();
            }
        };
        label.onTextChange = [this]() { commitPositionEdit(); };
        PositionUnit unit = field.unit;
        label.onWheel = [this, unit](int direction) { nudgePosition(unit, direction); };
    }

    addAndMakeVisible(tempoValueLabel);
    tempoValueLabel.setFont(font::mono(font::sizeDisplay).boldened());
    tempoValueLabel.setColour(juce::Label::textColourId, text::t1);
    tempoValueLabel.setJustificationType(juce::Justification::centred);
    tempoValueLabel.setBorderSize(juce::BorderSize<int>(0));
    tempoValueLabel.setMinimumHorizontalScale(1.0f);
    tempoValueLabel.setEditable(true);
    tempoValueLabel.onEditorShow = [this]()
    {
        if (auto* editor = tempoValueLabel.getCurrentTextEditor())
        {
            editor->setInputRestrictions(7, "0123456789.");
            editor->setJustification(juce::Justification::centred);
            editor->selectAll();
        }
    };
    tempoValueLabel.onTextChange = [this]() { commitTempoEdit(); };
    tempoValueLabel.onWheel = [this](int direction) { nudgeTempo(direction); };

    addAndMakeVisible(keyValueLabel);
    keyValueLabel.setFont(font::mono(font::sizeDisplay).boldened());
    keyValueLabel.setColour(juce::Label::textColourId, text::t1);
    keyValueLabel.setJustificationType(juce::Justification::centred);
    keyValueLabel.setBorderSize(juce::BorderSize<int>(0));
    keyValueLabel.setMinimumHorizontalScale(1.0f);
    keyValueLabel.setEditable(true);
    keyValueLabel.onEditorShow = [this]()
    {
        if (auto* editor = keyValueLabel.getCurrentTextEditor())
        {
            editor->setInputRestrictions(3, "ABCDEFGabcdefg#m");
            editor->setJustification(juce::Justification::centred);
            editor->selectAll();
        }
    };
    keyValueLabel.onTextChange = [this]() { commitKeySignatureEdit(); };
    keyValueLabel.onWheel = [this](int direction) { nudgeKeySignature(direction); };

    struct TimeSigField
    {
        WheelLabel* label;
        int maxLength;
        juce::Justification justification;
        TimeSigUnit unit;
    };
    for (auto field : {TimeSigField{&timeSigNumLabel, 2, juce::Justification::centredRight, TimeSigUnit::Numerator},
                       TimeSigField{&timeSigDenLabel, 2, juce::Justification::centredLeft, TimeSigUnit::Denominator}})
    {
        auto* label = field.label;
        int maxLength = field.maxLength;
        auto justification = field.justification;
        addAndMakeVisible(label);
        label->setFont(font::mono(font::sizeDisplay).boldened());
        label->setColour(juce::Label::textColourId, text::t1);
        label->setJustificationType(justification);
        label->setBorderSize(juce::BorderSize<int>(0));
        label->setMinimumHorizontalScale(1.0f);
        label->setEditable(true);
        label->onEditorShow = [label, maxLength, justification]()
        {
            if (auto* editor = label->getCurrentTextEditor())
            {
                editor->setInputRestrictions(maxLength, "0123456789");
                editor->setJustification(justification);
                editor->selectAll();
            }
        };
        label->onTextChange = [this]() { commitTimeSignatureEdit(); };
        TimeSigUnit unit = field.unit;
        label->onWheel = [this, unit](int direction) { nudgeTimeSignature(unit, direction); };
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

    trackList.setWantsKeyboardFocus(true);
    pianoRoll.setWantsKeyboardFocus(true);
    eventList.setWantsKeyboardFocus(true);
    controllerLane.setWantsKeyboardFocus(true);
    selectToolButton.setWantsKeyboardFocus(true);
    editToolButton.setWantsKeyboardFocus(true);
    addAndMakeVisible(focusBorder);
    juce::Desktop::getInstance().addFocusChangeListener(this);

    commandManager.registerAllCommandsForTarget(this);
    setApplicationCommandManagerToWatch(&commandManager);
    setWantsKeyboardFocus(true);
    setSize(1280, 800);

    int c4Y = PianoRollComponent::gridTopOffset + (127 - 60) * pianoRoll.noteHeight - getHeight() / 2;
    viewport.setViewPosition(0, c4Y);
}

MainComponent::~MainComponent()
{
    juce::Desktop::getInstance().removeFocusChangeListener(this);
    knownPluginList.removeChangeListener(this);
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
    if (source == &knownPluginList)
    {
        if (auto xml = knownPluginList.createXml())
            getAppProperties().getUserSettings()->setValue("knownPluginList", xml.get());
    }
    else if (source == &audioDeviceManager)
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

juce::StringArray MainComponent::getMenuBarNames()
{
    return {"File", "Edit", "View", "Plugins", "Settings"};
}

juce::PopupMenu MainComponent::getMenuForIndex(int menuIndex, const juce::String&)
{
    juce::PopupMenu menu;
    if (menuIndex == 0)
    {
        menu.addCommandItem(&commandManager, CommandID::newFile_);
        menu.addCommandItem(&commandManager, CommandID::openFile);
        menu.addCommandItem(&commandManager, CommandID::saveFile_);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandID::quitApp);
    }
    else if (menuIndex == 1)
    {
        menu.addCommandItem(&commandManager, CommandID::undoAction);
        menu.addCommandItem(&commandManager, CommandID::redoAction);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandID::cutAction);
        menu.addCommandItem(&commandManager, CommandID::copyAction);
        menu.addCommandItem(&commandManager, CommandID::pasteAction);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandID::selectAllAction);
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

        pluginMenuSnapshot = knownPluginList.getTypes();
        juce::PopupMenu scannedSubmenu;
        juce::KnownPluginList::addToMenu(scannedSubmenu, pluginMenuSnapshot, juce::KnownPluginList::sortByManufacturer);
        menu.addSubMenu("Load Scanned Plugin", scannedSubmenu, !pluginMenuSnapshot.isEmpty());

        menu.addSeparator();

        juce::PopupMenu::Item manageItem;
        manageItem.itemID = CommandID::managePlugins_;
        manageItem.text = "Manage Plugins...";
        manageItem.action = [this]() { managePlugins(); };
        menu.addItem(manageItem);
    }
    else if (menuIndex == 4)
    {
        juce::PopupMenu::Item audioSettingsItem;
        audioSettingsItem.itemID = CommandID::audioSettings_;
        audioSettingsItem.text = "Audio Settings...";
        audioSettingsItem.action = [this]() { showAudioSettings(); };
        menu.addItem(audioSettingsItem);

        menu.addSeparator();

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

void MainComponent::menuItemSelected(int menuItemID, int)
{
    int index = juce::KnownPluginList::getIndexChosenByMenu(pluginMenuSnapshot, menuItemID);
    if (index < 0)
        return;

    if (pluginHost.loadPlugin(pluginMenuSnapshot.getReference(index)))
    {
        auto& track = sequence.getTrack(0);
        track.setRouteTargetTrackIndex(-1);
        track.setOutputDestination(MidiTrack::OutputDestination::Plugin);
    }
}

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
                       CommandID::pasteAction,       CommandID::selectAllAction,
                       CommandID::moveNotesUp,       CommandID::moveNotesDown,
                       CommandID::moveSelectionPrev, CommandID::moveSelectionNext,
                       CommandID::scrollViewUp,      CommandID::scrollViewDown,
                       CommandID::scrollViewLeft,    CommandID::scrollViewRight,
                       CommandID::zoomInHorizontal,  CommandID::zoomOutHorizontal,
                       CommandID::zoomInVertical,    CommandID::zoomOutVertical,
                       CommandID::zoomReset,         CommandID::toggleLoop});
}

void MainComponent::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    switch (commandID)
    {
    case CommandID::newFile_:
        result.setInfo("New", "", "File", 0);
        result.addDefaultKeypress('N', juce::ModifierKeys::commandModifier);
        break;
    case CommandID::openFile:
        result.setInfo("Open...", "", "File", 0);
        result.addDefaultKeypress('O', juce::ModifierKeys::commandModifier);
        break;
    case CommandID::saveFile_:
        result.setInfo("Save...", "", "File", 0);
        result.addDefaultKeypress('S', juce::ModifierKeys::commandModifier);
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
        result.addDefaultKeypress(',', juce::ModifierKeys::commandModifier);
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
        result.addDefaultKeypress('Z', juce::ModifierKeys::commandModifier);
        result.setActive(undoManager.canUndo());
        break;
    case CommandID::redoAction:
        result.setInfo("Redo", "", "Edit", 0);
        result.addDefaultKeypress('Y', juce::ModifierKeys::commandModifier);
        result.setActive(undoManager.canRedo());
        break;
    case CommandID::cutAction:
        result.setInfo("Cut", "", "Edit", 0);
        result.addDefaultKeypress('X', juce::ModifierKeys::commandModifier);
        result.setActive(pianoRoll.hasSelectedNotes());
        break;
    case CommandID::copyAction:
        result.setInfo("Copy", "", "Edit", 0);
        result.addDefaultKeypress('C', juce::ModifierKeys::commandModifier);
        result.setActive(pianoRoll.hasSelectedNotes());
        break;
    case CommandID::pasteAction:
        result.setInfo("Paste", "", "Edit", 0);
        result.addDefaultKeypress('V', juce::ModifierKeys::commandModifier);
        result.setActive(pianoRoll.hasClipboardNotes());
        break;
    case CommandID::selectAllAction:
        result.setInfo("Select All", "", "Edit", 0);
        result.addDefaultKeypress('A', juce::ModifierKeys::commandModifier);
        result.setActive(focusedPanel == FocusPanel::PianoRoll && pianoRoll.hasNotesInActiveTrack());
        break;
    case CommandID::moveNotesUp:
        result.setInfo("Move Up", "", "Edit", 0);
        result.addDefaultKeypress(juce::KeyPress::upKey, 0);
        result.setActive(focusedPanel == FocusPanel::PianoRoll && pianoRoll.hasSelectedNotes());
        break;
    case CommandID::moveNotesDown:
        result.setInfo("Move Down", "", "Edit", 0);
        result.addDefaultKeypress(juce::KeyPress::downKey, 0);
        result.setActive(focusedPanel == FocusPanel::PianoRoll && pianoRoll.hasSelectedNotes());
        break;
    case CommandID::moveSelectionPrev:
        result.setInfo("Select Previous Note", "", "Edit", 0);
        result.addDefaultKeypress(juce::KeyPress::leftKey, 0);
        result.setActive(focusedPanel == FocusPanel::PianoRoll && pianoRoll.hasNotesInActiveTrack());
        break;
    case CommandID::moveSelectionNext:
        result.setInfo("Select Next Note", "", "Edit", 0);
        result.addDefaultKeypress(juce::KeyPress::rightKey, 0);
        result.setActive(focusedPanel == FocusPanel::PianoRoll && pianoRoll.hasNotesInActiveTrack());
        break;
    case CommandID::scrollViewUp:
        result.setInfo("Scroll Up", "", "View", 0);
        result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::commandModifier);
        result.setActive(focusedPanel == FocusPanel::PianoRoll);
        break;
    case CommandID::scrollViewDown:
        result.setInfo("Scroll Down", "", "View", 0);
        result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::commandModifier);
        result.setActive(focusedPanel == FocusPanel::PianoRoll);
        break;
    case CommandID::scrollViewLeft:
        result.setInfo("Scroll Left", "", "View", 0);
        result.addDefaultKeypress(juce::KeyPress::leftKey, juce::ModifierKeys::commandModifier);
        result.setActive(focusedPanel == FocusPanel::PianoRoll);
        break;
    case CommandID::scrollViewRight:
        result.setInfo("Scroll Right", "", "View", 0);
        result.addDefaultKeypress(juce::KeyPress::rightKey, juce::ModifierKeys::commandModifier);
        result.setActive(focusedPanel == FocusPanel::PianoRoll);
        break;
    case CommandID::zoomInHorizontal:
        result.setInfo("Zoom In (Horizontal)", "", "View", 0);
        result.addDefaultKeypress('=', juce::ModifierKeys::commandModifier);
        break;
    case CommandID::zoomOutHorizontal:
        result.setInfo("Zoom Out (Horizontal)", "", "View", 0);
        result.addDefaultKeypress('-', juce::ModifierKeys::commandModifier);
        break;
    case CommandID::zoomInVertical:
        result.setInfo("Zoom In (Vertical)", "", "View", 0);
        result.addDefaultKeypress('=', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
        break;
    case CommandID::zoomOutVertical:
        result.setInfo("Zoom Out (Vertical)", "", "View", 0);
        result.addDefaultKeypress('-', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
        break;
    case CommandID::zoomReset:
        result.setInfo("Reset Zoom", "", "View", 0);
        result.addDefaultKeypress('0', juce::ModifierKeys::commandModifier);
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
        jumpToTick(sequence.barStartToTick(targetBar));
        return true;
    }
    case CommandID::nextBar:
    {
        int currentTick = static_cast<int>(playbackEngine.getCurrentTick());
        auto bbt = sequence.tickToBarBeatTick(currentTick);
        jumpToTick(sequence.barStartToTick(bbt.bar + 1));
        return true;
    }
    case CommandID::switchToEditTool:
        setActiveTool(PianoRollComponent::EditMode::Edit);
        return true;
    case CommandID::switchToSelectTool:
        setActiveTool(PianoRollComponent::EditMode::Select);
        return true;
    case CommandID::undoAction:
    {
        const bool structural = (undoManager.getUndoDescription() == juce::String(kStructuralTxn));
        bool wasRunning = false;
        if (structural)
            wasRunning = playbackEngine.suspendForStructuralChange();
        undoManager.undo();
        if (structural)
            playbackEngine.resumeAfterStructuralChange(wasRunning);
        else
            playbackEngine.rebuildSnapshot();
        refreshAllViews();
        return true;
    }
    case CommandID::redoAction:
    {
        const bool structural = (undoManager.getRedoDescription() == juce::String(kStructuralTxn));
        bool wasRunning = false;
        if (structural)
            wasRunning = playbackEngine.suspendForStructuralChange();
        undoManager.redo();
        if (structural)
            playbackEngine.resumeAfterStructuralChange(wasRunning);
        else
            playbackEngine.rebuildSnapshot();
        refreshAllViews();
        return true;
    }
    case CommandID::cutAction:
        pianoRoll.cutSelectedNotes();
        return true;
    case CommandID::copyAction:
        pianoRoll.copySelectedNotes();
        return true;
    case CommandID::pasteAction:
        pianoRoll.pasteNotes(static_cast<int>(playbackEngine.getCurrentTick()));
        return true;
    case CommandID::selectAllAction:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRoll.selectAllNotes();
        return true;
    case CommandID::moveNotesUp:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRoll.nudgeSelectedNotesPitch(1);
        return true;
    case CommandID::moveNotesDown:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRoll.nudgeSelectedNotesPitch(-1);
        return true;
    case CommandID::moveSelectionPrev:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRoll.moveSelectionToAdjacentNote(-1);
        return true;
    case CommandID::moveSelectionNext:
        if (focusedPanel == FocusPanel::PianoRoll)
            pianoRoll.moveSelectionToAdjacentNote(1);
        return true;
    case CommandID::scrollViewUp:
        if (focusedPanel == FocusPanel::PianoRoll)
            scrollViewVertically(-PianoRollComponent::defaultNoteHeight);
        return true;
    case CommandID::scrollViewDown:
        if (focusedPanel == FocusPanel::PianoRoll)
            scrollViewVertically(PianoRollComponent::defaultNoteHeight);
        return true;
    case CommandID::scrollViewLeft:
        if (focusedPanel == FocusPanel::PianoRoll)
            scrollViewHorizontally(-PianoRollComponent::defaultNoteHeight);
        return true;
    case CommandID::scrollViewRight:
        if (focusedPanel == FocusPanel::PianoRoll)
            scrollViewHorizontally(PianoRollComponent::defaultNoteHeight);
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
    using namespace calliope::theme;
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    int transportBarTop = getHeight() - transportBarHeight;
    g.setColour(surface::bg2);
    g.fillRect(0, transportBarTop, getWidth(), transportBarHeight);
    g.setColour(border::strong);
    g.drawHorizontalLine(transportBarTop, 0.0f, static_cast<float>(getWidth()));

    auto drawInfoBox = [&](juce::Rectangle<int> b)
    {
        if (b.isEmpty())
            return;
        g.setColour(surface::surface);
        g.fillRoundedRectangle(b.toFloat(), radius::r2);
        g.setColour(border::normal);
        g.drawRoundedRectangle(b.toFloat().reduced(0.5f), radius::r2, 1.0f);
    };
    drawInfoBox(positionBoxBounds);
    drawInfoBox(infoBoxBounds);

    if (!infoBoxBounds.isEmpty())
    {
        g.setColour(border::soft);
        auto top = static_cast<float>(infoBoxBounds.getY()) + 1.0f;
        auto bottom = static_cast<float>(infoBoxBounds.getBottom()) - 1.0f;
        g.drawVerticalLine(infoDividerX1, top, bottom);
        g.drawVerticalLine(infoDividerX2, top, bottom);
    }

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
        int numTracks = sequence.getNumTracks();
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
    auto transportArea = area.removeFromBottom(transportBarHeight);
    auto toolbar = transportArea;

    const int posW = 176;
    const int btnW = 172;
    const int tsW = 68;
    const int keyW = 60;
    const int tempoW = 96;
    const int infoW = tsW + keyW + tempoW;
    const int g1 = 20, g2 = 20;
    const int contentWidth = posW + g1 + btnW + g2 + infoW;

    auto content = toolbar.withSizeKeepingCentre(contentWidth, transportBarHeight);

    const int boxH = 44;
    const int boxPadX = 8;
    const int boxPadTop = 5;
    const int headerH = 13;

    auto layoutSegment = [&](juce::Rectangle<int> segment, juce::Label& header, juce::Label& value)
    {
        auto inner = segment.reduced(boxPadX, 0);
        inner.removeFromTop(boxPadTop);
        header.setBounds(inner.removeFromTop(headerH));
        value.setBounds(inner.removeFromTop(boxH - boxPadTop - headerH));
    };

    auto posBox = content.removeFromLeft(posW).withSizeKeepingCentre(posW, boxH);
    positionBoxBounds = posBox;
    {
        auto inner = posBox.reduced(boxPadX, 0);
        inner.removeFromTop(boxPadTop);
        positionHeaderLabel.setBounds(inner.removeFromTop(headerH));
        auto valueRow = inner.removeFromTop(boxH - boxPadTop - headerH);

        using namespace calliope::theme;
        auto valueFont = font::mono(font::sizeDisplay).boldened();
        auto widthOf = [&](const char* s) { return juce::GlyphArrangement::getStringWidthInt(valueFont, s); };

        const int pad = 4;
        int barW = widthOf("000") + pad;
        int beatW = widthOf("00") + pad;
        int tickW = widthOf("0000") + pad;
        int dotW = widthOf(".");

        int groupW = barW + dotW + beatW + dotW + tickW;
        auto group = valueRow.withSizeKeepingCentre(groupW, valueRow.getHeight());
        positionBarLabel.setBounds(group.removeFromLeft(barW));
        positionDot1.setBounds(group.removeFromLeft(dotW));
        positionBeatLabel.setBounds(group.removeFromLeft(beatW));
        positionDot2.setBounds(group.removeFromLeft(dotW));
        positionTickLabel.setBounds(group.removeFromLeft(tickW));
    }
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

    auto infoBox = content.removeFromLeft(infoW).withSizeKeepingCentre(infoW, boxH);
    infoBoxBounds = infoBox;
    auto timeSeg = infoBox.removeFromLeft(tsW);
    auto keySeg = infoBox.removeFromLeft(keyW);
    auto tempoSeg = infoBox;
    infoDividerX1 = timeSeg.getRight();
    infoDividerX2 = keySeg.getRight();
    {
        auto inner = timeSeg.reduced(boxPadX, 0);
        inner.removeFromTop(boxPadTop);
        timeSigHeaderLabel.setBounds(inner.removeFromTop(headerH));
        auto valueRow = inner.removeFromTop(boxH - boxPadTop - headerH);

        using namespace calliope::theme;
        auto valueFont = font::mono(font::sizeDisplay).boldened();
        auto widthOf = [&](const char* s) { return juce::GlyphArrangement::getStringWidthInt(valueFont, s); };

        const int pad = 4;
        int numW = widthOf("00") + pad;
        int denW = widthOf("00") + pad;
        int slashW = widthOf("/");

        int groupW = numW + slashW + denW;
        auto group = valueRow.withSizeKeepingCentre(groupW, valueRow.getHeight());
        timeSigNumLabel.setBounds(group.removeFromLeft(numW));
        timeSigSlashLabel.setBounds(group.removeFromLeft(slashW));
        timeSigDenLabel.setBounds(group.removeFromLeft(denW));
    }
    layoutSegment(keySeg, keyHeaderLabel, keyValueLabel);
    layoutSegment(tempoSeg, tempoHeaderLabel, tempoValueLabel);

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
    updateTransportDisplay();
    scrollToPlayhead(static_cast<int>(tick));
}

void MainComponent::jumpToTick(int tick)
{
    playbackEngine.setPositionInTicks(tick);
    pianoRoll.setPlayheadTick(tick);
    controllerLane.setPlayheadTick(tick);
    eventList.setPlayheadTick(tick);
    updateTransportDisplay();
    scrollToPlayhead(tick);
}

void MainComponent::commitPositionEdit()
{
    int currentTick = static_cast<int>(playbackEngine.getCurrentTick());
    auto current = sequence.tickToBarBeatTick(currentTick);

    int bar = positionBarLabel.getText().isEmpty() ? current.bar : positionBarLabel.getText().getIntValue();
    int beat = positionBeatLabel.getText().isEmpty() ? current.beat : positionBeatLabel.getText().getIntValue();
    int tickInBeat = positionTickLabel.getText().isEmpty() ? current.tick : positionTickLabel.getText().getIntValue();

    jumpToTick(sequence.barBeatTickToTick(bar, beat, tickInBeat));
}

void MainComponent::nudgePosition(PositionUnit unit, int direction)
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());
    auto ts = sequence.getTimeSignatureAt(tick);
    int ticksPerBeat = sequence.getTicksPerQuarterNote() * 4 / ts.denominator;

    int step = 1;
    switch (unit)
    {
    case PositionUnit::Bar:
        step = ticksPerBeat * ts.numerator;
        break;
    case PositionUnit::Beat:
        step = ticksPerBeat;
        break;
    case PositionUnit::Tick:
        step = 1;
        break;
    }

    jumpToTick(juce::jmax(0, tick + direction * step));
}

void MainComponent::commitTempoEdit()
{
    double bpm = tempoValueLabel.getText().getDoubleValue();
    if (bpm > 0.0)
        setTempoAtPlayhead(bpm);
    else
        updateTransportDisplay();
}

void MainComponent::nudgeTempo(int direction)
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());
    auto tc = sequence.getTempoChangeAt(tick);

    setTempoAtPlayhead(juce::roundToInt(tc.bpm) + direction);
}

void MainComponent::setTempoAtPlayhead(double bpm)
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());
    auto tc = sequence.getTempoChangeAt(tick);

    double clamped = juce::jlimit(MidiSequence::minBpm, MidiSequence::maxBpm, bpm);
    if (clamped == tc.bpm)
    {
        updateTransportDisplay();
        return;
    }

    undoManager.beginNewTransaction();
    undoManager.perform(new TempoChangeAction(&sequence, tc.tick, clamped));

    updateTransportDisplay();
    pianoRoll.repaint();
    controllerLane.repaint();
    eventList.refresh();
}

void MainComponent::commitTimeSignatureEdit()
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());
    auto ts = sequence.getTimeSignatureAt(tick);

    int num = timeSigNumLabel.getText().isEmpty() ? ts.numerator : timeSigNumLabel.getText().getIntValue();
    int den = timeSigDenLabel.getText().isEmpty() ? ts.denominator : timeSigDenLabel.getText().getIntValue();

    setTimeSignatureAtPlayhead(num, den);
}

void MainComponent::nudgeTimeSignature(TimeSigUnit unit, int direction)
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());
    auto ts = sequence.getTimeSignatureAt(tick);

    if (unit == TimeSigUnit::Numerator)
        setTimeSignatureAtPlayhead(ts.numerator + direction, ts.denominator);
    else
        setTimeSignatureAtPlayhead(ts.numerator, direction > 0 ? ts.denominator * 2 : ts.denominator / 2);
}

void MainComponent::setTimeSignatureAtPlayhead(int num, int den)
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());
    auto ts = sequence.getTimeSignatureAt(tick);

    auto snapToPowerOfTwo = [](int value)
    {
        value = juce::jlimit(2, 64, value);
        int lower = 1;
        while (lower * 2 <= value)
            lower *= 2;
        int upper = juce::jmin(64, lower * 2);
        return (value - lower <= upper - value) ? lower : upper;
    };

    num = juce::jlimit(1, 64, num);
    den = snapToPowerOfTwo(den);

    if (num == ts.numerator && den == ts.denominator)
    {
        updateTransportDisplay();
        return;
    }

    undoManager.beginNewTransaction();
    undoManager.perform(new TimeSignatureChangeAction(&sequence, ts.tick, num, den));

    updateTransportDisplay();
    pianoRoll.repaint();
    controllerLane.repaint();
    eventList.refresh();
}

void MainComponent::commitKeySignatureEdit()
{
    int sharpsOrFlats = 0;
    bool isMinor = false;
    if (MidiSequence::keySignatureFromString(keyValueLabel.getText().toStdString(), sharpsOrFlats, isMinor))
        setKeySignatureAtPlayhead(sharpsOrFlats, isMinor);
    else
        updateTransportDisplay();
}

void MainComponent::nudgeKeySignature(int direction)
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());
    auto ks = sequence.getKeySignatureAt(tick);

    int sf = juce::jlimit(-6, 6, ks.sharpsOrFlats);
    int index = juce::jlimit(0, 25, (sf + 6) + (ks.isMinor ? 13 : 0) + direction);

    bool isMinor = index >= 13;
    setKeySignatureAtPlayhead((isMinor ? index - 13 : index) - 6, isMinor);
}

void MainComponent::setKeySignatureAtPlayhead(int sharpsOrFlats, bool isMinor)
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());
    auto ks = sequence.getKeySignatureAt(tick);

    bool hasActiveKey = false;
    for (const auto& k : sequence.getKeySignatureChanges())
        if (k.tick <= tick)
        {
            hasActiveKey = true;
            break;
        }

    sharpsOrFlats = MidiSequence::normalizeSharpsOrFlats(sharpsOrFlats);
    if (hasActiveKey && sharpsOrFlats == ks.sharpsOrFlats && isMinor == ks.isMinor)
    {
        updateTransportDisplay();
        return;
    }

    undoManager.beginNewTransaction();
    undoManager.perform(new KeySignatureChangeAction(&sequence, ks.tick, sharpsOrFlats, isMinor));

    updateTransportDisplay();
    pianoRoll.repaint();
    controllerLane.repaint();
    eventList.refresh();
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

void MainComponent::updateTransportDisplay()
{
    int tick = static_cast<int>(playbackEngine.getCurrentTick());

    auto bbt = sequence.tickToBarBeatTick(tick);
    if (positionBarLabel.getCurrentTextEditor() == nullptr)
        positionBarLabel.setText(juce::String(bbt.bar).paddedLeft('0', 3), juce::dontSendNotification);
    if (positionBeatLabel.getCurrentTextEditor() == nullptr)
        positionBeatLabel.setText(juce::String(bbt.beat).paddedLeft('0', 2), juce::dontSendNotification);
    if (positionTickLabel.getCurrentTextEditor() == nullptr)
        positionTickLabel.setText(juce::String(bbt.tick).paddedLeft('0', 4), juce::dontSendNotification);

    auto ts = sequence.getTimeSignatureAt(tick);
    if (timeSigNumLabel.getCurrentTextEditor() == nullptr)
        timeSigNumLabel.setText(juce::String(ts.numerator), juce::dontSendNotification);
    if (timeSigDenLabel.getCurrentTextEditor() == nullptr)
        timeSigDenLabel.setText(juce::String(ts.denominator), juce::dontSendNotification);

    if (keyValueLabel.getCurrentTextEditor() == nullptr)
    {
        if (sequence.getKeySignatureChanges().empty())
            keyValueLabel.setText("-", juce::dontSendNotification);
        else
        {
            auto ks = sequence.getKeySignatureAt(tick);
            keyValueLabel.setText(MidiSequence::keySignatureToString(ks.sharpsOrFlats, ks.isMinor),
                                  juce::dontSendNotification);
        }
    }

    if (tempoValueLabel.getCurrentTextEditor() == nullptr)
    {
        double tempo = sequence.getTempoAt(tick);
        tempoValueLabel.setText(juce::String(tempo, 2), juce::dontSendNotification);
    }
}

void MainComponent::refreshAllViews()
{
    pianoRoll.setSelectedNotes({});
    pianoRoll.repaint();
    controllerLane.repaint();
    trackList.refresh();
    eventList.refresh();
    updateTransportDisplay();
    repaint(trackListHeaderBounds);
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
                                 pluginHost.detachAllPlugins();
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
                                 if (playbackEngine.isPlaying())
                                     stopPlayback();
                                 if (pluginHost.loadPlugin(file))
                                 {
                                     auto& track = sequence.getTrack(0);
                                     track.setRouteTargetTrackIndex(-1);
                                     track.setOutputDestination(MidiTrack::OutputDestination::Plugin);
                                     playbackEngine.rebuildSnapshot();
                                 }
                             });
}

void MainComponent::managePlugins()
{
    auto* listComp =
        new juce::PluginListComponent(pluginHost.getFormatManager(), knownPluginList, juce::File{}, nullptr);
    listComp->setSize(800, 600);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(listComp);
    options.dialogTitle = "Manage Plugins";
    options.dialogBackgroundColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.launchAsync();
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
    playButton.setActive(false);
    vblankAttachment.reset();
}

PlaybackTrackContext MainComponent::makeTrackContext(int trackIndex) const
{
    PlaybackTrackContext ctx;
    ctx.trackIndex = trackIndex;
    if (trackIndex < 0 || trackIndex >= sequence.getNumTracks())
        return ctx;
    const auto& track = sequence.getTrack(trackIndex);
    ctx.channel = track.getChannel();
    ctx.destination = track.getOutputDestination();
    const int rt = track.getRouteTargetTrackIndex();
    ctx.routeTarget = (rt >= 0 && rt < sequence.getNumTracks()) ? rt : trackIndex;
    return ctx;
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
    controllerLane.setPlayheadTick(0);

    eventList.setSequence(&sequence);
    eventList.setSelectedTracks(allTracks);
    eventList.setPlayheadTick(0);

    int c4Y = PianoRollComponent::gridTopOffset + (127 - 60) * pianoRoll.noteHeight - getHeight() / 2;
    viewport.setViewPosition(0, c4Y);
    repaint(trackListHeaderBounds);

    playbackEngine.rebuildSnapshot();
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
