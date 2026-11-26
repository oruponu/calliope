#include "ui/pianoroll/PianoRollPanel.h"
#include "ui/widgets/PassFocusUpTraverser.h"
#include <algorithm>
#include <cmath>

PianoRollPanel::PianoRollPanel(UndoHistory& historyRef)
    : pianoRoll(historyRef, clipboard), controllerLane(historyRef, clipboard)
{
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

    controllerLane.setSelectedTracks(0, {0});
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
        int maxH = getHeight() - PianoRollToolBar::height - 150;
        controllerLaneHeight = juce::jlimit(minH, maxH, controllerLaneHeightOnDragStart - deltaY);
        resized();
    };

    addAndMakeVisible(toolBar);
    toolBar.onToolClicked = [this](auto mode) { setActiveTool(mode); };
    toolBar.onSnapChanged = [this](bool enabled)
    {
        pianoRoll.setSnapEnabled(enabled);
        controllerLane.setSnapEnabled(enabled);
    };
    toolBar.onGridDivisionChanged = [this](GridDivision division)
    {
        pianoRoll.setGridDivision(division);
        controllerLane.setGridDivision(division);
        onGridDivisionChanged(division);
    };
    toolBar.onBackgroundClicked = [this]() { pianoRoll.grabKeyboardFocus(); };
    const bool snapEnabled = toolBar.isSnapEnabled();
    pianoRoll.setSnapEnabled(snapEnabled);
    controllerLane.setSnapEnabled(snapEnabled);

    pianoRoll.setWantsKeyboardFocus(true);
    controllerLane.setWantsKeyboardFocus(true);
}

void PianoRollPanel::setSequence(MidiSequence* sequence)
{
    pianoRoll.setSequence(sequence);
    controllerLane.setSequence(sequence);
    controllerLane.setContentBeats(pianoRoll.getContentBeats());
}

void PianoRollPanel::setSelectedTracks(int activeIndex, const std::set<int>& selected)
{
    pianoRoll.setSelectedTracks(activeIndex, selected);
    controllerLane.setSelectedTracks(activeIndex, selected);
}

void PianoRollPanel::setPlayheadTick(double tick)
{
    pianoRoll.setPlayheadTick(tick);
    controllerLane.setPlayheadTick(tick);
}

void PianoRollPanel::setLoopRegion(bool enabled, int startTick, int endTick)
{
    pianoRoll.setLoopRegion(enabled, startTick, endTick);
    controllerLane.setLoopRegion(enabled, startTick, endTick);
}

void PianoRollPanel::scrollToMiddleC(int halfHeight)
{
    int c4Y = PianoRollComponent::gridTopOffset + (127 - 60) * pianoRoll.noteHeight - halfHeight;
    viewport.setViewPosition(0, c4Y);
}

void PianoRollPanel::scrollToPlayhead(int tick)
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

void PianoRollPanel::scrollToStart()
{
    viewport.setViewPosition(0, viewport.getViewPositionY());
}

GridDivision PianoRollPanel::getGridDivision() const
{
    return toolBar.getGridDivision();
}

void PianoRollPanel::setActiveTool(PianoRollComponent::EditMode mode)
{
    toolBar.setActiveTool(mode);
    pianoRoll.setEditMode(mode);
    controllerLane.setEditToolSelected(mode == PianoRollComponent::EditMode::Edit);
}

void PianoRollPanel::toggleSnap()
{
    toolBar.toggleSnap();
}

bool PianoRollPanel::canCutOrCopy() const
{
    return pianoRoll.hasSelection() || controllerLane.hasPointSelection();
}

bool PianoRollPanel::canPaste() const
{
    return clipboard.hasControllerPoints() ? controllerLane.canPastePoints() : pianoRoll.hasClipboardContent();
}

bool PianoRollPanel::canSelectAll() const
{
    return selectAllTakesControllerPoints() ? controllerLane.canSelectAllPoints() : pianoRoll.hasNotesInActiveTrack();
}

bool PianoRollPanel::hasSelectedNotes() const
{
    return pianoRoll.hasSelectedNotes();
}

bool PianoRollPanel::hasNotesInActiveTrack() const
{
    return pianoRoll.hasNotesInActiveTrack();
}

void PianoRollPanel::cut()
{
    if (controllerLane.hasPointSelection())
        controllerLane.cutSelectedPoints();
    else
        pianoRoll.cutSelection();
}

void PianoRollPanel::copy()
{
    if (controllerLane.hasPointSelection())
        controllerLane.copySelectedPoints();
    else
        pianoRoll.copySelection();
}

void PianoRollPanel::paste(int tick)
{
    if (clipboard.hasControllerPoints())
        controllerLane.pastePoints(tick);
    else
        pianoRoll.paste(tick);
}

void PianoRollPanel::selectAll()
{
    if (selectAllTakesControllerPoints())
        controllerLane.selectAllPoints();
    else
        pianoRoll.selectAllNotes();
}

void PianoRollPanel::nudgeSelectedNotesPitch(int semitones)
{
    pianoRoll.nudgeSelectedNotesPitch(semitones);
}

void PianoRollPanel::moveSelectionToAdjacentNote(int direction)
{
    pianoRoll.moveSelectionToAdjacentNote(direction);
}

void PianoRollPanel::quantizeSelectedNotes()
{
    pianoRoll.quantizeSelectedNotes();
}

void PianoRollPanel::deselectNotes()
{
    pianoRoll.setSelectedNotes({});
}

void PianoRollPanel::clearSelections()
{
    pianoRoll.clearSelection();
    controllerLane.clearPointSelection();
}

void PianoRollPanel::scrollViewUp()
{
    scrollViewVertically(-PianoRollComponent::defaultNoteHeight);
}

void PianoRollPanel::scrollViewDown()
{
    scrollViewVertically(PianoRollComponent::defaultNoteHeight);
}

void PianoRollPanel::scrollViewLeft()
{
    scrollViewHorizontally(-PianoRollComponent::defaultBeatWidth);
}

void PianoRollPanel::scrollViewRight()
{
    scrollViewHorizontally(PianoRollComponent::defaultBeatWidth);
}

void PianoRollPanel::zoomInHorizontally()
{
    zoomHorizontal(1.15f, viewport.getViewWidth() / 2);
}

void PianoRollPanel::zoomOutHorizontally()
{
    zoomHorizontal(1.0f / 1.15f, viewport.getViewWidth() / 2);
}

void PianoRollPanel::zoomInVertically()
{
    zoomVertical(1.15f, viewport.getViewHeight() / 2);
}

void PianoRollPanel::zoomOutVertically()
{
    zoomVertical(1.0f / 1.15f, viewport.getViewHeight() / 2);
}

void PianoRollPanel::resetZoom()
{
    setHorizontalZoom(PianoRollComponent::defaultBeatWidth, viewport.getViewWidth() / 2);
    setVerticalZoom(PianoRollComponent::defaultNoteHeight, viewport.getViewHeight() / 2);
}

bool PianoRollPanel::claimsFocus(const juce::Component* focused)
{
    auto belongsTo = [focused](const juce::Component& panel) { return &panel == focused || panel.isParentOf(focused); };

    if (belongsTo(pianoRoll) || belongsTo(controllerLane) || toolBar.contains(focused))
    {
        controllerLaneFocused = belongsTo(controllerLane);
        return true;
    }
    return false;
}

PianoRollComponent& PianoRollPanel::getPianoRoll()
{
    return pianoRoll;
}

ControllerLaneComponent& PianoRollPanel::getControllerLane()
{
    return controllerLane;
}

void PianoRollPanel::resized()
{
    auto area = getLocalBounds();
    auto toolBarArea = area.removeFromTop(PianoRollToolBar::height);
    toolBar.setBounds(toolBarArea);

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
}

std::unique_ptr<juce::ComponentTraverser> PianoRollPanel::createKeyboardFocusTraverser()
{
    return std::make_unique<PassFocusUpTraverser>();
}

void PianoRollPanel::scrollNoteIntoView(int startTick, int noteNumber)
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

void PianoRollPanel::scrollViewVertically(int deltaY)
{
    int newY = viewport.getViewPositionY() + deltaY;
    newY = juce::jlimit(0, juce::jmax(0, pianoRoll.getHeight() - viewport.getViewHeight()), newY);
    viewport.setViewPosition(viewport.getViewPositionX(), newY);
}

void PianoRollPanel::scrollViewHorizontally(int deltaX)
{
    int newX = viewport.getViewPositionX() + deltaX;
    newX = juce::jlimit(0, juce::jmax(0, pianoRoll.getWidth() - viewport.getViewWidth()), newX);
    viewport.setViewPosition(newX, viewport.getViewPositionY());
}

void PianoRollPanel::setHorizontalZoom(int newBeatWidth, int anchorXInViewport)
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

void PianoRollPanel::setVerticalZoom(int newNoteHeight, int anchorYInViewport)
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

void PianoRollPanel::zoomHorizontal(float factor, int anchorXInViewport)
{
    setHorizontalZoom(juce::roundToInt(pianoRoll.getBeatWidth() * factor), anchorXInViewport);
}

void PianoRollPanel::zoomVertical(float factor, int anchorYInViewport)
{
    setVerticalZoom(juce::roundToInt(pianoRoll.getNoteHeight() * factor), anchorYInViewport);
}

bool PianoRollPanel::selectAllTakesControllerPoints() const
{
    // The velocity view shows notes, so Select All there still selects the piano roll's notes.
    return controllerLaneFocused && controllerLane.getDisplayMode() != ControllerLaneComponent::DisplayMode::Velocity;
}
