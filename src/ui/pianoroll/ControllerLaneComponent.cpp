#include "ui/pianoroll/ControllerLaneComponent.h"
#include "edit/ControllerPointEdits.h"
#include "edit/LayeredNoteEdits.h"
#include "edit/NoteEdit.h"
#include "edit/ProgramChangeEdits.h"
#include "model/ControllerLineValue.h"
#include "model/GmPrograms.h"
#include "model/TimelineGrid.h"
#include "ui/pianoroll/ControllerLaneScale.h"
#include "ui/pianoroll/DragValueTip.h"
#include "ui/pianoroll/DragValueTipPainter.h"
#include "ui/pianoroll/strips/RangeBandPainter.h"
#include "ui/theme/TrackColours.h"
#include "undo/ReplaceEventsAction.h"
#include <algorithm>
#include <cstdlib>
#include <utility>

namespace
{
struct CCEntry
{
    int cc;
    const char* name;
};

const CCEntry commonCCs[] = {
    {1, "CC 1 (Modulation)"}, {7, "CC 7 (Volume)"},   {10, "CC 10 (Pan)"},    {11, "CC 11 (Expression)"},
    {64, "CC 64 (Sustain)"},  {91, "CC 91 (Reverb)"}, {93, "CC 93 (Chorus)"},
};

juce::String ccLabel(int cc)
{
    for (const auto& entry : commonCCs)
        if (entry.cc == cc)
            return entry.name;
    return "CC " + juce::String(cc);
}

// Space between a program change label's edges and its text.
constexpr int programLabelInset = 5;

juce::String programLabel(int program)
{
    juce::String label(program);
    if (const char* name = GmPrograms::programName(program))
        label += " " + juce::String(name);
    return label;
}

int programLabelWidth(int program)
{
    using namespace calliope::theme;
    return juce::GlyphArrangement::getStringWidthInt(font::sans(font::sizeSM), programLabel(program)) +
           2 * programLabelInset;
}
} // namespace

ControllerLaneComponent::~ControllerLaneComponent()
{
    if (sequence != nullptr)
        sequence->removeListener(this);
}

void ControllerLaneComponent::setSequence(MidiSequence* seq)
{
    if (sequence != nullptr)
        sequence->removeListener(this);
    sequence = seq;
    if (sequence != nullptr)
        sequence->addListener(this);

    contentBeats = 16;
    if (sequence && sequence->getNumTracks() > 0)
    {
        int lastTick = 0;
        for (int t = 0; t < sequence->getNumTracks(); ++t)
        {
            const auto& track = sequence->getTrack(t);
            for (int i = 0; i < track.getNumNotes(); ++i)
            {
                int end = track.getNote(i).endTick();
                if (end > lastTick)
                    lastTick = end;
            }
        }
        contentBeats = std::max(contentBeats, lastTick / sequence->getTimeline().getTicksPerQuarterNote() + 4);
    }

    updateSize();
    repaint();
}

void ControllerLaneComponent::setSelectedTracks(int activeIndex, const std::set<int>& selectedIndices)
{
    if (activeIndex != activeTrackIndex)
        resetPointEditing();
    activeTrackIndex = activeIndex;
    selectedTrackIndices = selectedIndices;
    publishDragPreview();
    repaint();
}

void ControllerLaneComponent::setDisplayMode(DisplayMode mode)
{
    resetPointEditing();
    displayMode = mode;
    repaint();
}

void ControllerLaneComponent::setCCNumber(int cc)
{
    resetPointEditing();
    ccNumber = cc;
    repaint();
}

void ControllerLaneComponent::setPlayheadTick(double tick)
{
    int oldX = tickToX(static_cast<int>(playheadTick));
    playheadTick = tick;
    int newX = tickToX(static_cast<int>(playheadTick));

    int margin = 2;
    repaint(oldX - margin, 0, margin * 2 + 2, getHeight());
    repaint(newX - margin, 0, margin * 2 + 2, getHeight());
}

void ControllerLaneComponent::setLoopRegion(bool enabled, int startTick, int endTick)
{
    loopEnabled = enabled;
    loopStartTick = startTick;
    loopEndTick = endTick;
    repaint();
}

void ControllerLaneComponent::setRecordingLayer(const RecordingLayer* layer, RecordingLayerEditor* editor)
{
    recordingLayer = layer;
    recordingEditor = editor;
    repaint();
}

void ControllerLaneComponent::setEditToolSelected(bool editTool)
{
    editToolSelected = editTool;
}

void ControllerLaneComponent::setSnapEnabled(bool enabled)
{
    snapEnabled = enabled;
}

void ControllerLaneComponent::clearPointSelection()
{
    // Selecting notes by their velocity bars makes the piano roll call this; the range selection goes on.
    if (std::holds_alternative<VelocityRangeSelecting>(drag))
        return;
    if (pointSelection.isEmpty() && std::holds_alternative<Idle>(drag))
        return;
    resetPointEditing();
    repaint();
}

void ControllerLaneComponent::recordingLayerChanged()
{
    publishDragPreview();
    repaint();
}

std::vector<std::pair<int, int>>
ControllerLaneComponent::recordedValues(int trackIndex,
                                        const std::function<std::optional<int>(const MidiEvent&)>& valueOf) const
{
    std::vector<std::pair<int, int>> values;
    if (recordingLayer == nullptr)
        return values;
    const auto& take = recordingLayer->getTake();
    const auto recorded = take.find(sequence->getTrack(trackIndex).getId());
    if (recorded == take.end())
        return values;
    for (const auto& event : recorded->second.events)
        if (const auto value = valueOf(event))
            values.push_back({event.tick, *value});
    // Later loop passes record earlier ticks, so the take is not in tick order.
    std::ranges::stable_sort(values, {}, &std::pair<int, int>::first);
    return values;
}

void ControllerLaneComponent::setContentBeats(int beats)
{
    contentBeats = beats;
    updateSize();
}

void ControllerLaneComponent::setBeatWidth(int w)
{
    beatWidth = w;
    updateSize();
    repaint();
}

void ControllerLaneComponent::setRightPadding(int px)
{
    rightPadding = px;
    updateSize();
}

void ControllerLaneComponent::setGridDivision(GridDivision division)
{
    gridDivision = division;
    repaint();
}

void ControllerLaneComponent::updateSize()
{
    if (!sequence)
        return;

    int width = leftPanelWidth + contentBeats * beatWidth + rightPadding;

    if (auto* vp = findParentComponentOfClass<juce::Viewport>())
        width = std::max(width, vp->getMaximumVisibleWidth());

    setSize(width, getHeight());
}

void ControllerLaneComponent::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    g.fillAll(surface::bg);

    if (getDrawAreaHeight() <= 0)
        return;

    drawGrid(g);

    switch (displayMode)
    {
    case DisplayMode::Velocity:
        drawVelocity(g);
        break;
    case DisplayMode::ControlChange:
        drawControlChange(g);
        break;
    case DisplayMode::PitchBend:
        drawPitchBend(g);
        break;
    case DisplayMode::ProgramChange:
        drawProgramChange(g);
        break;
    }

    drawLoopRegion(g);
    drawPlayhead(g);
    if (const auto* selecting = std::get_if<PointRangeSelecting>(&drag))
        RangeBandPainter::draw(g, selecting->gesture, getHeight(), accent::soft, accent::base.withAlpha(0.6f));
    else if (const auto* selectingNotes = std::get_if<VelocityRangeSelecting>(&drag))
        RangeBandPainter::draw(g, selectingNotes->gesture, getHeight(), accent::soft, accent::base.withAlpha(0.6f));
    drawLeftPanel(g);
    drawDragValueTip(g);
}

void ControllerLaneComponent::drawDragValueTip(juce::Graphics& g)
{
    const auto* pointDragging = std::get_if<PointDragging>(&drag);
    const auto kind = editedKind();
    if (pointDragging == nullptr || !pointDragging->tip || !kind || !sequence)
        return;

    const auto& [mouse, anchor] = *pointDragging->tip;
    const int value = kind->valueOf(anchor);
    const int displayed =
        kind->type == MidiEvent::Type::PitchBend ? ControllerLaneScale::displayedPitchBend(value) : value;
    const auto position = DragValueTip::formatPosition(sequence->getTimeline().tickToBarBeatTick(anchor.tick));
    DragValueTipPainter::draw(g, *this, mouse, DragValueTipPainter::visibleArea(*this).withTrimmedLeft(leftPanelWidth),
                              juce::String(position), juce::String(displayed));
}

void ControllerLaneComponent::mouseDown(const juce::MouseEvent& e)
{
    if (!sequence)
        return;
    if (activeTrackIndex < 0 || activeTrackIndex >= sequence->getNumTracks())
        return;
    if (programEditSession.isOpen())
        return;

    const auto kind = editedKind();
    const bool onLabelPanel = isOnLabelPanel(e.x);
    if (e.getNumberOfClicks() == 1)
        doubleClickPoint = e.mods.isPopupMenu() || e.mods.isShiftDown() ? -1 : hitTestPoint(e.x, e.y);
    if (e.mods.isPopupMenu())
    {
        if (const int hit = kind ? hitTestPoint(e.x, e.y) : -1; hit >= 0)
        {
            openValueEditor(hit);
            return;
        }

        juce::PopupMenu menu;
        menu.addItem(1, "Velocity", true, displayMode == DisplayMode::Velocity);

        juce::PopupMenu ccMenu;
        const auto showing = [this](int cc) { return displayMode == DisplayMode::ControlChange && ccNumber == cc; };
        for (const auto& cc : commonCCs)
            ccMenu.addItem(100 + cc.cc, cc.name, true, showing(cc.cc));
        ccMenu.addSeparator();
        constexpr int ccsPerGroup = 16;
        for (int first = 0; first < 128; first += ccsPerGroup)
        {
            const int last = first + ccsPerGroup - 1;
            juce::PopupMenu group;
            for (int cc = first; cc <= last; ++cc)
                group.addItem(100 + cc, ccLabel(cc), true, showing(cc));
            const bool groupShowing =
                displayMode == DisplayMode::ControlChange && ccNumber >= first && ccNumber <= last;
            ccMenu.addSubMenu("CC " + juce::String(first) + "-" + juce::String(last), group, true, nullptr,
                              groupShowing);
        }
        menu.addSubMenu("Control Change", ccMenu);

        menu.addItem(2, "Pitch Bend", true, displayMode == DisplayMode::PitchBend);
        menu.addItem(3, "Program Change", true, displayMode == DisplayMode::ProgramChange);

        auto screenPos = e.getScreenPosition();
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({screenPos.x, screenPos.y, 1, 1}),
                           [this](int result)
                           {
                               if (result == 1)
                                   setDisplayMode(DisplayMode::Velocity);
                               else if (result == 2)
                                   setDisplayMode(DisplayMode::PitchBend);
                               else if (result == 3)
                                   setDisplayMode(DisplayMode::ProgramChange);
                               else if (result >= 100)
                               {
                                   setCCNumber(result - 100);
                                   setDisplayMode(DisplayMode::ControlChange);
                               }
                           });
        return;
    }

    if (onLabelPanel)
        return;

    if (showsProgramChange())
    {
        const auto& events = sequence->getTrack(activeTrackIndex).getEvents();
        const int hit = hitTestLabel(e.x);
        if (hit >= 0)
        {
            const auto block = ProgramChangeEdits::onTick(events, events[static_cast<size_t>(hit)].tick);
            if (e.mods.isShiftDown())
            {
                auto selected = pointSelection.indices();
                const bool allSelected =
                    std::ranges::all_of(block, [&selected](int index) { return selected.contains(index); });
                for (int index : block)
                {
                    if (allSelected)
                        selected.erase(index);
                    else
                        selected.insert(index);
                }
                selectPoints(std::move(selected));
                return;
            }

            const bool hasSelectedBlock =
                std::ranges::any_of(block, [this](int index) { return pointSelection.contains(index); });
            const bool hasSelectionOutsideBlock =
                std::ranges::any_of(pointSelection.indices(), [&block](int index) { return !block.contains(index); });
            auto moved = block;
            if (hasSelectedBlock && hasSelectionOutsideBlock)
                moved.insert(pointSelection.indices().begin(), pointSelection.indices().end());
            drag = PointDragging{activeTrackIndex, events, moved, hit, false, {events, moved}};
            repaint();
            return;
        }

        if (editToolSelected != e.mods.isCommandDown())
        {
            const int tick = std::max(0, snapGrid(e.mods).round(xToTick(e.x)));
            openProgramBrowser(tick, tickToX(tick));
            return;
        }

        drag = PointRangeSelecting{{e.x, e.x, e.mods.isShiftDown() ? pointSelection.indices() : std::set<int>{}}};
        if (!e.mods.isShiftDown())
        {
            pointSelection.clear();
            publishPointSelection();
        }
        repaint();
        return;
    }

    if (kind)
    {
        pointMouseDown(e, *kind);
        return;
    }

    if (displayMode != DisplayMode::Velocity)
        return;

    if (editToolSelected == e.mods.isCommandDown())
    {
        if (const auto grabbed = velocityTopAt(e.x, e.y); grabbed.isValid())
        {
            if (const auto note = editableNotes().find(grabbed))
                drag = VelocityDragging{
                    activeTrackIndex, e.x, {{grabbed, note->velocity}}, VelocityTopGrab{e.y, note->velocity}};
            return;
        }
        drag = VelocityRangeSelecting{{e.x, e.x, {}},
                                      e.mods.isShiftDown() && noteSelection ? noteSelection() : std::set<NoteRef>{}};
        selectNotesInVelocityRange();
        repaint();
        return;
    }

    int newVelocity = yToValue(e.y);

    NoteRef best;
    int bestDist = INT_MAX;
    for (const auto& [ref, note] : editableNotes().onTrack(activeTrackIndex))
    {
        int nx = tickToX(note.startTick);
        if (e.x >= nx - velocityBarHitMargin && e.x < nx + velocityBarWidth + velocityBarHitMargin)
        {
            int dist = std::abs(nx - e.x);
            // Recorded notes come after the sequence's, so a tie goes to the recorded one, as in the piano roll.
            if (dist <= bestDist && (dist < bestDist || ref.isRecorded()))
            {
                bestDist = dist;
                best = ref;
            }
        }
    }

    if (best.isValid())
    {
        drag = VelocityDragging{activeTrackIndex, e.x, {{best, newVelocity}}};
        publishDragPreview();
        repaint();
    }
    else
    {
        // Pressed between bars, so the drag still sets the bars it crosses.
        drag = VelocityDragging{activeTrackIndex, e.x, {}};
    }
}

void ControllerLaneComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (auto* pointDragging = std::get_if<PointDragging>(&drag))
    {
        const auto kind = editedKind();
        if (!e.mouseWasDraggedSinceMouseDown() || !kind)
            return;
        // Measured from where the anchor is drawn, not from the mouse, so grabbing it off-centre does not move it.
        const auto& anchor = pointDragging->before[static_cast<size_t>(pointDragging->anchor)];
        const int tick = ControllerLaneScale::draggedTick(anchor.tick, tickToX(anchor.tick),
                                                          e.getDistanceFromDragStartX(), [this, &e](int x)
                                                          { return std::max(0, snapGrid(e.mods).round(xToTick(x))); });
        const int value = showsProgramChange()
                              ? kind->valueOf(anchor)
                              : ControllerLaneScale::draggedValue(
                                    kind->valueOf(anchor), pointY(*kind, kind->valueOf(anchor)),
                                    e.getDistanceFromDragStartY(), [this, &kind](int y) { return valueAtY(*kind, y); });
        pointDragging->preview = ControllerPointEdits::move(pointDragging->before, *kind, pointDragging->moved,
                                                            pointDragging->anchor, tick, value);
        pointDragging->changed = pointDragging->preview.events != pointDragging->before;
        if (const auto landed = ControllerPointEdits::movedAnchor(pointDragging->before, *kind, pointDragging->moved,
                                                                  pointDragging->anchor, tick, value))
            pointDragging->tip = ValueTip{e.getPosition(), *landed};
        repaint();
        return;
    }

    if (auto* selecting = std::get_if<PointRangeSelecting>(&drag))
    {
        const auto kind = editedKind();
        if (!kind || !hasActiveTrack())
            return;
        selecting->gesture.currentX = e.x;
        const auto& events = sequence->getTrack(activeTrackIndex).getEvents();
        struct LaneGeometry
        {
            const ControllerLaneComponent& lane;
            int xToTick(int x) const { return lane.xToTick(x); }
        };
        pointSelection.assign(selecting->gesture.selectionFor(static_cast<int>(events.size()), LaneGeometry{*this},
                                                              [&events, &kind](int i, int tickLo, int tickHi)
                                                              {
                                                                  const auto& event = events[static_cast<size_t>(i)];
                                                                  return kind->matches(event) && event.tick >= tickLo &&
                                                                         event.tick <= tickHi;
                                                              }));
        pointSelectionChanged();
        repaint();
        return;
    }

    if (auto* selectingNotes = std::get_if<VelocityRangeSelecting>(&drag))
    {
        selectingNotes->gesture.currentX = e.x;
        selectNotesInVelocityRange();
        repaint();
        return;
    }

    auto* dragging = std::get_if<VelocityDragging>(&drag);
    if (dragging == nullptr || !sequence)
        return;
    if (dragging->trackIndex < 0 || dragging->trackIndex >= sequence->getNumTracks())
        return;

    if (displayMode != DisplayMode::Velocity)
        return;

    if (dragging->topGrab)
    {
        dragging->preview.begin()->second = VelocityBarGrab::draggedValue(*dragging->topGrab, e.y, getDrawAreaHeight());
        publishDragPreview();
        repaint();
        return;
    }

    int newVelocity = yToValue(e.y);

    int startX = std::min(dragging->lastDragX, e.x);
    int endX = std::max(dragging->lastDragX, e.x);

    bool changed = false;
    for (const auto& [ref, note] : editableNotes().onTrack(dragging->trackIndex))
    {
        int nx = tickToX(note.startTick);
        if (nx + velocityBarWidth + velocityBarHitMargin >= startX && nx - velocityBarHitMargin <= endX)
        {
            dragging->preview[ref] = newVelocity;
            changed = true;
        }
    }

    dragging->lastDragX = e.x;

    if (changed)
    {
        publishDragPreview();
        repaint();
    }
}

void ControllerLaneComponent::mouseUp(const juce::MouseEvent&)
{
    const juce::ScopeGuard publishAtEnd{[this] { publishDragPreview(); }};
    auto state = std::exchange(drag, Idle{});

    if (const auto* pointDragging = std::get_if<PointDragging>(&state))
    {
        // An edit made elsewhere during the drag leaves base stale; applying it would undo that edit.
        const auto& base = pointDragging->beforeAdd ? *pointDragging->beforeAdd : pointDragging->before;
        const bool stillCurrent = pointDragging->trackIndex == activeTrackIndex && hasActiveTrack() &&
                                  sequence->getTrack(activeTrackIndex).getEvents() == base;
        if (pointDragging->beforeAdd)
        {
            if (stillCurrent)
                applyPointEdit("Add Controller Point", base, pointDragging->preview);
        }
        else if (pointDragging->changed && stillCurrent)
            applyPointEdit(showsProgramChange() ? "Move Program Changes" : "Move Controller Points",
                           pointDragging->before, pointDragging->preview);
        else if (!pointDragging->changed)
        {
            if (showsProgramChange())
            {
                const int tick = pointDragging->before[static_cast<size_t>(pointDragging->anchor)].tick;
                const auto block = ProgramChangeEdits::onTick(pointDragging->before, tick);
                if (block == pointSelection.indices() && !programEditSession.isOpen())
                    openProgramBrowser(tick, tickToX(tick));
                else
                    selectPoints(block);
            }
            else
                selectPoints({pointDragging->anchor});
        }
        repaint();
        return;
    }

    if (std::holds_alternative<PointRangeSelecting>(state))
    {
        repaint();
        return;
    }

    if (std::holds_alternative<VelocityRangeSelecting>(state))
    {
        repaint();
        return;
    }

    const auto* dragging = std::get_if<VelocityDragging>(&state);
    if (dragging == nullptr || !sequence || dragging->trackIndex < 0 ||
        dragging->trackIndex >= sequence->getNumTracks())
        return;

    const auto notes = editableNotes();
    std::vector<NoteEdit> edits;
    for (const auto& [ref, velocity] : dragging->preview)
        if (const auto note = notes.find(ref); note.has_value() && note->velocity != velocity)
            edits.push_back({ref, {.velocity = velocity}});
    LayeredNoteEdits(*sequence, undoHistory, recordingEditor).apply(edits, "Edit Velocity");
}

void ControllerLaneComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (programEditSession.isOpen())
        return;

    if (showsProgramChange())
    {
        if (!hasActiveTrack() || e.mods.isPopupMenu() || e.mods.isShiftDown() ||
            editToolSelected != e.mods.isCommandDown() || isOnLabelPanel(e.x) || hitTestLabel(e.x) >= 0)
            return;

        const int tick = std::max(0, snapGrid(e.mods).round(xToTick(e.x)));
        openProgramBrowser(tick, tickToX(tick));
        return;
    }

    const int pressed = std::exchange(doubleClickPoint, -1);
    if (pressed < 0 || e.mods.isPopupMenu() || e.mods.isShiftDown() || valueEditSession.isOpen() ||
        hitTestPoint(e.x, e.y) != pressed)
        return;
    const auto& events = sequence->getTrack(activeTrackIndex).getEvents();
    applyPointEdit("Delete Controller Point", events, ControllerPointEdits::remove(events, {pressed}));
}

void ControllerLaneComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (onMouseWheel)
        onMouseWheel(e, w);
    else
        Component::mouseWheelMove(e, w);
}

int ControllerLaneComponent::tickToX(int tick) const
{
    if (!sequence)
        return leftPanelWidth;

    double beatsFromTick = static_cast<double>(tick) / sequence->getTimeline().getTicksPerQuarterNote();
    return leftPanelWidth + static_cast<int>(beatsFromTick * beatWidth);
}

void ControllerLaneComponent::notesChanged(int)
{
    resetPointEditing();
    publishDragPreview();
    repaint();
}
void ControllerLaneComponent::tracksChanged()
{
    resetPointEditing();
    publishDragPreview();
    repaint();
}
void ControllerLaneComponent::tempoChanged()
{
    drag = Idle{};
    publishDragPreview();
    repaint();
}
void ControllerLaneComponent::timelineMetadataChanged()
{
    drag = Idle{};
    publishDragPreview();
    repaint();
}

void ControllerLaneComponent::sequenceReset()
{
    resetPointEditing();
    publishDragPreview();
    repaint();
}

void ControllerLaneComponent::drawLeftPanel(juce::Graphics& g)
{
    using namespace calliope::theme;
    auto leftRect = juce::Rectangle<int>(0, 0, leftPanelWidth, getHeight());

    auto* vp = findParentComponentOfClass<juce::Viewport>();
    if (vp)
        leftRect.setX(vp->getViewPositionX());

    g.setColour(surface::surface);
    g.fillRect(leftRect);

    g.setColour(border::strong);
    g.drawVerticalLine(leftRect.getRight() - 1, 0.0f, static_cast<float>(getHeight()));

    juce::String label;
    switch (displayMode)
    {
    case DisplayMode::Velocity:
        label = "Velocity";
        break;
    case DisplayMode::ControlChange:
        label = "CC " + juce::String(ccNumber);
        break;
    case DisplayMode::PitchBend:
        label = "PitchBend";
        break;
    case DisplayMode::ProgramChange:
        label = "PC";
        break;
    }

    g.setColour(text::t2);
    g.setFont(font::sans(font::sizeSM));
    g.drawText(label, leftRect.reduced(4, 0), juce::Justification::centred, true);

    g.setFont(font::sans(font::size2XS));
    g.setColour(text::t3);

    int lx = leftRect.getX() + 2;
    int lw = leftPanelWidth - 6;

    if (displayMode == DisplayMode::PitchBend)
    {
        g.drawText("+8191", lx, getDrawAreaTop() - 5, lw, 10, juce::Justification::right);
        g.drawText("0", lx, (getDrawAreaTop() + getDrawAreaBottom()) / 2 - 5, lw, 10, juce::Justification::right);
        g.drawText("-8192", lx, getDrawAreaBottom() - 5, lw, 10, juce::Justification::right);
    }
    else if (displayMode != DisplayMode::ProgramChange)
    {
        g.drawText("127", lx, getDrawAreaTop() - 5, lw, 10, juce::Justification::right);
        g.drawText("0", lx, getDrawAreaBottom() - 5, lw, 10, juce::Justification::right);
    }
}

void ControllerLaneComponent::drawGrid(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (!sequence)
        return;

    auto* vp = findParentComponentOfClass<juce::Viewport>();
    int visibleLeft = vp ? vp->getViewPositionX() : 0;
    int visibleRight = vp ? visibleLeft + vp->getViewWidth() : getWidth();

    if (displayMode == DisplayMode::PitchBend)
    {
        int centerY = (getDrawAreaTop() + getDrawAreaBottom()) / 2;
        g.setColour(border::strong);
        g.drawHorizontalLine(centerY, static_cast<float>(leftPanelWidth), static_cast<float>(getWidth()));
    }
    else if (displayMode != DisplayMode::ProgramChange)
    {
        for (int v : {32, 64, 96})
        {
            int y = valueToY(v);
            float alpha = (v == 64) ? 0.12f : 0.06f;
            g.setColour(text::t3.withAlpha(alpha));
            g.drawHorizontalLine(y, static_cast<float>(leftPanelWidth), static_cast<float>(getWidth()));
        }
    }

    const TimelineGrid grid(sequence->getTimeline(), gridDivision);
    const int firstX = std::max(visibleLeft, leftPanelWidth);
    for (const auto& line : grid.lines(xToTick(firstX - 1), xToTick(visibleRight + 1)))
    {
        const int x = tickToX(line.tick);
        if (x < firstX || x > visibleRight)
            continue;

        switch (line.kind)
        {
        case GridLineKind::Bar:
            g.setColour(border::strong);
            break;
        case GridLineKind::Beat:
            g.setColour(border::soft);
            break;
        case GridLineKind::Sub:
            g.setColour(surface::hover);
            break;
        }
        g.drawVerticalLine(x, static_cast<float>(getDrawAreaTop()), static_cast<float>(getDrawAreaBottom()));
    }
}

void ControllerLaneComponent::drawVelocity(juce::Graphics& g)
{
    if (!sequence)
        return;

    auto* vp = findParentComponentOfClass<juce::Viewport>();
    int visibleLeft = vp ? vp->getViewPositionX() : 0;
    int visibleRight = vp ? visibleLeft + vp->getViewWidth() : getWidth();

    int bottom = getDrawAreaBottom();

    const auto notes = editableNotes();
    const auto selected = noteSelection ? noteSelection() : std::set<NoteRef>{};
    for (int trackIdx : selectedTrackIndices)
    {
        if (trackIdx < 0 || trackIdx >= sequence->getNumTracks())
            continue;

        auto colour = TrackColours::getColour(trackIdx);
        bool isActive = (trackIdx == activeTrackIndex);
        float alpha = isActive ? 0.85f : 0.3f;

        for (const auto& [ref, note] : notes.onTrack(trackIdx))
        {
            int x = tickToX(note.startTick);

            if (x + velocityBarWidth < visibleLeft || x > visibleRight)
                continue;

            int barHeight = static_cast<int>(static_cast<float>(displayedVelocity(ref, note.velocity)) / 127.0f *
                                             getDrawAreaHeight());
            int barTop = bottom - barHeight;

            const bool isSelected = selected.contains(ref);
            g.setColour((isSelected ? colour.brighter(0.4f) : colour).withAlpha(alpha));
            g.fillRect(x, barTop, velocityBarWidth, barHeight);

            if (ref.isRecorded())
            {
                g.setColour(calliope::theme::status::danger.withAlpha(alpha));
                g.drawRect(x, barTop, velocityBarWidth, barHeight, 1);
            }
            else if (isSelected)
            {
                g.setColour(calliope::theme::text::t1);
                g.drawRect(x, barTop, velocityBarWidth, barHeight, 1);
            }
            else if (isActive)
            {
                g.setColour(colour.brighter(0.3f).withAlpha(0.6f));
                g.drawRect(x, barTop, velocityBarWidth, barHeight, 1);
            }
        }
    }
}

ControllerLaneComponent::VelocityBars ControllerLaneComponent::activeVelocityBars() const
{
    VelocityBars result;
    if (!hasActiveTrack())
        return result;
    for (const auto& [ref, note] : editableNotes().onTrack(activeTrackIndex))
    {
        result.refs.push_back(ref);
        result.bars.push_back({tickToX(note.startTick), valueToY(note.velocity)});
    }
    return result;
}

NoteRef ControllerLaneComponent::velocityTopAt(int x, int y) const
{
    if (displayMode != DisplayMode::Velocity || !hasActiveTrack() || isOnLabelPanel(x))
        return {};

    const auto [refs, bars] = activeVelocityBars();
    const int hit = VelocityBarGrab::hit(bars, x, y, velocityBarWidth, velocityBarHitMargin, velocityTopTolerance);
    return hit >= 0 ? refs[static_cast<size_t>(hit)] : NoteRef{};
}

void ControllerLaneComponent::selectNotesInVelocityRange()
{
    const auto* selecting = std::get_if<VelocityRangeSelecting>(&drag);
    if (selecting == nullptr || !onNotesSelected)
        return;

    auto selected = selecting->base;
    const auto [refs, bars] = activeVelocityBars();
    for (const int i :
         VelocityBarSelection::overlapping(bars, selecting->gesture.lo(), selecting->gesture.hi(), velocityBarWidth))
        selected.insert(refs[static_cast<size_t>(i)]);
    if (!noteSelection || selected != noteSelection())
        onNotesSelected(selected);
}

EditableNotes ControllerLaneComponent::editableNotes() const
{
    return {*sequence, recordingLayer, static_cast<int>(playheadTick)};
}

int ControllerLaneComponent::displayedVelocity(const NoteRef& ref, int velocity) const
{
    if (const auto* dragging = std::get_if<VelocityDragging>(&drag))
        if (const auto it = dragging->preview.find(ref); it != dragging->preview.end())
            return it->second;
    return velocity;
}

NoteDragPreview ControllerLaneComponent::dragPreview() const
{
    NoteDragPreview preview;
    const auto* dragging = std::get_if<VelocityDragging>(&drag);
    if (dragging == nullptr || !sequence)
        return preview;

    preview.dragging = true;
    const auto notes = editableNotes();
    for (const auto& [ref, velocity] : dragging->preview)
    {
        auto note = notes.find(ref);
        if (!note.has_value() || note->velocity == velocity)
            continue;
        note->velocity = velocity;
        preview.changedNotes[ref] = *note;
    }
    return preview;
}

void ControllerLaneComponent::publishDragPreview()
{
    auto preview = dragPreview();
    if (preview == publishedPreview)
        return;
    publishedPreview = std::move(preview);
    if (onNoteDragPreviewChanged)
        onNoteDragPreviewChanged(publishedPreview);
}

void ControllerLaneComponent::drawControlChange(juce::Graphics& g)
{
    if (!sequence)
        return;

    for (int trackIdx : selectedTrackIndices)
    {
        if (trackIdx < 0 || trackIdx >= sequence->getNumTracks())
            continue;

        auto colour = TrackColours::getColour(trackIdx);
        float alpha = (trackIdx == activeTrackIndex) ? 0.85f : 0.3f;
        drawControllerPoints(g, ControllerKind::controlChange(ccNumber), trackIdx, colour, alpha);

        const auto recorded = recordedValues(trackIdx,
                                             [this](const MidiEvent& e) -> std::optional<int>
                                             {
                                                 if (e.type == MidiEvent::Type::ControlChange && e.data1 == ccNumber)
                                                     return e.data2;
                                                 return std::nullopt;
                                             });
        if (!recorded.empty())
            drawStepGraph(g, recorded, calliope::theme::status::danger, 0.85f, [this](int v) { return valueToY(v); });
    }
}

void ControllerLaneComponent::drawPitchBend(juce::Graphics& g)
{
    if (!sequence)
        return;

    const auto toY = [this](int v) { return pointY(ControllerKind::pitchBend(), v); };

    for (int trackIdx : selectedTrackIndices)
    {
        if (trackIdx < 0 || trackIdx >= sequence->getNumTracks())
            continue;

        auto colour = TrackColours::getColour(trackIdx);
        float alpha = (trackIdx == activeTrackIndex) ? 0.85f : 0.3f;
        drawControllerPoints(g, ControllerKind::pitchBend(), trackIdx, colour, alpha);

        const auto recorded = recordedValues(trackIdx,
                                             [](const MidiEvent& e) -> std::optional<int>
                                             {
                                                 if (e.type == MidiEvent::Type::PitchBend)
                                                     return e.data1;
                                                 return std::nullopt;
                                             });
        if (!recorded.empty())
            drawStepGraph(g, recorded, calliope::theme::status::danger, 0.85f, toY);
    }
}

void ControllerLaneComponent::drawProgramChange(juce::Graphics& g)
{
    if (!sequence)
        return;

    for (int trackIdx : selectedTrackIndices)
    {
        if (trackIdx < 0 || trackIdx >= sequence->getNumTracks())
            continue;

        const auto& trackEvents = displayedEvents(trackIdx);
        bool isActive = (trackIdx == activeTrackIndex);
        float alpha = isActive ? 0.85f : 0.3f;

        std::vector<std::pair<int, int>> events;
        std::set<int> selectedTicks;
        const auto selected = isActive ? displayedSelection() : std::set<int>{};
        for (int i = 0; i < static_cast<int>(trackEvents.size()); ++i)
        {
            const auto& event = trackEvents[static_cast<size_t>(i)];
            if (event.type == MidiEvent::Type::ProgramChange)
            {
                events.push_back({event.tick, event.data1});
                if (selected.contains(i))
                    selectedTicks.insert(event.tick);
            }
        }
        std::ranges::stable_sort(events, {}, &std::pair<int, int>::first);

        auto colour = TrackColours::getColour(trackIdx);
        if (!events.empty())
            drawProgramChanges(g, events, colour, alpha, selectedTicks);

        const auto recorded = recordedValues(trackIdx,
                                             [](const MidiEvent& e) -> std::optional<int>
                                             {
                                                 if (e.type == MidiEvent::Type::ProgramChange)
                                                     return e.data1;
                                                 return std::nullopt;
                                             });
        if (!recorded.empty())
            drawProgramChanges(g, recorded, calliope::theme::status::danger, 0.85f);
    }
}

void ControllerLaneComponent::drawProgramChanges(juce::Graphics& g, const std::vector<std::pair<int, int>>& events,
                                                 juce::Colour colour, float alpha, const std::set<int>& selectedTicks)
{
    using namespace calliope::theme;
    auto* vp = findParentComponentOfClass<juce::Viewport>();
    int visibleLeft = vp ? vp->getViewPositionX() : 0;
    int visibleRight = vp ? visibleLeft + vp->getViewWidth() : getWidth();
    const auto labelFont = font::sans(font::sizeSM);
    const int labelHeight = juce::roundToInt(labelFont.getHeight()) + 4;
    const int labelY = getDrawAreaTop() + (getDrawAreaHeight() - labelHeight) / 2;

    g.setFont(labelFont);
    for (size_t i = 0; i < events.size(); ++i)
    {
        int x1 = tickToX(events[i].first);
        int x2 = (i + 1 < events.size()) ? tickToX(events[i + 1].first) : getWidth();

        if (x2 < visibleLeft || x1 > visibleRight)
            continue;

        g.setColour(colour.withAlpha(alpha * 0.6f));
        g.drawVerticalLine(x1, static_cast<float>(getDrawAreaTop()), static_cast<float>(getDrawAreaBottom()));

        const int labelRight = std::min(x1 + programLabelWidth(events[i].second), x2);
        auto labelRect = juce::Rectangle<int>(x1, labelY, labelRight - x1, labelHeight);
        const bool selected = selectedTicks.contains(events[i].first);
        if (selected)
        {
            g.setColour(surface::selection);
            g.fillRoundedRectangle(labelRect.toFloat(), radius::r1);
        }

        g.setColour((selected ? colour.brighter(0.5f) : colour).withAlpha(alpha));
        auto textRect = labelRect.reduced(programLabelInset, 0);
        if (textRect.getWidth() > 10)
            g.drawText(programLabel(events[i].second), textRect, juce::Justification::centredLeft, true);
    }
}

void ControllerLaneComponent::drawStepGraph(juce::Graphics& g, const std::vector<std::pair<int, int>>& events,
                                            juce::Colour colour, float alpha, std::function<int(int)> toY)
{
    auto* vp = findParentComponentOfClass<juce::Viewport>();
    int visibleLeft = vp ? vp->getViewPositionX() : 0;
    int visibleRight = vp ? visibleLeft + vp->getViewWidth() : getWidth();

    juce::Path path;
    for (size_t i = 0; i < events.size(); ++i)
    {
        int x = tickToX(events[i].first);
        int y = toY(events[i].second);

        if (i == 0)
            path.startNewSubPath(static_cast<float>(x), static_cast<float>(y));
        else
            path.lineTo(static_cast<float>(x), static_cast<float>(y));

        int nextX = (i + 1 < events.size()) ? tickToX(events[i + 1].first) : getWidth();
        path.lineTo(static_cast<float>(nextX), static_cast<float>(y));
    }

    g.setColour(colour.withAlpha(alpha));
    g.strokePath(path, juce::PathStrokeType(2.0f));

    for (const auto& [tick, value] : events)
    {
        int x = tickToX(tick);
        if (x < visibleLeft - 10 || x > visibleRight + 10)
            continue;
        int y = toY(value);
        g.fillEllipse(static_cast<float>(x - 3), static_cast<float>(y - 3), 6.0f, 6.0f);
    }
}

void ControllerLaneComponent::drawPlayhead(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (!sequence)
        return;

    int x = tickToX(static_cast<int>(playheadTick));
    if (x < leftPanelWidth)
        return;

    g.setColour(text::t1);
    g.drawVerticalLine(x, 0.0f, static_cast<float>(getHeight()));
}

void ControllerLaneComponent::drawLoopRegion(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (!sequence || loopEndTick <= loopStartTick)
        return;

    int x1 = tickToX(loopStartTick);
    int x2 = tickToX(loopEndTick);
    auto clip = g.getClipBounds();

    if (x2 < clip.getX() || x1 > clip.getRight())
        return;

    g.setColour(loopEnabled ? accent::soft : surface::hover);
    g.fillRect(x1, 0, x2 - x1, getHeight());

    g.setColour(loopEnabled ? accent::base.withAlpha(0.7f) : text::t4);
    g.drawVerticalLine(x1, 0.0f, static_cast<float>(getHeight()));
    g.drawVerticalLine(x2, 0.0f, static_cast<float>(getHeight()));
}

int ControllerLaneComponent::xToTick(int x) const
{
    if (!sequence)
        return 0;

    double beats = static_cast<double>(x - leftPanelWidth) / beatWidth;
    return static_cast<int>(beats * sequence->getTimeline().getTicksPerQuarterNote());
}

int ControllerLaneComponent::valueToY(int value) const
{
    float ratio = static_cast<float>(value) / 127.0f;
    return getDrawAreaBottom() - static_cast<int>(ratio * getDrawAreaHeight());
}

int ControllerLaneComponent::yToValue(int y) const
{
    float ratio = static_cast<float>(getDrawAreaBottom() - y) / static_cast<float>(getDrawAreaHeight());
    return juce::jlimit(0, 127, static_cast<int>(ratio * 127.0f));
}

int ControllerLaneComponent::getDrawAreaTop() const
{
    return topPadding;
}

int ControllerLaneComponent::getDrawAreaBottom() const
{
    return getHeight() - bottomPadding;
}

int ControllerLaneComponent::getDrawAreaHeight() const
{
    return getDrawAreaBottom() - getDrawAreaTop();
}

void ControllerLaneComponent::drawControllerPoints(juce::Graphics& g, const ControllerKind& kind, int trackIndex,
                                                   juce::Colour colour, float alpha)
{
    using namespace calliope::theme;
    const auto& events = displayedEvents(trackIndex);
    std::vector<int> points;
    for (int i = 0; i < static_cast<int>(events.size()); ++i)
        if (kind.matches(events[static_cast<size_t>(i)]))
            points.push_back(i);
    if (points.empty())
        return;
    std::ranges::stable_sort(points, {}, [&events](int i) { return events[static_cast<size_t>(i)].tick; });

    juce::Path path;
    float prevY = 0.0f;
    for (size_t n = 0; n < points.size(); ++n)
    {
        const auto& event = events[static_cast<size_t>(points[n])];
        const auto x = static_cast<float>(tickToX(event.tick));
        const auto y = static_cast<float>(pointY(kind, kind.valueOf(event)));
        if (n == 0)
            path.startNewSubPath(x, y);
        else if (event.ramp && kind.canRamp())
            path.lineTo(x, y);
        else
        {
            path.lineTo(x, prevY);
            path.lineTo(x, y);
        }
        prevY = y;
    }
    path.lineTo(static_cast<float>(getWidth()), prevY);

    g.setColour(colour.withAlpha(alpha));
    g.strokePath(path, juce::PathStrokeType(2.0f));

    auto* vp = findParentComponentOfClass<juce::Viewport>();
    const int visibleLeft = vp ? vp->getViewPositionX() : 0;
    const int visibleRight = vp ? visibleLeft + vp->getViewWidth() : getWidth();
    const auto selected = trackIndex == activeTrackIndex ? displayedSelection() : std::set<int>{};
    for (int i : points)
    {
        const auto& event = events[static_cast<size_t>(i)];
        const int x = tickToX(event.tick);
        if (x < visibleLeft - 10 || x > visibleRight + 10)
            continue;
        const bool isSelected = selected.contains(i);
        const float radius = isSelected ? 4.0f : 3.0f;
        const juce::Rectangle<float> dot(static_cast<float>(x) - radius,
                                         static_cast<float>(pointY(kind, kind.valueOf(event))) - radius, radius * 2.0f,
                                         radius * 2.0f);
        g.setColour(colour.withAlpha(alpha));
        g.fillEllipse(dot);
        if (isSelected)
        {
            g.setColour(text::t1);
            g.drawEllipse(dot, 1.5f);
        }
    }
}

bool ControllerLaneComponent::showsProgramChange() const
{
    return displayMode == DisplayMode::ProgramChange;
}

std::optional<ControllerKind> ControllerLaneComponent::editedKind() const
{
    if (displayMode == DisplayMode::ControlChange)
        return ControllerKind::controlChange(ccNumber);
    if (displayMode == DisplayMode::PitchBend)
        return ControllerKind::pitchBend();
    if (showsProgramChange())
        return ControllerKind::programChange();
    return std::nullopt;
}

const std::vector<MidiEvent>& ControllerLaneComponent::displayedEvents(int trackIndex) const
{
    if (const auto* dragging = std::get_if<PointDragging>(&drag);
        dragging != nullptr && dragging->trackIndex == trackIndex)
        return dragging->preview.events;
    return sequence->getTrack(trackIndex).getEvents();
}

std::set<int> ControllerLaneComponent::displayedSelection() const
{
    if (const auto* dragging = std::get_if<PointDragging>(&drag))
        return dragging->preview.selected;
    return pointSelection.indices();
}

bool ControllerLaneComponent::isOnLabelPanel(int x) const
{
    // The label panel stays at the left of the view, over whatever is scrolled under it.
    const auto* viewport = findParentComponentOfClass<juce::Viewport>();
    return x < (viewport != nullptr ? viewport->getViewPositionX() : 0) + leftPanelWidth;
}

bool ControllerLaneComponent::hasActiveTrack() const
{
    return sequence != nullptr && activeTrackIndex >= 0 && activeTrackIndex < sequence->getNumTracks();
}

int ControllerLaneComponent::hitTestPoint(int x, int y) const
{
    const auto kind = editedKind();
    if (!kind || showsProgramChange() || !hasActiveTrack() || isOnLabelPanel(x))
        return -1;

    const auto& events = sequence->getTrack(activeTrackIndex).getEvents();
    std::vector<int> points;
    for (int i = 0; i < static_cast<int>(events.size()); ++i)
        if (kind->matches(events[static_cast<size_t>(i)]))
            points.push_back(i);
    std::ranges::stable_sort(points, {}, [&events](int i) { return events[static_cast<size_t>(i)].tick; });

    std::vector<std::pair<float, float>> offsets;
    offsets.reserve(points.size());
    for (int i : points)
    {
        const auto& event = events[static_cast<size_t>(i)];
        offsets.push_back(
            {static_cast<float>(x - tickToX(event.tick)), static_cast<float>(y - pointY(*kind, kind->valueOf(event)))});
    }
    const int nearest = ControllerLaneScale::nearestWithin(offsets, pointHitRadius);
    return nearest < 0 ? -1 : points[static_cast<size_t>(nearest)];
}

int ControllerLaneComponent::hitTestLabel(int x) const
{
    if (!hasActiveTrack() || isOnLabelPanel(x))
        return -1;

    const auto& events = sequence->getTrack(activeTrackIndex).getEvents();
    const auto tick = ProgramChangeEdits::blockTickAt(events, xToTick(x));
    if (!tick)
        return -1;
    const auto block = ProgramChangeEdits::onTick(events, *tick);
    if (block.empty())
        return -1;

    const int hit = *block.rbegin();
    const int labelRight = tickToX(*tick) + programLabelWidth(events[static_cast<size_t>(hit)].data1);
    return x < labelRight ? hit : -1;
}

int ControllerLaneComponent::pointY(const ControllerKind& kind, int value) const
{
    if (kind.type != MidiEvent::Type::PitchBend)
        return valueToY(value);
    const int centerY = (getDrawAreaTop() + getDrawAreaBottom()) / 2;
    const int halfHeight = getDrawAreaHeight() / 2;
    const float offset = static_cast<float>(value - ControllerLaneScale::pitchBendCentre) /
                         static_cast<float>(ControllerLaneScale::pitchBendCentre);
    return centerY - static_cast<int>(offset * static_cast<float>(halfHeight));
}

int ControllerLaneComponent::valueAtY(const ControllerKind& kind, int y) const
{
    if (kind.type != MidiEvent::Type::PitchBend)
        return yToValue(y);
    const int centerY = (getDrawAreaTop() + getDrawAreaBottom()) / 2;
    return ControllerLaneScale::pitchBendAt(centerY - y, getDrawAreaHeight() / 2);
}

TimelineGrid ControllerLaneComponent::snapGrid(const juce::ModifierKeys& mods) const
{
    if (snapEnabled != mods.isShiftDown())
        return TimelineGrid(sequence->getTimeline(), gridDivision);
    return TimelineGrid::everyTick(sequence->getTimeline());
}

void ControllerLaneComponent::pointMouseDown(const juce::MouseEvent& e, const ControllerKind& kind)
{
    const auto& events = sequence->getTrack(activeTrackIndex).getEvents();
    if (const int hit = hitTestPoint(e.x, e.y); hit >= 0)
    {
        if (e.mods.isShiftDown())
        {
            pointSelection.toggle(hit);
            pointSelectionChanged();
            repaint();
            return;
        }
        const auto group = pointSelection.dragGroup(hit);
        const std::set<int> moved(group.begin(), group.end());
        drag = PointDragging{activeTrackIndex, events, moved, hit, false, {events, moved}};
        repaint();
        return;
    }

    if (editToolSelected != e.mods.isCommandDown())
    {
        const int tick = std::max(0, snapGrid(e.mods).round(xToTick(e.x)));
        applyPointEdit("Add Controller Point", events,
                       ControllerPointEdits::add(events, kind, tick, valueAtY(kind, e.y)));
        return;
    }

    if (const auto lineValue = controllerLineValueAt(events, kind, xToTick(e.x));
        !e.mods.isShiftDown() && lineValue && std::abs(e.y - pointY(kind, *lineValue)) <= lineHitTolerance)
    {
        const int tick = std::max(0, snapGrid(e.mods).round(xToTick(e.x)));
        const bool taken = std::ranges::any_of(events, [&](const MidiEvent& event)
                                               { return kind.matches(event) && event.tick == tick; });
        if (!taken)
        {
            auto added = ControllerPointEdits::add(events, kind, tick,
                                                   controllerLineValueAt(events, kind, tick).value_or(*lineValue));
            const int anchor = *added.selected.begin();
            drag = PointDragging{activeTrackIndex, added.events, added.selected, anchor, false, added, {}, events};
            repaint();
        }
        return;
    }

    drag = PointRangeSelecting{{e.x, e.x, e.mods.isShiftDown() ? pointSelection.indices() : std::set<int>{}}};
    if (!e.mods.isShiftDown())
    {
        pointSelection.clear();
        publishPointSelection();
    }
    repaint();
}

void ControllerLaneComponent::applyPointEdit(const juce::String& transactionName, const std::vector<MidiEvent>& before,
                                             const ControllerPointEditResult& result)
{
    if (!hasActiveTrack())
        return;
    // Copied first: before may be the track's own list, which the edit replaces.
    auto previous = before;
    auto selected = result.selected;
    performReplaceEvents(undoHistory, sequence, transactionName, sequence->getTrack(activeTrackIndex).getId(),
                         std::move(previous), result.events);
    // The edit's own notification clears the selection, so the new one is set afterwards.
    selectPoints(std::move(selected));
}

void ControllerLaneComponent::selectPoints(std::set<int> indices)
{
    pointSelection.assign(std::move(indices));
    pointSelectionChanged();
    repaint();
}

void ControllerLaneComponent::pointSelectionChanged()
{
    // A selection already sent has taken the selection from the piano roll, so a range drag takes it once.
    if (!pointSelection.isEmpty() && publishedPointSelection.empty() && onPointSelectionTaken)
        onPointSelectionTaken();
    publishPointSelection();
}

void ControllerLaneComponent::publishPointSelection()
{
    const auto selected = editedKind() && hasActiveTrack() ? pointSelection.indices() : std::set<int>{};
    if (selected == publishedPointSelection && (selected.empty() || activeTrackIndex == publishedPointTrack))
        return;
    publishedPointTrack = activeTrackIndex;
    publishedPointSelection = selected;
    if (onPointSelectionChanged)
        onPointSelectionChanged(activeTrackIndex, publishedPointSelection);
}

void ControllerLaneComponent::resetPointEditing()
{
    drag = Idle{};
    pointSelection.clear();
    doubleClickPoint = -1;
    valueEditSession.close();
    programEditSession.close();
    publishPointSelection();
}

void ControllerLaneComponent::openValueEditor(int eventIndex)
{
    const auto kind = editedKind();
    if (!kind || !hasActiveTrack())
        return;
    const auto& event = sequence->getTrack(activeTrackIndex).getEvent(eventIndex);
    const bool bend = kind->type == MidiEvent::Type::PitchBend;
    const int value = kind->valueOf(event);
    const int displayed = bend ? ControllerLaneScale::displayedPitchBend(value) : value;
    const int minDisplayed = bend ? ControllerLaneScale::displayedPitchBend(0) : 0;
    const int maxDisplayed = bend ? ControllerLaneScale::displayedPitchBend(kind->maxValue()) : kind->maxValue();
    const auto anchor = juce::Rectangle<int>(tickToX(event.tick), pointY(*kind, value), 1, 1).expanded(4);

    valueEditSession.close();
    selectPoints({eventIndex});

    auto content = std::make_unique<ControllerValueEditor>(displayed, minDisplayed, maxDisplayed);
    content->onCommit = [this](int displayedValue) { commitValueEdit(displayedValue); };
    content->onCancel = [this]() { valueEditSession.finish(); };
    valueEditSession.open({eventIndex}, std::move(content), localAreaToGlobal(anchor));
}

void ControllerLaneComponent::commitValueEdit(int displayedValue)
{
    const auto draft = valueEditSession.finish();
    const auto kind = editedKind();
    if (!kind || !hasActiveTrack())
        return;
    const auto& events = sequence->getTrack(activeTrackIndex).getEvents();
    if (draft.eventIndex < 0 || draft.eventIndex >= static_cast<int>(events.size()) ||
        !kind->matches(events[static_cast<size_t>(draft.eventIndex)]))
        return;

    const int value = kind->type == MidiEvent::Type::PitchBend
                          ? ControllerLaneScale::pitchBendFromDisplayed(displayedValue)
                          : displayedValue;
    const auto result = ControllerPointEdits::setValue(events, *kind, draft.eventIndex, value);
    if (result.events != events)
        applyPointEdit("Edit Controller Value", events, result);
}

void ControllerLaneComponent::openProgramBrowser(int tick, int anchorX)
{
    if (!hasActiveTrack())
        return;

    const auto& events = sequence->getTrack(activeTrackIndex).getEvents();
    const int program = ProgramChangeEdits::programAt(events, tick);
    const auto anchor = juce::Rectangle<int>(anchorX, getDrawAreaTop(), 1, getDrawAreaHeight());

    valueEditSession.close();
    programEditSession.close();

    auto content = std::make_unique<ProgramBrowser>(program);
    content->onCommit = [this](int selectedProgram) { commitProgram(selectedProgram); };
    content->onCancel = [this]() { programEditSession.finish(); };
    programEditSession.open({activeTrackIndex, tick, events}, std::move(content), localAreaToGlobal(anchor));
}

void ControllerLaneComponent::commitProgram(int program)
{
    const auto draft = programEditSession.finish();
    if (!showsProgramChange() || !hasActiveTrack() || draft.trackIndex != activeTrackIndex)
        return;

    const auto& events = sequence->getTrack(activeTrackIndex).getEvents();
    if (events != draft.eventsAtOpen)
        return;

    const bool changesExisting = !ProgramChangeEdits::onTick(draft.eventsAtOpen, draft.tick).empty();
    const auto result = ProgramChangeEdits::place(events, draft.tick, program);
    if (result.events == events)
        selectPoints(result.selected);
    else
        applyPointEdit(changesExisting ? "Change Program" : "Add Program Change", events, result);
}

void ControllerLaneComponent::mouseMove(const juce::MouseEvent& e)
{
    if (hitTestPoint(e.x, e.y) >= 0)
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    else if (editToolSelected == e.mods.isCommandDown() && velocityTopAt(e.x, e.y).isValid())
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    else
        setMouseCursor(juce::MouseCursor::NormalCursor);
}

bool ControllerLaneComponent::hasPointSelection() const
{
    return editedKind() && hasActiveTrack() && !pointSelection.isEmpty();
}

bool ControllerLaneComponent::canSelectAllPoints() const
{
    const auto kind = editedKind();
    return kind && hasActiveTrack() &&
           !ControllerPointEdits::allPoints(sequence->getTrack(activeTrackIndex).getEvents(), *kind).empty();
}

void ControllerLaneComponent::showAndSelectPoints(const ControllerKind& kind, std::set<int> eventIndices)
{
    if (kind.type == MidiEvent::Type::ProgramChange)
    {
        if (!showsProgramChange())
            setDisplayMode(DisplayMode::ProgramChange);
    }
    else if (kind.type == MidiEvent::Type::PitchBend)
    {
        if (displayMode != DisplayMode::PitchBend)
            setDisplayMode(DisplayMode::PitchBend);
    }
    else if (displayMode != DisplayMode::ControlChange || ccNumber != kind.number)
    {
        setCCNumber(kind.number);
        setDisplayMode(DisplayMode::ControlChange);
    }
    selectPoints(std::move(eventIndices));
}

void ControllerLaneComponent::selectAllPoints()
{
    const auto kind = editedKind();
    if (!kind || !hasActiveTrack())
        return;
    selectPoints(ControllerPointEdits::allPoints(sequence->getTrack(activeTrackIndex).getEvents(), *kind));
}

void ControllerLaneComponent::copySelectedPoints()
{
    if (!hasPointSelection())
        return;
    const auto& events = sequence->getTrack(activeTrackIndex).getEvents();
    clipboard.setControllerPoints(ControllerPointEdits::copy(events, *editedKind(), pointSelection.indices()));
}

void ControllerLaneComponent::cutSelectedPoints()
{
    if (!hasPointSelection())
        return;
    copySelectedPoints();
    const auto& events = sequence->getTrack(activeTrackIndex).getEvents();
    applyPointEdit(showsProgramChange() ? "Cut Program Changes" : "Cut Controller Points", events,
                   ControllerPointEdits::remove(events, pointSelection.indices()));
}

bool ControllerLaneComponent::canPastePoints() const
{
    const auto kind = editedKind();
    return clipboard.hasControllerPoints() && kind && hasActiveTrack() &&
           kind->acceptsPointsOf(clipboard.getControllerPoints().kind);
}

void ControllerLaneComponent::pastePoints(int atTick)
{
    programEditSession.close();
    if (!canPastePoints())
        return;
    valueEditSession.close();
    const auto& events = sequence->getTrack(activeTrackIndex).getEvents();
    const auto result =
        ControllerPointEdits::paste(events, *editedKind(), clipboard.getControllerPoints().points, atTick);
    if (result.events != events)
        applyPointEdit(showsProgramChange() ? "Paste Program Changes" : "Paste Controller Points", events, result);
    else
        selectPoints(result.selected);
}

bool ControllerLaneComponent::keyPressed(const juce::KeyPress& key)
{
    if (key != juce::KeyPress::deleteKey && key != juce::KeyPress::backspaceKey)
        return false;
    if (!editedKind() || !hasActiveTrack() || pointSelection.isEmpty())
        return false;
    const auto& events = sequence->getTrack(activeTrackIndex).getEvents();
    applyPointEdit(showsProgramChange() ? "Delete Program Changes" : "Delete Controller Points", events,
                   ControllerPointEdits::remove(events, pointSelection.indices()));
    return true;
}
