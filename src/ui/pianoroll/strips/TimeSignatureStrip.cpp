#include "ui/pianoroll/strips/TimeSignatureStrip.h"
#include "ui/theme/Theme.h"
#include "undo/TimeSignatureActions.h"
#include <algorithm>
#include <memory>

TimeSignatureStrip::TimeSignatureStrip(const TimelineGeometry& geometryRef, EditClipboard& clipboardRef)
    : TimelineStrip(geometryRef, "Time Sig"), clipboard(clipboardRef)
{
}

TimeSignatureStrip::~TimeSignatureStrip()
{
    closeTimeSignatureEditor();
}

void TimeSignatureStrip::setUndoManager(juce::UndoManager* um)
{
    undoManager = um;
}

void TimeSignatureStrip::setSequence(MidiSequence* seq)
{
    closeTimeSignatureEditor();
    clearTimeSignatureSelection();
    isTimeSigRangeSelecting = false;
    TimelineStrip::setSequence(seq);
}

bool TimeSignatureStrip::hasSelection() const
{
    return !selectedTimeSigIndices.empty();
}

void TimeSignatureStrip::clearTimeSignatureSelection()
{
    if (selectedTimeSigIndices.empty())
        return;
    selectedTimeSigIndices.clear();
    repaint();
}

void TimeSignatureStrip::deleteSelectedTimeSignatures()
{
    deleteSelectedTimeSignaturesImpl("Delete Time Signature Changes");
}

void TimeSignatureStrip::deleteSelectedTimeSignaturesImpl(const juce::String& transactionName)
{
    if (!sequence || selectedTimeSigIndices.empty())
        return;

    auto before = sequence->getTimeSignatureChanges();
    auto after = MidiSequence::buildTimeSignatureChangesAfterDelete(before, selectedTimeSigIndices,
                                                                    sequence->getTicksPerQuarterNote());
    if (after.size() == before.size())
        return;

    if (undoManager)
    {
        undoManager->beginNewTransaction(transactionName);
        undoManager->perform(new TimeSignatureDeleteAction(sequence, std::move(before), std::move(after)));
    }
    else
    {
        sequence->setTimeSignatureChanges(std::move(after));
        sequence->notifyTimelineMetadataChanged();
    }

    clearTimeSignatureSelection();
    repaint();
}

void TimeSignatureStrip::copySelectedTimeSignatures()
{
    if (!sequence || selectedTimeSigIndices.empty())
        return;

    const auto& changes = sequence->getTimeSignatureChanges();
    const int count = static_cast<int>(changes.size());

    std::vector<RelativeTimeSignature> items;
    for (int i : selectedTimeSigIndices)
        if (i >= 0 && i < count)
            items.push_back(
                {sequence->tickToBarBeatTick(changes[i].tick).bar, changes[i].numerator, changes[i].denominator});

    if (items.empty())
        return;

    const int firstBar = items.front().barOffset;
    for (auto& item : items)
        item.barOffset -= firstBar;

    clipboard.setTimeSignatures(std::move(items));
}

void TimeSignatureStrip::cutSelectedTimeSignatures()
{
    if (!sequence || selectedTimeSigIndices.empty())
        return;

    copySelectedTimeSignatures();
    deleteSelectedTimeSignaturesImpl("Cut Time Signature Changes");
}

void TimeSignatureStrip::pasteTimeSignatures(int atTick)
{
    if (!sequence || !clipboard.hasTimeSignatures())
        return;

    const int anchorBar = sequence->tickToBarBeatTick(std::max(0, atTick)).bar;
    auto before = sequence->getTimeSignatureChanges();
    auto after = MidiSequence::buildTimeSignatureChangesAfterPaste(before, clipboard.getTimeSignatures(), anchorBar,
                                                                   sequence->getTicksPerQuarterNote());

    std::vector<int> pastedBars;
    for (const auto& item : clipboard.getTimeSignatures())
        pastedBars.push_back(anchorBar + item.barOffset);

    const bool changed = (after != before);
    if (changed)
    {
        if (undoManager)
        {
            undoManager->beginNewTransaction("Paste Time Signature Changes");
            undoManager->perform(new TimeSignaturePasteAction(sequence, std::move(before), after));
        }
        else
        {
            sequence->setTimeSignatureChanges(after);
            sequence->notifyTimelineMetadataChanged();
        }
    }

    selectedTimeSigIndices.clear();
    const auto& changes = sequence->getTimeSignatureChanges();
    for (int b : pastedBars)
    {
        const int t = sequence->barStartToTick(b);
        auto it = std::ranges::find(changes, t, &TimeSignatureChange::tick);
        if (it != changes.end())
            selectedTimeSigIndices.insert(static_cast<int>(it - changes.begin()));
    }

    repaint();
}

juce::Rectangle<int> TimeSignatureStrip::timeSignatureLabelRect(int index) const
{
    const auto& changes = sequence->getTimeSignatureChanges();
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

    const auto& changes = sequence->getTimeSignatureChanges();
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

    const auto& tsChanges = sequence->getTimeSignatureChanges();
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
                bool selected = selectedTimeSigIndices.count(static_cast<int>(i)) > 0;
                bool editingThis = isTimeSigEditing && !timeSigEditIsNew && tsChanges[i].tick == timeSigEditTick;
                auto labelRect = timeSignatureLabelRect(static_cast<int>(i));
                if (selected || editingThis)
                {
                    g.setColour(surface::selection);
                    g.fillRoundedRectangle(labelRect.reduced(0, 2).toFloat(), radius::r1);
                }
                g.setColour(selected || editingThis ? tsColour.brighter(0.5f) : tsColour);
                g.setFont(font::sans(font::sizeSM));
                int labelNum = editingThis ? timeSigDraftNum : tsChanges[i].numerator;
                int labelDen = editingThis ? timeSigDraftDen : tsChanges[i].denominator;
                juce::String labelText = juce::String(labelNum) + "/" + juce::String(labelDen);
                g.drawText(labelText, labelRect, juce::Justification::centredLeft);
            }
        }
    }

    if (isTimeSigEditing && timeSigEditIsNew)
    {
        juce::Colour draftColour = track::teal.withAlpha(0.6f);
        int x = geometry.tickToX(timeSigEditTick);
        if (x >= visibleLeft && x <= visibleRight)
        {
            g.setColour(draftColour.withAlpha(0.4f));
            g.drawVerticalLine(x, 2.0f, static_cast<float>(getHeight() - 2));
        }
        int textX = std::max(x + 4, viewLeftX + labelWidth() + 4);
        g.setColour(draftColour);
        g.setFont(font::sans(font::sizeSM));
        juce::String labelText = juce::String(timeSigDraftNum) + "/" + juce::String(timeSigDraftDen);
        g.drawText(labelText, textX, 0, 40, getHeight(), juce::Justification::centredLeft);
    }

    float phX = playheadX();
    if (phX >= static_cast<float>(visibleLeft) - 1.0f && phX <= static_cast<float>(visibleRight) + 1.0f)
    {
        g.setColour(text::t1);
        g.drawLine(phX, 0.0f, phX, static_cast<float>(getHeight()), 1.0f);
    }

    drawTimeSignatureRangeSelection(g);

    drawLoopOverlay(g, 0, getHeight(), 0.12f);

    g.restoreState();

    drawLabelColumn(g);
}

void TimeSignatureStrip::drawTimeSignatureRangeSelection(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (!isTimeSigRangeSelecting)
        return;

    int lo = std::min(timeSigSelectStartX, timeSigSelectCurrentX);
    int hi = std::max(timeSigSelectStartX, timeSigSelectCurrentX);
    if (hi <= lo)
        return;

    juce::Rectangle<float> band(static_cast<float>(lo), 0.0f, static_cast<float>(hi - lo),
                                static_cast<float>(getHeight()));
    g.setColour(track::teal.withAlpha(0.15f));
    g.fillRect(band);
    g.setColour(track::teal.withAlpha(0.6f));
    g.drawRect(band, 1.0f);
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
            if (selectedTimeSigIndices.count(tsIndex) > 0)
                selectedTimeSigIndices.erase(tsIndex);
            else
                selectedTimeSigIndices.insert(tsIndex);
            repaint();
            return;
        }

        timeSigDragBefore = sequence->getTimeSignatureChanges();
        timeSigDragIndex = tsIndex;
        isTimeSigPointDragging = true;
        timeSigDragMoved = false;
        timeSigDragGrabOffset = geometry.xToTick(e.x) - timeSigDragBefore[static_cast<size_t>(tsIndex)].tick;

        if (selectedTimeSigIndices.count(tsIndex) > 0 && selectedTimeSigIndices.size() > 1)
            timeSigDragGroup.assign(selectedTimeSigIndices.begin(), selectedTimeSigIndices.end());
        else
            timeSigDragGroup = {tsIndex};
        return;
    }

    if (e.y >= 0 && e.y < getHeight() && e.x >= viewLeftX + labelWidth())
    {
        if (onSelectionTaken)
            onSelectionTaken();
        isTimeSigRangeSelecting = true;
        timeSigSelectStartX = e.x;
        timeSigSelectCurrentX = e.x;
        timeSigSelectBase = e.mods.isShiftDown() ? selectedTimeSigIndices : std::set<int>{};
        if (!e.mods.isShiftDown())
            selectedTimeSigIndices.clear();
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

        auto changes = MidiSequence::buildTimeSignatureChangesAfterMove(
            timeSigDragBefore, timeSigDragGroup, timeSigDragIndex, geometry.xToTick(e.x) - timeSigDragGrabOffset,
            sequence->getTicksPerQuarterNote());
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
        timeSigSelectCurrentX = e.x;
        int lo = std::min(timeSigSelectStartX, timeSigSelectCurrentX);
        int hi = std::max(timeSigSelectStartX, timeSigSelectCurrentX);
        int tickLo = geometry.xToTick(lo);
        int tickHi = geometry.xToTick(hi);

        selectedTimeSigIndices = timeSigSelectBase;
        const auto& changes = sequence->getTimeSignatureChanges();
        for (int i = 0; i < static_cast<int>(changes.size()); ++i)
            if (changes[static_cast<size_t>(i)].tick >= tickLo && changes[static_cast<size_t>(i)].tick <= tickHi)
                selectedTimeSigIndices.insert(i);

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

        const auto& changes = sequence->getTimeSignatureChanges();
        bool validIndex = draggedIndex >= 0 && draggedIndex < static_cast<int>(changes.size()) &&
                          draggedIndex < static_cast<int>(timeSigDragBefore.size());
        bool movedFinal = validIndex && changes[static_cast<size_t>(draggedIndex)].tick !=
                                            timeSigDragBefore[static_cast<size_t>(draggedIndex)].tick;

        if (movedFinal)
        {
            if (undoManager)
            {
                undoManager->beginNewTransaction("Move Time Signature Change");
                undoManager->perform(new TimeSignatureMoveAction(sequence, timeSigDragBefore, changes));
            }
            else
            {
                sequence->notifyTimelineMetadataChanged();
            }
            if (onSelectionTaken)
                onSelectionTaken();
            selectedTimeSigIndices = std::set<int>(timeSigDragGroup.begin(), timeSigDragGroup.end());
        }
        else if (validIndex && !timeSigDragMoved)
        {
            const bool soleSelection =
                selectedTimeSigIndices.size() == 1 && selectedTimeSigIndices.count(draggedIndex) > 0;
            if (soleSelection && !isTimeSigEditing)
            {
                const auto& ts = changes[static_cast<size_t>(draggedIndex)];
                openTimeSignatureEditor(ts.tick, ts.numerator, ts.denominator, false,
                                        timeSignatureLabelRect(draggedIndex));
            }
            else if (!soleSelection)
            {
                if (onSelectionTaken)
                    onSelectionTaken();
                selectedTimeSigIndices = {draggedIndex};
            }
        }
        else if (validIndex)
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selectedTimeSigIndices = {draggedIndex};
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
        timeSigSelectBase.clear();
        repaint();
        return;
    }
}

void TimeSignatureStrip::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!sequence || e.mods.isRightButtonDown() || isTimeSigEditing)
        return;

    if (e.y < 0 || e.y >= getHeight() || e.x < viewLeftX + labelWidth())
        return;

    if (hitTestTimeSignaturePoint(e.x, e.y) >= 0)
        return;

    isTimeSigRangeSelecting = false;
    timeSigSelectBase.clear();

    int barStart = sequence->barStartToTick(sequence->tickToBarBeatTick(std::max(0, geometry.xToTick(e.x))).bar);

    const auto& changes = sequence->getTimeSignatureChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
    {
        if (changes[i].tick == barStart)
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selectedTimeSigIndices = {i};
            repaint();
            openTimeSignatureEditor(changes[static_cast<size_t>(i)].tick, changes[static_cast<size_t>(i)].numerator,
                                    changes[static_cast<size_t>(i)].denominator, false, timeSignatureLabelRect(i));
            return;
        }
    }

    if (onSelectionTaken)
        onSelectionTaken();
    clearTimeSignatureSelection();
    auto effective = sequence->getTimeSignatureAt(barStart);
    juce::Rectangle<int> anchor{geometry.tickToX(barStart), 0, 40, getHeight()};
    openTimeSignatureEditor(barStart, effective.numerator, effective.denominator, true, anchor);
}

void TimeSignatureStrip::openTimeSignatureEditor(int tick, int num, int den, bool isNew,
                                                 juce::Rectangle<int> anchorInLocal)
{
    anchorInLocal.setX(std::max(anchorInLocal.getX(), viewLeftX + labelWidth()));

    isTimeSigEditing = true;
    timeSigEditTick = tick;
    timeSigDraftNum = num;
    timeSigDraftDen = den;
    timeSigEditIsNew = isNew;

    auto content = std::make_unique<TimeSignatureEditor>(num, den, isNew);
    timeSigEditor = content.get();
    content->onDraftChanged = [this](int n, int d)
    {
        timeSigDraftNum = n;
        timeSigDraftDen = d;
        repaint();
    };
    content->onCommit = [this](int n, int d) { commitTimeSignatureEdit(n, d); };
    content->onCancel = [this]() { cancelTimeSignatureEdit(); };

    auto& box = juce::CallOutBox::launchAsynchronously(std::move(content), localAreaToGlobal(anchorInLocal), nullptr);
    box.setDismissalMouseClicksAreAlwaysConsumed(true);
    timeSigCallout = &box;
    repaint();
}

void TimeSignatureStrip::commitTimeSignatureEdit(int num, int den)
{
    isTimeSigEditing = false;
    timeSigEditor = nullptr;
    timeSigCallout = nullptr;

    if (!sequence)
        return;

    const auto& existing = sequence->getTimeSignatureChanges();
    auto atTick = std::ranges::find(existing, timeSigEditTick, &TimeSignatureChange::tick);
    if (atTick != existing.end() && num == atTick->numerator && den == atTick->denominator)
    {
        repaint();
        return;
    }

    if (undoManager)
    {
        undoManager->beginNewTransaction(timeSigEditIsNew ? "Add Time Signature Change" : "Edit Time Signature Change");
        undoManager->perform(new TimeSignatureChangeAction(sequence, timeSigEditTick, num, den));
    }
    else
    {
        sequence->addTimeSignatureChange(timeSigEditTick, num, den);
        sequence->notifyTimelineMetadataChanged();
    }

    const auto& changes = sequence->getTimeSignatureChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
        if (changes[i].tick == timeSigEditTick)
            selectedTimeSigIndices = {i};
    repaint();
}

void TimeSignatureStrip::cancelTimeSignatureEdit()
{
    isTimeSigEditing = false;
    timeSigEditor = nullptr;
    timeSigCallout = nullptr;
    repaint();
}

void TimeSignatureStrip::closeTimeSignatureEditor()
{
    if (timeSigEditor != nullptr)
        timeSigEditor->abandon();
    if (timeSigCallout != nullptr)
        timeSigCallout->dismiss();

    isTimeSigEditing = false;
    timeSigEditor = nullptr;
    timeSigCallout = nullptr;
}
