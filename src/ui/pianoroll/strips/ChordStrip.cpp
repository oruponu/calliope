#include "ui/pianoroll/strips/ChordStrip.h"
#include "edit/ChordTrackEdits.h"
#include "ui/theme/Theme.h"
#include "undo/ReplaceListAction.h"
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <memory>
#include <utility>

namespace
{
int chordSpanEnd(const std::vector<ChordChange>& changes, size_t index)
{
    return index + 1 < changes.size() ? changes[index + 1].tick : INT_MAX;
}

bool isSameChord(const ChordChange& a, const ChordChange& b)
{
    return a.chordRoot == b.chordRoot && a.chordType == b.chordType && a.bassRoot == b.bassRoot &&
           a.bassType == b.bassType;
}
} // namespace

ChordStrip::ChordStrip(const TimelineGeometry& geometryRef, EditClipboard& clipboardRef,
                       juce::UndoManager& undoManagerRef)
    : TimelineStrip(geometryRef, "Chord"), undoManager(undoManagerRef), clipboard(clipboardRef)
{
}

ChordStrip::~ChordStrip()
{
    closeChordEditor();
}

void ChordStrip::setSequence(MidiSequence* seq)
{
    closeChordEditor();
    clearChordSelection();
    isChordRangeSelecting = false;
    rangeSelect = {};
    chordRangeToggleIndex = -1;
    TimelineStrip::setSequence(seq);
}

void ChordStrip::clearChordSelection()
{
    if (selection.isEmpty())
        return;
    selection.clear();
    repaint();
}

bool ChordStrip::hasSelection() const
{
    return !selection.isEmpty();
}

void ChordStrip::deleteSelectedChords()
{
    deleteSelectedChordsImpl("Delete Chords");
}

void ChordStrip::deleteSelectedChordsImpl(const juce::String& transactionName)
{
    if (!sequence || selection.isEmpty())
        return;

    auto before = sequence->getChordChanges();
    auto after =
        ChordTrackEdits::afterDelete(before, std::vector<int>(selection.indices().begin(), selection.indices().end()));
    if (after == before)
        return;

    undoManager.beginNewTransaction(transactionName);
    undoManager.perform(new ReplaceListAction<ChordChange>(sequence, std::move(before), std::move(after)));

    clearChordSelection();
    repaint();
}

void ChordStrip::copySelectedChords()
{
    if (!sequence || selection.isEmpty())
        return;

    const auto& changes = sequence->getChordChanges();
    const int count = static_cast<int>(changes.size());

    std::vector<RelativeChord> items;
    int baseTick = -1;
    for (int i : selection.indices())
    {
        if (i < 0 || i >= count || changes[static_cast<size_t>(i)].isNoChord())
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
    if (!sequence || selection.isEmpty())
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
    auto after = ChordTrackEdits::afterPaste(before, clipboard.getChords(), anchorTick);

    const bool changed = (after != before);
    if (changed)
    {
        undoManager.beginNewTransaction("Paste Chords");
        undoManager.perform(new ReplaceListAction<ChordChange>(sequence, std::move(before), after));
    }

    selection.clear();
    const auto& changes = sequence->getChordChanges();
    for (const auto& item : clipboard.getChords())
    {
        const int target = anchorTick + item.tickOffset;
        for (int i = 0; i < static_cast<int>(changes.size()); ++i)
            if (changes[static_cast<size_t>(i)].tick == target && !changes[static_cast<size_t>(i)].isNoChord())
                selection.add(i);
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
    if (changes[static_cast<size_t>(index)].isNoChord())
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

bool ChordStrip::isJointChordEdge(int index, ResizeEdge edge) const
{
    const auto& changes = sequence->getChordChanges();
    const int neighbor = edge == ResizeEdge::Right ? index + 1 : index - 1;
    return neighbor >= 0 && neighbor < static_cast<int>(changes.size()) &&
           !changes[static_cast<size_t>(neighbor)].isNoChord();
}

void ChordStrip::beginChordEdgeDrag(const std::vector<ChordChange>& changes, int index, ResizeEdge edge, int grabX)
{
    if (edge == ResizeEdge::Right)
    {
        chordResizeBefore = changes;
        chordResizeIndex = index;
        isChordResizing = true;
        chordResizeGrabOffset = 0;
        if (index + 1 < static_cast<int>(changes.size()))
            chordResizeGrabOffset = geometry.xToTick(grabX) - changes[static_cast<size_t>(index) + 1].tick;
        return;
    }

    chordStartResizeBefore = changes;
    chordStartResizeIndex = index;
    isChordStartResizing = true;
    chordStartResizeGrabOffset = geometry.xToTick(grabX) - changes[static_cast<size_t>(index)].tick;
}

void ChordStrip::remapSelectionAfterResize(const std::vector<ChordChange>& before, int draggedIndex, ResizeEdge edge)
{
    const auto& after = sequence->getChordChanges();
    const size_t dragged = static_cast<size_t>(draggedIndex);

    int draggedAfter = -1;
    for (size_t j = 0; j < after.size(); ++j)
    {
        if (after[j].isNoChord())
            continue;
        const bool matched = edge == ResizeEdge::Right ? after[j].tick == before[dragged].tick
                                                       : chordSpanEnd(after, j) == chordSpanEnd(before, dragged);
        if (matched)
        {
            draggedAfter = static_cast<int>(j);
            break;
        }
    }

    selection.clear();
    for (int i : chordEdgeSelectionBefore)
    {
        if (i < 0 || i >= static_cast<int>(before.size()))
            continue;
        const size_t bi = static_cast<size_t>(i);
        if (i == draggedIndex)
        {
            if (draggedAfter >= 0)
                selection.add(draggedAfter);
            continue;
        }
        for (size_t j = 0; j < after.size(); ++j)
        {
            if (static_cast<int>(j) == draggedAfter || after[j].isNoChord() || !isSameChord(after[j], before[bi]))
                continue;
            if (after[j].tick < chordSpanEnd(before, bi) && chordSpanEnd(after, j) > before[bi].tick)
            {
                selection.add(static_cast<int>(j));
                break;
            }
        }
    }
}

void ChordStrip::switchJointChordEdge(int x, int grabX)
{
    const int dx = x - grabX;
    const ResizeEdge edge = dx > 0 ? ResizeEdge::Right : dx < 0 ? ResizeEdge::Left : chordJointEdge;
    isChordResizing = false;
    isChordStartResizing = false;
    beginChordEdgeDrag(chordJointBefore, edge == ResizeEdge::Right ? chordJointLeftIndex : chordJointLeftIndex + 1,
                       edge, grabX);
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

        bool selected = selection.contains(i);
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
            g.drawText(juce::String(ChordSymbol::toString(displayed)), textX, 0, textWidth, getHeight(),
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
                ChordChange draft{chordEditTick, chordDraftRoot, chordDraftType, chordDraftBassRoot, ChordChange::none};
                g.setColour(chordColour.withAlpha(0.6f));
                g.setFont(font::sans(font::sizeSM));
                g.drawText(juce::String(ChordSymbol::toString(draft)), textX, 0, textWidth, getHeight(),
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

    if (isChordRangeSelecting)
        drawRangeBand(g, rangeSelect, track::violet.withAlpha(0.15f), track::violet.withAlpha(0.6f));

    drawLoopOverlay(g, 0, getHeight(), 0.12f);

    g.restoreState();

    drawLabelColumn(g);
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
            rangeSelect = {e.x, e.x, selection.indices()};
            chordRangeToggleIndex = hitTestChordSpan(e.x, e.y);
            repaint();
        }
        return;
    }

    auto [edgeIndex, edgeKind] = hitTestChordEdge(e.x, e.y);
    if (edgeIndex >= 0 && edgeKind != ResizeEdge::None)
    {
        chordEdgeSelectionBefore = selection.indices();
        if (isJointChordEdge(edgeIndex, edgeKind))
        {
            isChordJointDragging = true;
            chordJointBefore = sequence->getChordChanges();
            chordJointLeftIndex = edgeKind == ResizeEdge::Right ? edgeIndex : edgeIndex - 1;
            chordJointEdge = edgeKind;
            switchJointChordEdge(e.x, e.x);
            return;
        }
        beginChordEdgeDrag(sequence->getChordChanges(), edgeIndex, edgeKind, e.x);
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
            rangeSelect = {e.x, e.x, {}};
            chordRangeToggleIndex = -1;
            selection.clear();
            repaint();
        }
        return;
    }

    const auto& changes = sequence->getChordChanges();
    chordMoveBefore = changes;
    chordMoveIndex = index;
    isChordMoving = true;
    chordMoveGrabOffset = geometry.xToTick(e.x) - changes[static_cast<size_t>(index)].tick;
    chordMoveGroup = selection.dragGroup(index);
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
            chordMoveBefore[static_cast<size_t>(g)].isNoChord())
            continue;
        delta = std::max(delta, -chordMoveBefore[static_cast<size_t>(g)].tick);
        break;
    }
    selection.clear();
    for (int g : chordMoveGroup)
    {
        if (g < 0 || g >= static_cast<int>(chordMoveBefore.size()) ||
            chordMoveBefore[static_cast<size_t>(g)].isNoChord())
            continue;
        const int target = chordMoveBefore[static_cast<size_t>(g)].tick + delta;
        for (int i = 0; i < static_cast<int>(changes.size()); ++i)
            if (changes[static_cast<size_t>(i)].tick == target && !changes[static_cast<size_t>(i)].isNoChord())
                selection.add(i);
    }
}

void ChordStrip::mouseDrag(const juce::MouseEvent& e)
{
    if (!sequence || !e.mouseWasDraggedSinceMouseDown())
        return;

    if (isChordJointDragging)
        switchJointChordEdge(e.x, e.getMouseDownX());

    if (isChordResizing)
    {
        if (chordResizeIndex < 0 || chordResizeIndex >= static_cast<int>(chordResizeBefore.size()))
            return;

        sequence->setChordChanges(ChordTrackEdits::afterResize(
            chordResizeBefore, chordResizeIndex, geometry.xToTick(e.x) - chordResizeGrabOffset, geometry.gridTicks()));
        remapSelectionAfterResize(chordResizeBefore, chordResizeIndex, ResizeEdge::Right);
        repaint();
        return;
    }

    if (isChordStartResizing)
    {
        if (chordStartResizeIndex < 0 || chordStartResizeIndex >= static_cast<int>(chordStartResizeBefore.size()))
            return;

        sequence->setChordChanges(ChordTrackEdits::afterStartResize(chordStartResizeBefore, chordStartResizeIndex,
                                                                    geometry.xToTick(e.x) - chordStartResizeGrabOffset,
                                                                    geometry.gridTicks()));
        remapSelectionAfterResize(chordStartResizeBefore, chordStartResizeIndex, ResizeEdge::Left);
        repaint();
        return;
    }

    if (isChordMoving)
    {
        if (chordMoveIndex < 0 || chordMoveIndex >= static_cast<int>(chordMoveBefore.size()))
            return;

        sequence->setChordChanges(ChordTrackEdits::afterMove(chordMoveBefore, chordMoveGroup, chordMoveIndex,
                                                             geometry.xToTick(e.x) - chordMoveGrabOffset,
                                                             geometry.gridTicks()));
        selectMovedChords(chordMoveIndex, geometry.xToTick(e.x) - chordMoveGrabOffset);
        repaint();
        return;
    }

    if (isChordRangeSelecting)
    {
        rangeSelect.currentX = e.x;
        const auto& changes = sequence->getChordChanges();
        selection.assign(rangeSelect.selectionFor(static_cast<int>(changes.size()), geometry,
                                                  [&changes](int i, int tickLo, int tickHi)
                                                  {
                                                      const auto index = static_cast<size_t>(i);
                                                      if (changes[index].isNoChord() || changes[index].tick > tickHi)
                                                          return false;
                                                      return index + 1 >= changes.size() ||
                                                             changes[index + 1].tick > tickLo;
                                                  }));
        repaint();
    }
}

void ChordStrip::mouseUp(const juce::MouseEvent& e)
{
    if (isChordJointDragging)
    {
        isChordJointDragging = false;
        switchJointChordEdge(e.x, e.getMouseDownX());
        chordJointBefore.clear();
    }

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

        const bool validIndex = resizedIndex >= 0 && resizedIndex < static_cast<int>(chordResizeBefore.size());
        if (validIndex && e.mouseWasDraggedSinceMouseDown())
            sequence->setChordChanges(ChordTrackEdits::afterResize(
                chordResizeBefore, resizedIndex, geometry.xToTick(e.x) - chordResizeGrabOffset, geometry.gridTicks()));

        const auto& changes = sequence->getChordChanges();
        const bool resized = validIndex && changes != chordResizeBefore;

        if (resized)
        {
            undoManager.beginNewTransaction("Resize Chord");
            undoManager.perform(new ReplaceListAction<ChordChange>(sequence, chordResizeBefore, changes));
        }

        if (validIndex && e.mouseWasDraggedSinceMouseDown())
        {
            remapSelectionAfterResize(chordResizeBefore, resizedIndex, ResizeEdge::Right);
        }
        else if (validIndex)
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selection.selectOnly(resizedIndex);
        }

        chordEdgeSelectionBefore.clear();
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
            sequence->setChordChanges(ChordTrackEdits::afterStartResize(
                chordStartResizeBefore, movedIndex, geometry.xToTick(e.x) - chordStartResizeGrabOffset,
                geometry.gridTicks()));

        const auto& changes = sequence->getChordChanges();
        if (changes != chordStartResizeBefore)
        {
            undoManager.beginNewTransaction("Resize Chord");
            undoManager.perform(new ReplaceListAction<ChordChange>(sequence, chordStartResizeBefore, changes));
        }

        if (e.mouseWasDraggedSinceMouseDown())
        {
            remapSelectionAfterResize(chordStartResizeBefore, movedIndex, ResizeEdge::Left);
        }
        else
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selection.selectOnly(movedIndex);
        }

        chordEdgeSelectionBefore.clear();
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
            sequence->setChordChanges(ChordTrackEdits::afterMove(chordMoveBefore, chordMoveGroup, movedIndex,
                                                                 geometry.xToTick(e.x) - chordMoveGrabOffset,
                                                                 geometry.gridTicks()));

        const auto& changes = sequence->getChordChanges();
        if (changes != chordMoveBefore)
        {
            undoManager.beginNewTransaction("Move Chord");
            undoManager.perform(new ReplaceListAction<ChordChange>(sequence, chordMoveBefore, changes));

            if (onSelectionTaken)
                onSelectionTaken();
            selectMovedChords(movedIndex, geometry.xToTick(e.x) - chordMoveGrabOffset);
        }
        else
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selection.selectOnly(movedIndex);
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
        rangeSelect = {};

        if (!e.mouseWasDraggedSinceMouseDown() && toggleIndex >= 0)
            selection.toggle(toggleIndex);
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
    rangeSelect = {};
    chordRangeToggleIndex = -1;

    int index = hitTestChordSpan(e.x, e.y);
    if (index >= 0)
    {
        const auto& cc = sequence->getChordChanges()[static_cast<size_t>(index)];
        if (onSelectionTaken)
            onSelectionTaken();
        selection.selectOnly(index);
        repaint();
        openChordEditor(cc.tick, 0, cc.chordRoot, cc.chordType, cc.bassRoot, false,
                        {chordSpanRect(index).getX() + 4, 0, 40, getHeight()});
        return;
    }

    auto [startTick, endTick] = ChordTrackEdits::addSpanAt(sequence->getChordChanges(),
                                                           std::max(0, geometry.xToTick(e.x)), sequence->getTimeline());
    if (endTick <= startTick)
        return;

    if (onSelectionTaken)
        onSelectionTaken();
    clearChordSelection();

    int root = 0x31;
    int type = 0;
    int bassRoot = ChordChange::none;
    const auto& changes = sequence->getChordChanges();
    for (int i = static_cast<int>(changes.size()) - 1; i >= 0; --i)
    {
        const auto& cc = changes[static_cast<size_t>(i)];
        if (cc.tick >= startTick)
            continue;
        if (cc.isNoChord())
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

    chordRoot = ChordSymbol::normalizeRoot(chordRoot);
    chordType = ChordSymbol::normalizeType(chordType);
    bassRoot = ChordSymbol::normalizeBassRoot(bassRoot);

    isChordEditing = true;
    chordEditTick = tick;
    chordEditEndTick = endTick;
    chordDraftRoot = chordRoot;
    chordDraftType = chordType;
    chordDraftBassRoot = bassRoot;
    chordEditIsNew = isNew;
    chordEditSpelling = ChordSymbol::spellingForKeySignature(sequence->getKeySignatureAt(tick).sharpsOrFlats);

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

    const int bassType = (bassRoot == ChordChange::none) ? ChordChange::none : chordType;

    if (chordEditIsNew)
    {
        auto after = ChordTrackEdits::afterAdd(sequence->getChordChanges(), chordEditTick, chordEditEndTick, chordRoot,
                                               chordType, bassRoot, bassType);
        undoManager.beginNewTransaction("Add Chord");
        undoManager.perform(
            new ReplaceListAction<ChordChange>(sequence, sequence->getChordChanges(), std::move(after)));
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

        undoManager.beginNewTransaction("Edit Chord");
        auto before = sequence->getChordChanges();
        auto after = before;
        ChordTrackEdits::add(after, chordEditTick, chordRoot, chordType, bassRoot, bassType);
        undoManager.perform(new ReplaceListAction<ChordChange>(sequence, std::move(before), std::move(after)));
    }

    const auto& changes = sequence->getChordChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
        if (changes[static_cast<size_t>(i)].tick == chordEditTick)
            selection.selectOnly(i);
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
