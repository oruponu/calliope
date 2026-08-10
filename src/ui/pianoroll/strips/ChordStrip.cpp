#include "ui/pianoroll/strips/ChordStrip.h"
#include "ui/theme/Theme.h"
#include "undo/ChordActions.h"
#include <algorithm>
#include <cstdlib>
#include <memory>

ChordStrip::ChordStrip(const TimelineGeometry& geometryRef, EditClipboard& clipboardRef)
    : TimelineStrip(geometryRef, "Chord"), clipboard(clipboardRef)
{
}

ChordStrip::~ChordStrip()
{
    closeChordEditor();
}

void ChordStrip::setUndoManager(juce::UndoManager* um)
{
    undoManager = um;
}

void ChordStrip::setSequence(MidiSequence* seq)
{
    closeChordEditor();
    clearChordSelection();
    isChordRangeSelecting = false;
    chordSelectBase.clear();
    chordRangeToggleIndex = -1;
    TimelineStrip::setSequence(seq);
}

void ChordStrip::clearChordSelection()
{
    if (selectedChordIndices.empty())
        return;
    selectedChordIndices.clear();
    repaint();
}

bool ChordStrip::hasSelection() const
{
    return !selectedChordIndices.empty();
}

void ChordStrip::deleteSelectedChords()
{
    deleteSelectedChordsImpl("Delete Chords");
}

void ChordStrip::deleteSelectedChordsImpl(const juce::String& transactionName)
{
    if (!sequence || selectedChordIndices.empty())
        return;

    auto before = sequence->getChordChanges();
    auto after = MidiSequence::buildChordChangesAfterDelete(
        before, std::vector<int>(selectedChordIndices.begin(), selectedChordIndices.end()));
    if (after == before)
        return;

    if (undoManager)
    {
        undoManager->beginNewTransaction(transactionName);
        undoManager->perform(new ChordDeleteAction(sequence, std::move(before), std::move(after)));
    }
    else
    {
        sequence->setChordChanges(std::move(after));
        sequence->notifyTimelineMetadataChanged();
    }

    clearChordSelection();
    repaint();
}

void ChordStrip::copySelectedChords()
{
    if (!sequence || selectedChordIndices.empty())
        return;

    const auto& changes = sequence->getChordChanges();
    const int count = static_cast<int>(changes.size());

    std::vector<RelativeChord> items;
    int baseTick = -1;
    for (int i : selectedChordIndices)
    {
        if (i < 0 || i >= count || MidiSequence::chordToString(changes[static_cast<size_t>(i)]).empty())
            continue;
        const auto& cc = changes[static_cast<size_t>(i)];
        if (baseTick < 0)
            baseTick = cc.tick;
        const int length = (i + 1 < count) ? changes[static_cast<size_t>(i) + 1].tick - cc.tick : 0;
        items.push_back({cc.tick - baseTick, length, cc.chordRoot, cc.chordType, cc.bassRoot, cc.bassType});
    }

    if (items.empty())
        return;

    clipboard.setChords(std::move(items));
}

void ChordStrip::cutSelectedChords()
{
    if (!sequence || selectedChordIndices.empty())
        return;

    copySelectedChords();
    deleteSelectedChordsImpl("Cut Chords");
}

void ChordStrip::pasteChords(int atTick)
{
    if (!sequence || !clipboard.hasChords())
        return;

    const int anchorTick = geometry.floorTickToGrid(std::max(0, atTick));
    auto before = sequence->getChordChanges();
    auto after = MidiSequence::buildChordChangesAfterPaste(before, clipboard.getChords(), anchorTick);

    const bool changed = (after != before);
    if (changed)
    {
        if (undoManager)
        {
            undoManager->beginNewTransaction("Paste Chords");
            undoManager->perform(new ChordPasteAction(sequence, std::move(before), after));
        }
        else
        {
            sequence->setChordChanges(after);
            sequence->notifyTimelineMetadataChanged();
        }
    }

    selectedChordIndices.clear();
    const auto& changes = sequence->getChordChanges();
    for (const auto& item : clipboard.getChords())
    {
        const int target = anchorTick + item.tickOffset;
        for (int i = 0; i < static_cast<int>(changes.size()); ++i)
            if (changes[static_cast<size_t>(i)].tick == target &&
                !MidiSequence::chordToString(changes[static_cast<size_t>(i)]).empty())
                selectedChordIndices.insert(i);
    }

    repaint();
}

juce::Rectangle<int> ChordStrip::chordSpanRect(int index) const
{
    if (!sequence)
        return {};

    const auto& changes = sequence->getChordChanges();
    if (index < 0 || index >= static_cast<int>(changes.size()))
        return {};
    if (MidiSequence::chordToString(changes[static_cast<size_t>(index)]).empty())
        return {};

    int x = geometry.tickToX(changes[static_cast<size_t>(index)].tick);
    int nextX = (index + 1 < static_cast<int>(changes.size()))
                    ? geometry.tickToX(changes[static_cast<size_t>(index + 1)].tick)
                    : geometry.tickToX(geometry.xToTick(getWidth()));
    return {x, spanTop, nextX - x, spanHeight()};
}

juce::Rectangle<int> ChordStrip::chordDraftSpanRect() const
{
    if (!sequence)
        return {};

    int x = geometry.tickToX(chordEditTick);
    int nextX = geometry.tickToX(chordEditEndTick);
    return {x, spanTop, nextX - x, spanHeight()};
}

int ChordStrip::hitTestChordSpan(int x, int y) const
{
    if (!sequence)
        return -1;

    if (x < viewLeftX + labelWidth())
        return -1;

    const auto& changes = sequence->getChordChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
    {
        auto spanRect = chordSpanRect(i);
        if (!spanRect.isEmpty() && spanRect.contains(x, y))
            return i;
    }
    return -1;
}

std::pair<int, ChordStrip::ResizeEdge> ChordStrip::hitTestChordEdge(int x, int y) const
{
    if (!sequence)
        return {-1, ResizeEdge::None};

    if (y < spanTop || y >= spanTop + spanHeight() || x < viewLeftX + labelWidth())
        return {-1, ResizeEdge::None};

    int containing = hitTestChordSpan(x, y);
    if (containing >= 0)
    {
        auto spanRect = chordSpanRect(containing);
        int rightDistance = spanRect.getRight() - x;
        int leftDistance = x - spanRect.getX();
        if (rightDistance <= resizeEdgeWidth && rightDistance <= leftDistance)
            return {containing, ResizeEdge::Right};
        if (leftDistance <= resizeEdgeWidth)
            return {containing, ResizeEdge::Left};
        return {-1, ResizeEdge::None};
    }

    const auto& changes = sequence->getChordChanges();
    int best = -1;
    ResizeEdge bestEdge = ResizeEdge::None;
    int bestDistance = resizeEdgeWidth + 1;
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
    {
        auto spanRect = chordSpanRect(i);
        if (spanRect.isEmpty())
            continue;
        int rightDistance = std::abs(x - spanRect.getRight());
        if (rightDistance < bestDistance)
        {
            bestDistance = rightDistance;
            best = i;
            bestEdge = ResizeEdge::Right;
        }
        int leftDistance = std::abs(x - spanRect.getX());
        if (leftDistance < bestDistance)
        {
            bestDistance = leftDistance;
            best = i;
            bestEdge = ResizeEdge::Left;
        }
    }
    return {best, bestEdge};
}

void ChordStrip::paint(juce::Graphics& g)
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

    juce::Colour chordColour = track::violet;
    const auto& chordChanges = sequence->getChordChanges();

    for (int i = 0; i < static_cast<int>(chordChanges.size()); ++i)
    {
        auto spanRect = chordSpanRect(i);
        if (spanRect.isEmpty())
            continue;
        if (spanRect.getX() > visibleRight || spanRect.getRight() < visibleLeft)
            continue;

        bool selected = selectedChordIndices.count(i) > 0;
        bool editingThis =
            isChordEditing && !chordEditIsNew && chordChanges[static_cast<size_t>(i)].tick == chordEditTick;
        bool highlighted = selected || editingThis;

        if (highlighted)
        {
            g.setColour(surface::selection);
            g.fillRoundedRectangle(spanRect.toFloat(), radius::r1);
        }
        else
        {
            g.setColour(chordColour.withAlpha(0.15f));
            g.fillRect(spanRect);
        }
        g.setColour(chordColour.withAlpha(highlighted ? 0.8f : 0.5f));
        g.drawRect(spanRect, 1);

        int textX = spanRect.getX() + 4;
        int textWidth = spanRect.getRight() - textX - 2;
        if (textWidth > 8)
        {
            ChordChange displayed = chordChanges[static_cast<size_t>(i)];
            if (editingThis)
            {
                displayed.chordRoot = chordDraftRoot;
                displayed.chordType = chordDraftType;
                displayed.bassRoot = chordDraftBassRoot;
            }
            g.setColour(highlighted ? chordColour.brighter(0.5f) : chordColour);
            g.setFont(font::sans(font::sizeSM));
            g.drawText(juce::String(MidiSequence::chordToString(displayed)), textX, 0, textWidth, getHeight(),
                       juce::Justification::centredLeft);
        }
    }

    if (isChordEditing && chordEditIsNew)
    {
        auto draftRect = chordDraftSpanRect();
        if (!draftRect.isEmpty() && draftRect.getX() <= visibleRight && draftRect.getRight() >= visibleLeft)
        {
            g.setColour(chordColour.withAlpha(0.1f));
            g.fillRect(draftRect);
            g.setColour(chordColour.withAlpha(0.35f));
            g.drawRect(draftRect, 1);

            int textX = draftRect.getX() + 4;
            int textWidth = draftRect.getRight() - textX - 2;
            if (textWidth > 8)
            {
                ChordChange draft{chordEditTick, chordDraftRoot, chordDraftType, chordDraftBassRoot,
                                  MidiSequence::chordNone};
                g.setColour(chordColour.withAlpha(0.6f));
                g.setFont(font::sans(font::sizeSM));
                g.drawText(juce::String(MidiSequence::chordToString(draft)), textX, 0, textWidth, getHeight(),
                           juce::Justification::centredLeft);
            }
        }
    }

    float phX = playheadX();
    if (phX >= static_cast<float>(visibleLeft) - 1.0f && phX <= static_cast<float>(visibleRight) + 1.0f)
    {
        g.setColour(text::t1);
        g.drawLine(phX, 0.0f, phX, static_cast<float>(getHeight()), 1.0f);
    }

    drawChordRangeSelection(g);

    drawLoopOverlay(g, 0, getHeight(), 0.12f);

    g.restoreState();

    drawLabelColumn(g);
}

void ChordStrip::drawChordRangeSelection(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (!isChordRangeSelecting)
        return;

    int lo = std::min(chordSelectStartX, chordSelectCurrentX);
    int hi = std::max(chordSelectStartX, chordSelectCurrentX);
    if (hi <= lo)
        return;

    juce::Rectangle<float> band(static_cast<float>(lo), 0.0f, static_cast<float>(hi - lo),
                                static_cast<float>(getHeight()));
    g.setColour(track::violet.withAlpha(0.15f));
    g.fillRect(band);
    g.setColour(track::violet.withAlpha(0.6f));
    g.drawRect(band, 1.0f);
}

void ChordStrip::mouseDown(const juce::MouseEvent& e)
{
    if (sequence == nullptr || sequence->getNumTracks() == 0)
        return;
    if (e.mods.isRightButtonDown())
        return;

    if (e.mods.isShiftDown())
    {
        if (e.y >= 0 && e.y < getHeight() && e.x >= viewLeftX + labelWidth())
        {
            if (onSelectionTaken)
                onSelectionTaken();
            isChordRangeSelecting = true;
            chordSelectStartX = e.x;
            chordSelectCurrentX = e.x;
            chordSelectBase = selectedChordIndices;
            chordRangeToggleIndex = hitTestChordSpan(e.x, e.y);
            repaint();
        }
        return;
    }

    auto [edgeIndex, edgeKind] = hitTestChordEdge(e.x, e.y);
    if (edgeIndex >= 0 && edgeKind == ResizeEdge::Right)
    {
        const auto& changes = sequence->getChordChanges();
        const bool joined = edgeIndex + 1 < static_cast<int>(changes.size()) &&
                            !MidiSequence::chordToString(changes[static_cast<size_t>(edgeIndex) + 1]).empty();
        if (joined)
        {
            chordStartResizeBefore = changes;
            chordStartResizeIndex = edgeIndex + 1;
            isChordStartResizing = true;
            chordStartResizeGrabOffset = geometry.xToTick(e.x) - changes[static_cast<size_t>(edgeIndex) + 1].tick;
            return;
        }

        chordResizeBefore = changes;
        chordResizeIndex = edgeIndex;
        isChordResizing = true;
        chordResizeGrabOffset = 0;
        if (edgeIndex + 1 < static_cast<int>(changes.size()))
            chordResizeGrabOffset = geometry.xToTick(e.x) - changes[static_cast<size_t>(edgeIndex) + 1].tick;
        return;
    }
    if (edgeIndex >= 0 && edgeKind == ResizeEdge::Left)
    {
        const auto& changes = sequence->getChordChanges();
        chordStartResizeBefore = changes;
        chordStartResizeIndex = edgeIndex;
        isChordStartResizing = true;
        chordStartResizeGrabOffset = geometry.xToTick(e.x) - changes[static_cast<size_t>(edgeIndex)].tick;
        return;
    }

    int index = hitTestChordSpan(e.x, e.y);
    if (index < 0)
    {
        if (e.y >= 0 && e.y < getHeight() && e.x >= viewLeftX + labelWidth())
        {
            if (onSelectionTaken)
                onSelectionTaken();
            isChordRangeSelecting = true;
            chordSelectStartX = e.x;
            chordSelectCurrentX = e.x;
            chordSelectBase.clear();
            chordRangeToggleIndex = -1;
            selectedChordIndices.clear();
            repaint();
        }
        return;
    }

    const auto& changes = sequence->getChordChanges();
    chordMoveBefore = changes;
    chordMoveIndex = index;
    isChordMoving = true;
    chordMoveGrabOffset = geometry.xToTick(e.x) - changes[static_cast<size_t>(index)].tick;
    if (selectedChordIndices.count(index) > 0 && selectedChordIndices.size() > 1)
        chordMoveGroup.assign(selectedChordIndices.begin(), selectedChordIndices.end());
    else
        chordMoveGroup = {index};
}

void ChordStrip::mouseMove(const juce::MouseEvent& e)
{
    setMouseCursor(!e.mods.isShiftDown() && hitTestChordEdge(e.x, e.y).first >= 0
                       ? juce::MouseCursor::LeftRightResizeCursor
                       : juce::MouseCursor::NormalCursor);
}

void ChordStrip::selectMovedChords(int anchorIndex, int cursorTick)
{
    const auto& changes = sequence->getChordChanges();
    const int landedTick = geometry.roundTickToGrid(std::max(0, cursorTick));
    int delta = landedTick - chordMoveBefore[static_cast<size_t>(anchorIndex)].tick;
    for (int g : chordMoveGroup)
    {
        if (g < 0 || g >= static_cast<int>(chordMoveBefore.size()) ||
            MidiSequence::chordToString(chordMoveBefore[static_cast<size_t>(g)]).empty())
            continue;
        delta = std::max(delta, -chordMoveBefore[static_cast<size_t>(g)].tick);
        break;
    }
    selectedChordIndices.clear();
    for (int g : chordMoveGroup)
    {
        if (g < 0 || g >= static_cast<int>(chordMoveBefore.size()) ||
            MidiSequence::chordToString(chordMoveBefore[static_cast<size_t>(g)]).empty())
            continue;
        const int target = chordMoveBefore[static_cast<size_t>(g)].tick + delta;
        for (int i = 0; i < static_cast<int>(changes.size()); ++i)
            if (changes[static_cast<size_t>(i)].tick == target &&
                !MidiSequence::chordToString(changes[static_cast<size_t>(i)]).empty())
                selectedChordIndices.insert(i);
    }
}

void ChordStrip::mouseDrag(const juce::MouseEvent& e)
{
    if (!sequence || !e.mouseWasDraggedSinceMouseDown())
        return;

    if (isChordResizing)
    {
        if (chordResizeIndex < 0 || chordResizeIndex >= static_cast<int>(chordResizeBefore.size()))
            return;

        sequence->setChordChanges(MidiSequence::buildChordChangesAfterResize(
            chordResizeBefore, chordResizeIndex, geometry.xToTick(e.x) - chordResizeGrabOffset, geometry.gridTicks()));
        repaint();
        return;
    }

    if (isChordStartResizing)
    {
        if (chordStartResizeIndex < 0 || chordStartResizeIndex >= static_cast<int>(chordStartResizeBefore.size()))
            return;

        sequence->setChordChanges(MidiSequence::buildChordChangesAfterStartResize(
            chordStartResizeBefore, chordStartResizeIndex, geometry.xToTick(e.x) - chordStartResizeGrabOffset,
            geometry.gridTicks()));
        repaint();
        return;
    }

    if (isChordMoving)
    {
        if (chordMoveIndex < 0 || chordMoveIndex >= static_cast<int>(chordMoveBefore.size()))
            return;

        sequence->setChordChanges(MidiSequence::buildChordChangesAfterMove(
            chordMoveBefore, chordMoveGroup, chordMoveIndex, geometry.xToTick(e.x) - chordMoveGrabOffset,
            geometry.gridTicks()));
        selectMovedChords(chordMoveIndex, geometry.xToTick(e.x) - chordMoveGrabOffset);
        repaint();
        return;
    }

    if (isChordRangeSelecting)
    {
        chordSelectCurrentX = e.x;
        int lo = std::min(chordSelectStartX, chordSelectCurrentX);
        int hi = std::max(chordSelectStartX, chordSelectCurrentX);
        int tickLo = geometry.xToTick(lo);
        int tickHi = geometry.xToTick(hi);

        selectedChordIndices = chordSelectBase;
        const auto& changes = sequence->getChordChanges();
        for (int i = 0; i < static_cast<int>(changes.size()); ++i)
        {
            if (MidiSequence::chordToString(changes[static_cast<size_t>(i)]).empty())
                continue;
            if (changes[static_cast<size_t>(i)].tick > tickHi)
                continue;
            if (i + 1 >= static_cast<int>(changes.size()) || changes[static_cast<size_t>(i) + 1].tick > tickLo)
                selectedChordIndices.insert(i);
        }
        repaint();
    }
}

void ChordStrip::mouseUp(const juce::MouseEvent& e)
{
    if (isChordResizing)
    {
        isChordResizing = false;
        const int resizedIndex = chordResizeIndex;
        chordResizeIndex = -1;

        if (!sequence)
        {
            chordResizeBefore.clear();
            return;
        }

        const auto& changes = sequence->getChordChanges();
        const bool validIndex = resizedIndex >= 0 && resizedIndex < static_cast<int>(chordResizeBefore.size()) &&
                                resizedIndex < static_cast<int>(changes.size());
        const bool resized = validIndex && changes != chordResizeBefore;

        if (resized)
        {
            if (undoManager)
            {
                undoManager->beginNewTransaction("Resize Chord");
                undoManager->perform(new ChordResizeAction(sequence, chordResizeBefore, changes));
            }
            else
            {
                sequence->notifyTimelineMetadataChanged();
            }
        }

        if (validIndex)
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selectedChordIndices = {resizedIndex};
        }

        chordResizeBefore.clear();
        repaint();
        return;
    }

    if (isChordStartResizing)
    {
        isChordStartResizing = false;
        const int movedIndex = chordStartResizeIndex;
        chordStartResizeIndex = -1;

        if (!sequence)
        {
            chordStartResizeBefore.clear();
            return;
        }

        if (movedIndex < 0 || movedIndex >= static_cast<int>(chordStartResizeBefore.size()))
        {
            chordStartResizeBefore.clear();
            repaint();
            return;
        }

        if (e.mouseWasDraggedSinceMouseDown())
            sequence->setChordChanges(MidiSequence::buildChordChangesAfterStartResize(
                chordStartResizeBefore, movedIndex, geometry.xToTick(e.x) - chordStartResizeGrabOffset,
                geometry.gridTicks()));

        const auto& changes = sequence->getChordChanges();
        if (changes != chordStartResizeBefore)
        {
            int landedIndex = static_cast<int>(changes.size()) - 1;
            if (movedIndex + 1 < static_cast<int>(chordStartResizeBefore.size()))
            {
                const int endTick = chordStartResizeBefore[static_cast<size_t>(movedIndex) + 1].tick;
                for (int i = 0; i < static_cast<int>(changes.size()); ++i)
                    if (changes[static_cast<size_t>(i)].tick == endTick)
                        landedIndex = i - 1;
            }

            if (undoManager)
            {
                undoManager->beginNewTransaction("Resize Chord");
                undoManager->perform(new ChordResizeAction(sequence, chordStartResizeBefore, changes));
            }
            else
            {
                sequence->notifyTimelineMetadataChanged();
            }

            if (onSelectionTaken)
                onSelectionTaken();
            selectedChordIndices = {landedIndex};
        }
        else
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selectedChordIndices = {movedIndex};
        }

        chordStartResizeBefore.clear();
        repaint();
        return;
    }

    if (isChordMoving)
    {
        isChordMoving = false;
        const int movedIndex = chordMoveIndex;
        chordMoveIndex = -1;

        if (!sequence)
        {
            chordMoveBefore.clear();
            chordMoveGroup.clear();
            return;
        }

        if (movedIndex < 0 || movedIndex >= static_cast<int>(chordMoveBefore.size()))
        {
            chordMoveBefore.clear();
            chordMoveGroup.clear();
            repaint();
            return;
        }

        if (e.mouseWasDraggedSinceMouseDown())
            sequence->setChordChanges(MidiSequence::buildChordChangesAfterMove(
                chordMoveBefore, chordMoveGroup, movedIndex, geometry.xToTick(e.x) - chordMoveGrabOffset,
                geometry.gridTicks()));

        const auto& changes = sequence->getChordChanges();
        if (changes != chordMoveBefore)
        {
            if (undoManager)
            {
                undoManager->beginNewTransaction("Move Chord");
                undoManager->perform(new ChordMoveAction(sequence, chordMoveBefore, changes));
            }
            else
            {
                sequence->notifyTimelineMetadataChanged();
            }

            if (onSelectionTaken)
                onSelectionTaken();
            selectMovedChords(movedIndex, geometry.xToTick(e.x) - chordMoveGrabOffset);
        }
        else
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selectedChordIndices = {movedIndex};
        }

        chordMoveBefore.clear();
        chordMoveGroup.clear();
        repaint();
        return;
    }

    if (isChordRangeSelecting)
    {
        isChordRangeSelecting = false;
        const int toggleIndex = chordRangeToggleIndex;
        chordRangeToggleIndex = -1;
        chordSelectBase.clear();

        if (!e.mouseWasDraggedSinceMouseDown() && toggleIndex >= 0)
        {
            if (selectedChordIndices.count(toggleIndex) > 0)
                selectedChordIndices.erase(toggleIndex);
            else
                selectedChordIndices.insert(toggleIndex);
        }
        repaint();
    }
}

void ChordStrip::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!sequence || e.mods.isRightButtonDown() || isChordEditing)
        return;

    if (e.y < 0 || e.y >= getHeight() || e.x < viewLeftX + labelWidth())
        return;

    isChordResizing = false;
    chordResizeIndex = -1;
    chordResizeBefore.clear();
    isChordMoving = false;
    chordMoveIndex = -1;
    chordMoveBefore.clear();
    chordMoveGroup.clear();
    isChordStartResizing = false;
    chordStartResizeIndex = -1;
    chordStartResizeBefore.clear();
    isChordRangeSelecting = false;
    chordSelectBase.clear();
    chordRangeToggleIndex = -1;

    int index = hitTestChordSpan(e.x, e.y);
    if (index >= 0)
    {
        const auto& cc = sequence->getChordChanges()[static_cast<size_t>(index)];
        if (onSelectionTaken)
            onSelectionTaken();
        selectedChordIndices = {index};
        repaint();
        openChordEditor(cc.tick, 0, cc.chordRoot, cc.chordType, cc.bassRoot, false,
                        {chordSpanRect(index).getX() + 4, 0, 40, getHeight()});
        return;
    }

    auto [startTick, endTick] = sequence->chordAddSpanAt(std::max(0, geometry.xToTick(e.x)));
    if (endTick <= startTick)
        return;

    if (onSelectionTaken)
        onSelectionTaken();
    clearChordSelection();

    int root = 0x31;
    int type = 0;
    int bassRoot = MidiSequence::chordNone;
    const auto& changes = sequence->getChordChanges();
    for (int i = static_cast<int>(changes.size()) - 1; i >= 0; --i)
    {
        const auto& cc = changes[static_cast<size_t>(i)];
        if (cc.tick >= startTick)
            continue;
        if (MidiSequence::chordToString(cc).empty())
            continue;
        root = cc.chordRoot;
        type = cc.chordType;
        bassRoot = cc.bassRoot;
        break;
    }

    openChordEditor(startTick, endTick, root, type, bassRoot, true,
                    {geometry.tickToX(startTick) + 4, 0, 40, getHeight()});
}

void ChordStrip::openChordEditor(int tick, int endTick, int chordRoot, int chordType, int bassRoot, bool isNew,
                                 juce::Rectangle<int> anchorInLocal)
{
    anchorInLocal.setX(std::max(anchorInLocal.getX(), viewLeftX + labelWidth()));

    chordRoot = MidiSequence::normalizeChordRoot(chordRoot);
    chordType = MidiSequence::normalizeChordType(chordType);
    bassRoot = MidiSequence::normalizeChordBassRoot(bassRoot);

    isChordEditing = true;
    chordEditTick = tick;
    chordEditEndTick = endTick;
    chordDraftRoot = chordRoot;
    chordDraftType = chordType;
    chordDraftBassRoot = bassRoot;
    chordEditIsNew = isNew;
    chordEditSpelling = MidiSequence::chordSpellingForKeySignature(sequence->getKeySignatureAt(tick).sharpsOrFlats);

    auto content = std::make_unique<ChordEditor>(chordRoot, chordType, bassRoot, chordEditSpelling, isNew);
    chordEditor = content.get();
    content->onDraftChanged = [this](int root, int type, int bass)
    {
        chordDraftRoot = root;
        chordDraftType = type;
        chordDraftBassRoot = bass;
        repaint();
    };
    content->onCommit = [this](int root, int type, int bass) { commitChordEdit(root, type, bass); };
    content->onCancel = [this]() { cancelChordEdit(); };

    auto& box = juce::CallOutBox::launchAsynchronously(std::move(content), localAreaToGlobal(anchorInLocal), nullptr);
    box.setDismissalMouseClicksAreAlwaysConsumed(true);
    chordCallout = &box;
    repaint();
}

void ChordStrip::commitChordEdit(int chordRoot, int chordType, int bassRoot)
{
    isChordEditing = false;
    chordEditor = nullptr;
    chordCallout = nullptr;

    if (!sequence)
        return;

    const int bassType = (bassRoot == MidiSequence::chordNone) ? MidiSequence::chordNone : chordType;

    if (chordEditIsNew)
    {
        auto after = MidiSequence::buildChordChangesAfterAdd(
            sequence->getChordChanges(), chordEditTick, chordEditEndTick, chordRoot, chordType, bassRoot, bassType);
        if (undoManager)
        {
            undoManager->beginNewTransaction("Add Chord");
            undoManager->perform(new ChordAddAction(sequence, sequence->getChordChanges(), std::move(after)));
        }
        else
        {
            sequence->setChordChanges(std::move(after));
            sequence->notifyTimelineMetadataChanged();
        }
    }
    else
    {
        const auto& existing = sequence->getChordChanges();
        auto atTick = std::ranges::find(existing, chordEditTick, &ChordChange::tick);
        if (atTick != existing.end() && atTick->chordRoot == chordRoot && atTick->chordType == chordType &&
            atTick->bassRoot == bassRoot)
        {
            repaint();
            return;
        }

        if (undoManager)
        {
            undoManager->beginNewTransaction("Edit Chord");
            undoManager->perform(
                new ChordChangeAction(sequence, chordEditTick, chordRoot, chordType, bassRoot, bassType));
        }
        else
        {
            sequence->addChordChange(chordEditTick, chordRoot, chordType, bassRoot, bassType);
            sequence->notifyTimelineMetadataChanged();
        }
    }

    const auto& changes = sequence->getChordChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
        if (changes[static_cast<size_t>(i)].tick == chordEditTick)
            selectedChordIndices = {i};
    repaint();
}

void ChordStrip::cancelChordEdit()
{
    isChordEditing = false;
    chordEditor = nullptr;
    chordCallout = nullptr;
    repaint();
}

void ChordStrip::closeChordEditor()
{
    if (chordEditor != nullptr)
        chordEditor->abandon();
    if (chordCallout != nullptr)
        chordCallout->dismiss();

    isChordEditing = false;
    chordEditor = nullptr;
    chordCallout = nullptr;
}
