#include "ui/pianoroll/strips/TimeSignatureStrip.h"
#include "edit/TimeSignatureEdits.h"
#include "ui/theme/Theme.h"
#include "undo/ReplaceListAction.h"
#include <algorithm>
#include <memory>
#include <utility>

TimeSignatureStrip::TimeSignatureStrip(const TimelineGeometry& geometryRef, EditClipboard& clipboardRef,
                                       juce::UndoManager& undoManagerRef)
    : TimelineStrip(geometryRef, "Time Sig"), clipboard(clipboardRef), undoManager(undoManagerRef)
{
}

void TimeSignatureStrip::setSequence(MidiSequence* seq)
{
    editSession.close();
    clearTimeSignatureSelection();
    isTimeSigRangeSelecting = false;
    TimelineStrip::setSequence(seq);
}

bool TimeSignatureStrip::hasSelection() const
{
    return !selection.isEmpty();
}

void TimeSignatureStrip::clearTimeSignatureSelection()
{
    if (selection.isEmpty())
        return;
    selection.clear();
    repaint();
}

void TimeSignatureStrip::deleteSelectedTimeSignatures()
{
    deleteSelectedTimeSignaturesImpl("Delete Time Signature Changes");
}

void TimeSignatureStrip::deleteSelectedTimeSignaturesImpl(const juce::String& transactionName)
{
    if (!sequence || selection.isEmpty())
        return;

    auto before = sequence->getTimeline().getTimeSignatureChanges();
    auto after =
        TimeSignatureEdits::afterDelete(before, selection.indices(), sequence->getTimeline().getTicksPerQuarterNote());
    if (after.size() == before.size())
        return;

    performReplaceList(undoManager, sequence, transactionName, std::move(before), std::move(after));

    clearTimeSignatureSelection();
    repaint();
}

void TimeSignatureStrip::copySelectedTimeSignatures()
{
    if (!sequence || selection.isEmpty())
        return;

    const auto& changes = sequence->getTimeline().getTimeSignatureChanges();
    const int count = static_cast<int>(changes.size());

    std::vector<RelativeTimeSignature> items;
    for (int i : selection.indices())
        if (i >= 0 && i < count)
            items.push_back({sequence->getTimeline().tickToBarBeatTick(changes[i].tick).bar, changes[i].numerator,
                             changes[i].denominator});

    if (items.empty())
        return;

    const int firstBar = items.front().barOffset;
    for (auto& item : items)
        item.barOffset -= firstBar;

    clipboard.setTimeSignatures(std::move(items));
}

void TimeSignatureStrip::cutSelectedTimeSignatures()
{
    if (!sequence || selection.isEmpty())
        return;

    copySelectedTimeSignatures();
    deleteSelectedTimeSignaturesImpl("Cut Time Signature Changes");
}

void TimeSignatureStrip::pasteTimeSignatures(int atTick)
{
    if (!sequence || !clipboard.hasTimeSignatures())
        return;

    const int anchorBar = sequence->getTimeline().tickToBarBeatTick(std::max(0, atTick)).bar;
    auto before = sequence->getTimeline().getTimeSignatureChanges();
    auto after = TimeSignatureEdits::afterPaste(before, clipboard.getTimeSignatures(), anchorBar,
                                                sequence->getTimeline().getTicksPerQuarterNote());

    std::vector<int> pastedBars;
    for (const auto& item : clipboard.getTimeSignatures())
        pastedBars.push_back(anchorBar + item.barOffset);

    const bool changed = (after != before);
    if (changed)
    {
        performReplaceList(undoManager, sequence, "Paste Time Signature Changes", std::move(before), after);
    }

    std::vector<int> pastedTicks;
    for (int b : pastedBars)
        pastedTicks.push_back(sequence->getTimeline().barStartToTick(b));
    selection.selectTicks(sequence->getTimeline().getTimeSignatureChanges(), pastedTicks);

    repaint();
}

juce::Rectangle<int> TimeSignatureStrip::timeSignatureLabelRect(int index) const
{
    const auto& changes = sequence->getTimeline().getTimeSignatureChanges();
    const auto& ts = changes[static_cast<size_t>(index)];
    int x = geometry.tickToX(ts.tick);
    int textX = (index == 0 && ts.tick == 0) ? viewLeftX + labelWidth() + 4 : x + 4;
    return {textX, 0, 40, getHeight()};
}

int TimeSignatureStrip::hitTestTimeSignaturePoint(int x, int y) const
{
    if (!sequence)
        return -1;

    if (y < 0 || y >= getHeight())
        return -1;

    if (x < viewLeftX + labelWidth())
        return -1;

    const auto& changes = sequence->getTimeline().getTimeSignatureChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
    {
        if (geometry.tickToX(changes[static_cast<size_t>(i)].tick) + 4 < viewLeftX - 40)
            continue;
        if (timeSignatureLabelRect(i).contains(x, y))
            return i;
    }
    return -1;
}

void TimeSignatureStrip::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (!sequence)
        return;

    auto clip = g.getClipBounds();
    int visibleLeft = clip.getX();
    int visibleRight = clip.getRight();

    g.setColour(surface::surface2);
    g.fillRect(viewLeftX, 0, getWidth() - viewLeftX, getHeight());

    g.saveState();
    g.reduceClipRegion(viewLeftX + labelWidth(), 0, getWidth(), getHeight());

    drawTrackGridLines(g, visibleLeft, visibleRight, 0.0f, static_cast<float>(getHeight()));
    const auto* draft = editSession.current();

    const auto& tsChanges = sequence->getTimeline().getTimeSignatureChanges();
    if (tsChanges.empty())
    {
        g.setColour(track::teal);
        g.setFont(font::sans(font::sizeSM));
        g.drawText("4/4", viewLeftX + labelWidth() + 4, 0, 40, getHeight(), juce::Justification::centredLeft);
    }
    else
    {
        juce::Colour tsColour = track::teal;

        for (size_t i = 0; i < tsChanges.size(); ++i)
        {
            int x = geometry.tickToX(tsChanges[i].tick);

            if (x > visibleRight)
                break;

            int nextX = (i + 1 < tsChanges.size()) ? geometry.tickToX(tsChanges[i + 1].tick)
                                                   : geometry.tickToX(geometry.xToTick(getWidth()));
            if (nextX < visibleLeft)
                continue;

            if (i > 0 && x >= visibleLeft && x <= visibleRight)
            {
                g.setColour(tsColour.withAlpha(0.6f));
                g.drawVerticalLine(x, 2.0f, static_cast<float>(getHeight() - 2));
            }

            if (x + 4 >= visibleLeft - 40 && x <= visibleRight)
            {
                bool selected = selection.contains(static_cast<int>(i));
                bool editingThis = draft != nullptr && !draft->isNew && tsChanges[i].tick == draft->tick;
                auto labelRect = timeSignatureLabelRect(static_cast<int>(i));
                if (selected || editingThis)
                {
                    g.setColour(surface::selection);
                    g.fillRoundedRectangle(labelRect.reduced(0, 2).toFloat(), radius::r1);
                }
                g.setColour(selected || editingThis ? tsColour.brighter(0.5f) : tsColour);
                g.setFont(font::sans(font::sizeSM));
                int labelNum = editingThis ? draft->numerator : tsChanges[i].numerator;
                int labelDen = editingThis ? draft->denominator : tsChanges[i].denominator;
                juce::String labelText = juce::String(labelNum) + "/" + juce::String(labelDen);
                g.drawText(labelText, labelRect, juce::Justification::centredLeft);
            }
        }
    }

    if (draft != nullptr && draft->isNew)
    {
        juce::Colour draftColour = track::teal.withAlpha(0.6f);
        int x = geometry.tickToX(draft->tick);
        if (x >= visibleLeft && x <= visibleRight)
        {
            g.setColour(draftColour.withAlpha(0.4f));
            g.drawVerticalLine(x, 2.0f, static_cast<float>(getHeight() - 2));
        }
        int textX = std::max(x + 4, viewLeftX + labelWidth() + 4);
        g.setColour(draftColour);
        g.setFont(font::sans(font::sizeSM));
        juce::String labelText = juce::String(draft->numerator) + "/" + juce::String(draft->denominator);
        g.drawText(labelText, textX, 0, 40, getHeight(), juce::Justification::centredLeft);
    }

    float phX = playheadX();
    if (phX >= static_cast<float>(visibleLeft) - 1.0f && phX <= static_cast<float>(visibleRight) + 1.0f)
    {
        g.setColour(text::t1);
        g.drawLine(phX, 0.0f, phX, static_cast<float>(getHeight()), 1.0f);
    }

    if (isTimeSigRangeSelecting)
        drawRangeBand(g, rangeSelect, track::teal.withAlpha(0.15f), track::teal.withAlpha(0.6f));

    drawLoopOverlay(g, 0, getHeight(), 0.12f);

    g.restoreState();

    drawLabelColumn(g);
}

void TimeSignatureStrip::mouseDown(const juce::MouseEvent& e)
{
    if (sequence == nullptr || sequence->getNumTracks() == 0)
        return;
    if (e.mods.isRightButtonDown())
        return;

    int tsIndex = hitTestTimeSignaturePoint(e.x, e.y);
    if (tsIndex >= 0)
    {
        if (e.mods.isShiftDown())
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selection.toggle(tsIndex);
            repaint();
            return;
        }

        timeSigDragBefore = sequence->getTimeline().getTimeSignatureChanges();
        timeSigDragIndex = tsIndex;
        isTimeSigPointDragging = true;
        timeSigDragMoved = false;
        timeSigDragGrabOffset = geometry.xToTick(e.x) - timeSigDragBefore[static_cast<size_t>(tsIndex)].tick;

        timeSigDragGroup = selection.dragGroup(tsIndex);
        return;
    }

    if (e.y >= 0 && e.y < getHeight() && e.x >= viewLeftX + labelWidth())
    {
        if (onSelectionTaken)
            onSelectionTaken();
        isTimeSigRangeSelecting = true;
        rangeSelect = {e.x, e.x, e.mods.isShiftDown() ? selection.indices() : std::set<int>{}};
        if (!e.mods.isShiftDown())
            selection.clear();
        repaint();
        return;
    }
}

void TimeSignatureStrip::mouseDrag(const juce::MouseEvent& e)
{
    if (sequence == nullptr)
        return;

    if (isTimeSigPointDragging)
    {
        if (timeSigDragIndex < 0 || timeSigDragIndex >= static_cast<int>(timeSigDragBefore.size()))
            return;

        auto changes = TimeSignatureEdits::afterMove(timeSigDragBefore, timeSigDragGroup, timeSigDragIndex,
                                                     geometry.xToTick(e.x) - timeSigDragGrabOffset,
                                                     sequence->getTimeline().getTicksPerQuarterNote());
        if (changes[static_cast<size_t>(timeSigDragIndex)].tick !=
            timeSigDragBefore[static_cast<size_t>(timeSigDragIndex)].tick)
            timeSigDragMoved = true;
        sequence->setTimeSignatureChanges(std::move(changes));
        repaint();
        if (onTimelineMetadataChanged)
            onTimelineMetadataChanged();
        return;
    }

    if (isTimeSigRangeSelecting)
    {
        rangeSelect.currentX = e.x;
        const auto& changes = sequence->getTimeline().getTimeSignatureChanges();
        selection.assign(rangeSelect.selectionFor(static_cast<int>(changes.size()), geometry,
                                                  [&changes](int i, int tickLo, int tickHi)
                                                  {
                                                      const int tick = changes[static_cast<size_t>(i)].tick;
                                                      return tick >= tickLo && tick <= tickHi;
                                                  }));

        repaint();
        return;
    }
}

void TimeSignatureStrip::mouseUp(const juce::MouseEvent&)
{
    if (isTimeSigPointDragging)
    {
        isTimeSigPointDragging = false;
        int draggedIndex = timeSigDragIndex;
        timeSigDragIndex = -1;

        const auto& changes = sequence->getTimeline().getTimeSignatureChanges();
        bool validIndex = draggedIndex >= 0 && draggedIndex < static_cast<int>(changes.size()) &&
                          draggedIndex < static_cast<int>(timeSigDragBefore.size());
        bool movedFinal = validIndex && changes[static_cast<size_t>(draggedIndex)].tick !=
                                            timeSigDragBefore[static_cast<size_t>(draggedIndex)].tick;

        if (movedFinal)
        {
            performReplaceList(undoManager, sequence, "Move Time Signature Change", timeSigDragBefore, changes);
            if (onSelectionTaken)
                onSelectionTaken();
            selection.assign(std::set<int>(timeSigDragGroup.begin(), timeSigDragGroup.end()));
        }
        else if (validIndex && !timeSigDragMoved)
        {
            const bool soleSelection = selection.isSole(draggedIndex);
            if (soleSelection && !editSession.isOpen())
            {
                const auto& ts = changes[static_cast<size_t>(draggedIndex)];
                openTimeSignatureEditor(ts.tick, ts.numerator, ts.denominator, false,
                                        timeSignatureLabelRect(draggedIndex));
            }
            else if (!soleSelection)
            {
                if (onSelectionTaken)
                    onSelectionTaken();
                selection.selectOnly(draggedIndex);
            }
        }
        else if (validIndex)
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selection.selectOnly(draggedIndex);
        }

        timeSigDragBefore.clear();
        timeSigDragGroup.clear();
        timeSigDragMoved = false;
        repaint();
        return;
    }

    if (isTimeSigRangeSelecting)
    {
        isTimeSigRangeSelecting = false;
        rangeSelect = {};
        repaint();
        return;
    }
}

void TimeSignatureStrip::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!sequence || e.mods.isRightButtonDown() || editSession.isOpen())
        return;

    if (e.y < 0 || e.y >= getHeight() || e.x < viewLeftX + labelWidth())
        return;

    if (hitTestTimeSignaturePoint(e.x, e.y) >= 0)
        return;

    isTimeSigRangeSelecting = false;
    rangeSelect = {};

    int barStart = sequence->getTimeline().barStartToTick(
        sequence->getTimeline().tickToBarBeatTick(std::max(0, geometry.xToTick(e.x))).bar);

    const auto& changes = sequence->getTimeline().getTimeSignatureChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
    {
        if (changes[i].tick == barStart)
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selection.selectOnly(i);
            repaint();
            openTimeSignatureEditor(changes[static_cast<size_t>(i)].tick, changes[static_cast<size_t>(i)].numerator,
                                    changes[static_cast<size_t>(i)].denominator, false, timeSignatureLabelRect(i));
            return;
        }
    }

    if (onSelectionTaken)
        onSelectionTaken();
    clearTimeSignatureSelection();
    auto effective = sequence->getTimeline().getTimeSignatureAt(barStart);
    juce::Rectangle<int> anchor{geometry.tickToX(barStart), 0, 40, getHeight()};
    openTimeSignatureEditor(barStart, effective.numerator, effective.denominator, true, anchor);
}

void TimeSignatureStrip::openTimeSignatureEditor(int tick, int num, int den, bool isNew,
                                                 juce::Rectangle<int> anchorInLocal)
{
    anchorInLocal.setX(std::max(anchorInLocal.getX(), viewLeftX + labelWidth()));

    auto content = std::make_unique<TimeSignatureEditor>(num, den, isNew);
    content->onDraftChanged = [this](int n, int d)
    {
        if (auto* draft = editSession.current())
        {
            draft->numerator = n;
            draft->denominator = d;
        }
        repaint();
    };
    content->onCommit = [this](int n, int d) { commitTimeSignatureEdit(n, d); };
    content->onCancel = [this]() { cancelTimeSignatureEdit(); };

    editSession.open({tick, num, den, isNew}, std::move(content), localAreaToGlobal(anchorInLocal));
    repaint();
}

void TimeSignatureStrip::commitTimeSignatureEdit(int num, int den)
{
    const auto draft = editSession.finish();

    if (!sequence)
        return;

    const auto& existing = sequence->getTimeline().getTimeSignatureChanges();
    auto atTick = std::ranges::find(existing, draft.tick, &TimeSignatureChange::tick);
    if (atTick != existing.end() && num == atTick->numerator && den == atTick->denominator)
    {
        repaint();
        return;
    }

    auto before = sequence->getTimeline().getTimeSignatureChanges();
    auto after = before;
    TimeSignatureEdits::add(after, draft.tick, num, den, sequence->getTimeline().getTicksPerQuarterNote());
    performReplaceList(undoManager, sequence, draft.isNew ? "Add Time Signature Change" : "Edit Time Signature Change",
                       std::move(before), std::move(after));

    const auto& changes = sequence->getTimeline().getTimeSignatureChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
        if (changes[i].tick == draft.tick)
            selection.selectOnly(i);
    repaint();
}

void TimeSignatureStrip::cancelTimeSignatureEdit()
{
    editSession.finish();
    repaint();
}
