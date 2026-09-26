#include "ui/pianoroll/strips/ChordStrip.h"
#include "edit/ChordTrackEdits.h"
#include "notation/ChordSymbol.h"
#include "ui/theme/Theme.h"
#include "undo/ReplaceListAction.h"
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <memory>
#include <set>
#include <utility>
#include <variant>

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

void ChordStrip::setSequence(MidiSequence* seq)
{
    editSession.close();
    clearChordSelection();
    drag = Idle{};
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

    performReplaceList(undoManager, sequence, transactionName, std::move(before), std::move(after));

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
        performReplaceList(undoManager, sequence, "Paste Chords", std::move(before), after);
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

juce::Rectangle<int> ChordStrip::chordDraftSpanRect(const ChordDraft& draft) const
{
    if (!sequence)
        return {};

    int x = geometry.tickToX(draft.tick);
    int nextX = geometry.tickToX(draft.endTick);
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

ChordStrip::EdgeDrag ChordStrip::makeEdgeDrag(const std::vector<ChordChange>& changes, int index, ResizeEdge edge,
                                              int grabX, const std::set<int>& selectionBefore) const
{
    if (edge == ResizeEdge::Right)
    {
        int grabOffset = 0;
        if (index + 1 < static_cast<int>(changes.size()))
            grabOffset = geometry.xToTick(grabX) - changes[static_cast<size_t>(index) + 1].tick;
        return EndResizing{index, changes, grabOffset, selectionBefore};
    }

    return StartResizing{index, changes, geometry.xToTick(grabX) - changes[static_cast<size_t>(index)].tick,
                         selectionBefore};
}

ChordStrip::DragState ChordStrip::fromEdgeDrag(EdgeDrag edge)
{
    return std::visit([](auto&& e) -> DragState { return std::move(e); }, std::move(edge));
}

void ChordStrip::switchJointChordEdge(JointDragging& joint, int x, int grabX) const
{
    const int dx = x - grabX;
    const ResizeEdge edge = dx > 0 ? ResizeEdge::Right : dx < 0 ? ResizeEdge::Left : joint.initialEdge;
    joint.edge = makeEdgeDrag(joint.before, edge == ResizeEdge::Right ? joint.leftIndex : joint.leftIndex + 1, edge,
                              grabX, joint.selectionBefore);
}

void ChordStrip::dragEdge(const EndResizing& resizing, int x)
{
    if (resizing.index < 0 || resizing.index >= static_cast<int>(resizing.before.size()))
        return;

    sequence->setChordChanges(ChordTrackEdits::afterResize(
        resizing.before, resizing.index, geometry.xToTick(x) - resizing.grabOffset, geometry.gridTicks()));
    remapSelectionAfterResize(resizing.before, resizing.index, ResizeEdge::Right, resizing.selectionBefore);
    repaint();
}

void ChordStrip::dragEdge(const StartResizing& resizing, int x)
{
    if (resizing.index < 0 || resizing.index >= static_cast<int>(resizing.before.size()))
        return;

    sequence->setChordChanges(ChordTrackEdits::afterStartResize(
        resizing.before, resizing.index, geometry.xToTick(x) - resizing.grabOffset, geometry.gridTicks()));
    remapSelectionAfterResize(resizing.before, resizing.index, ResizeEdge::Left, resizing.selectionBefore);
    repaint();
}

void ChordStrip::remapSelectionAfterResize(const std::vector<ChordChange>& before, int draggedIndex, ResizeEdge edge,
                                           const std::set<int>& selectionBefore)
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
    for (int i : selectionBefore)
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
    const auto* draft = editSession.current();

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
            draft != nullptr && !draft->isNew && chordChanges[static_cast<size_t>(i)].tick == draft->tick;
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
                displayed.chordRoot = draft->root;
                displayed.chordType = draft->type;
                displayed.bassRoot = draft->bassRoot;
            }
            g.setColour(highlighted ? chordColour.brighter(0.5f) : chordColour);
            g.setFont(font::sans(font::sizeSM));
            g.drawText(juce::String(ChordSymbol::toString(displayed)), textX, 0, textWidth, getHeight(),
                       juce::Justification::centredLeft);
        }
    }

    if (draft != nullptr && draft->isNew)
    {
        auto draftRect = chordDraftSpanRect(*draft);
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
                ChordChange draftChord{draft->tick, draft->root, draft->type, draft->bassRoot, ChordChange::none};
                g.setColour(chordColour.withAlpha(0.6f));
                g.setFont(font::sans(font::sizeSM));
                g.drawText(juce::String(ChordSymbol::toString(draftChord)), textX, 0, textWidth, getHeight(),
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

    if (const auto* selecting = std::get_if<RangeSelecting>(&drag))
        drawRangeBand(g, selecting->gesture, track::violet.withAlpha(0.15f), track::violet.withAlpha(0.6f));

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
            drag = RangeSelecting{{e.x, e.x, selection.indices()}, hitTestChordSpan(e.x, e.y)};
            repaint();
        }
        return;
    }

    auto [edgeIndex, edgeKind] = hitTestChordEdge(e.x, e.y);
    if (edgeIndex >= 0 && edgeKind != ResizeEdge::None)
    {
        const auto& changes = sequence->getChordChanges();
        if (isJointChordEdge(edgeIndex, edgeKind))
        {
            JointDragging joint{edgeKind == ResizeEdge::Right ? edgeIndex : edgeIndex - 1, edgeKind, changes,
                                selection.indices(), EndResizing{}};
            switchJointChordEdge(joint, e.x, e.x);
            drag = std::move(joint);
            return;
        }
        drag = fromEdgeDrag(makeEdgeDrag(changes, edgeIndex, edgeKind, e.x, selection.indices()));
        return;
    }

    int index = hitTestChordSpan(e.x, e.y);
    if (index < 0)
    {
        if (e.y >= 0 && e.y < getHeight() && e.x >= viewLeftX + labelWidth())
        {
            if (onSelectionTaken)
                onSelectionTaken();
            drag = RangeSelecting{{e.x, e.x, {}}, -1};
            selection.clear();
            repaint();
        }
        return;
    }

    const auto& changes = sequence->getChordChanges();
    drag = Moving{index, changes, geometry.xToTick(e.x) - changes[static_cast<size_t>(index)].tick,
                  selection.dragGroup(index)};
}

void ChordStrip::mouseMove(const juce::MouseEvent& e)
{
    setMouseCursor(!e.mods.isShiftDown() && hitTestChordEdge(e.x, e.y).first >= 0
                       ? juce::MouseCursor::LeftRightResizeCursor
                       : juce::MouseCursor::NormalCursor);
}

void ChordStrip::selectMovedChords(const Moving& moving, int cursorTick)
{
    const auto& changes = sequence->getChordChanges();
    const int landedTick = geometry.roundTickToGrid(std::max(0, cursorTick));
    int delta = landedTick - moving.before[static_cast<size_t>(moving.index)].tick;
    for (int g : moving.group)
    {
        if (g < 0 || g >= static_cast<int>(moving.before.size()) || moving.before[static_cast<size_t>(g)].isNoChord())
            continue;
        delta = std::max(delta, -moving.before[static_cast<size_t>(g)].tick);
        break;
    }
    selection.clear();
    for (int g : moving.group)
    {
        if (g < 0 || g >= static_cast<int>(moving.before.size()) || moving.before[static_cast<size_t>(g)].isNoChord())
            continue;
        const int target = moving.before[static_cast<size_t>(g)].tick + delta;
        for (int i = 0; i < static_cast<int>(changes.size()); ++i)
            if (changes[static_cast<size_t>(i)].tick == target && !changes[static_cast<size_t>(i)].isNoChord())
                selection.add(i);
    }
}

void ChordStrip::mouseDrag(const juce::MouseEvent& e)
{
    if (!sequence || !e.mouseWasDraggedSinceMouseDown())
        return;

    if (auto* joint = std::get_if<JointDragging>(&drag))
    {
        switchJointChordEdge(*joint, e.x, e.getMouseDownX());
        std::visit([this, &e](const auto& edge) { dragEdge(edge, e.x); }, joint->edge);
        return;
    }

    if (const auto* resizing = std::get_if<EndResizing>(&drag))
    {
        dragEdge(*resizing, e.x);
        return;
    }

    if (const auto* resizing = std::get_if<StartResizing>(&drag))
    {
        dragEdge(*resizing, e.x);
        return;
    }

    if (const auto* moving = std::get_if<Moving>(&drag))
    {
        if (moving->index < 0 || moving->index >= static_cast<int>(moving->before.size()))
            return;

        sequence->setChordChanges(ChordTrackEdits::afterMove(moving->before, moving->group, moving->index,
                                                             geometry.xToTick(e.x) - moving->grabOffset,
                                                             geometry.gridTicks()));
        selectMovedChords(*moving, geometry.xToTick(e.x) - moving->grabOffset);
        repaint();
        return;
    }

    if (auto* selecting = std::get_if<RangeSelecting>(&drag))
    {
        selecting->gesture.currentX = e.x;
        const auto& changes = sequence->getChordChanges();
        selection.assign(
            selecting->gesture.selectionFor(static_cast<int>(changes.size()), geometry,
                                            [&changes](int i, int tickLo, int tickHi)
                                            {
                                                const auto index = static_cast<size_t>(i);
                                                if (changes[index].isNoChord() || changes[index].tick > tickHi)
                                                    return false;
                                                return index + 1 >= changes.size() || changes[index + 1].tick > tickLo;
                                            }));
        repaint();
    }
}

void ChordStrip::mouseUp(const juce::MouseEvent& e)
{
    auto state = std::exchange(drag, Idle{});

    if (auto* joint = std::get_if<JointDragging>(&state))
    {
        switchJointChordEdge(*joint, e.x, e.getMouseDownX());
        auto edge = std::move(joint->edge);
        state = fromEdgeDrag(std::move(edge));
    }

    if (const auto* resizing = std::get_if<EndResizing>(&state))
    {
        const int resizedIndex = resizing->index;

        if (!sequence)
            return;

        const bool validIndex = resizedIndex >= 0 && resizedIndex < static_cast<int>(resizing->before.size());
        if (validIndex && e.mouseWasDraggedSinceMouseDown())
            sequence->setChordChanges(ChordTrackEdits::afterResize(
                resizing->before, resizedIndex, geometry.xToTick(e.x) - resizing->grabOffset, geometry.gridTicks()));

        const auto& changes = sequence->getChordChanges();
        const bool resized = validIndex && changes != resizing->before;

        if (resized)
            performReplaceList(undoManager, sequence, "Resize Chord", resizing->before, changes);

        if (validIndex && e.mouseWasDraggedSinceMouseDown())
        {
            remapSelectionAfterResize(resizing->before, resizedIndex, ResizeEdge::Right, resizing->selectionBefore);
        }
        else if (validIndex)
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selection.selectOnly(resizedIndex);
        }

        repaint();
        return;
    }

    if (const auto* resizing = std::get_if<StartResizing>(&state))
    {
        const int movedIndex = resizing->index;

        if (!sequence)
            return;

        if (movedIndex < 0 || movedIndex >= static_cast<int>(resizing->before.size()))
        {
            repaint();
            return;
        }

        if (e.mouseWasDraggedSinceMouseDown())
            sequence->setChordChanges(ChordTrackEdits::afterStartResize(
                resizing->before, movedIndex, geometry.xToTick(e.x) - resizing->grabOffset, geometry.gridTicks()));

        const auto& changes = sequence->getChordChanges();
        if (changes != resizing->before)
            performReplaceList(undoManager, sequence, "Resize Chord", resizing->before, changes);

        if (e.mouseWasDraggedSinceMouseDown())
        {
            remapSelectionAfterResize(resizing->before, movedIndex, ResizeEdge::Left, resizing->selectionBefore);
        }
        else
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selection.selectOnly(movedIndex);
        }

        repaint();
        return;
    }

    if (const auto* moving = std::get_if<Moving>(&state))
    {
        const int movedIndex = moving->index;

        if (!sequence)
            return;

        if (movedIndex < 0 || movedIndex >= static_cast<int>(moving->before.size()))
        {
            repaint();
            return;
        }

        if (e.mouseWasDraggedSinceMouseDown())
            sequence->setChordChanges(ChordTrackEdits::afterMove(moving->before, moving->group, movedIndex,
                                                                 geometry.xToTick(e.x) - moving->grabOffset,
                                                                 geometry.gridTicks()));

        const auto& changes = sequence->getChordChanges();
        if (changes != moving->before)
        {
            performReplaceList(undoManager, sequence, "Move Chord", moving->before, changes);

            if (onSelectionTaken)
                onSelectionTaken();
            selectMovedChords(*moving, geometry.xToTick(e.x) - moving->grabOffset);
        }
        else
        {
            if (onSelectionTaken)
                onSelectionTaken();
            selection.selectOnly(movedIndex);
        }

        repaint();
        return;
    }

    if (const auto* selecting = std::get_if<RangeSelecting>(&state))
    {
        if (!e.mouseWasDraggedSinceMouseDown() && selecting->toggleIndex >= 0)
            selection.toggle(selecting->toggleIndex);
        repaint();
    }
}

void ChordStrip::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!sequence || e.mods.isRightButtonDown() || editSession.isOpen())
        return;

    if (e.y < 0 || e.y >= getHeight() || e.x < viewLeftX + labelWidth())
        return;

    drag = Idle{};

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
    const auto spelling = ChordSymbol::spellingForKeySignature(sequence->getKeySignatureAt(tick).sharpsOrFlats);

    auto content = std::make_unique<ChordEditor>(chordRoot, chordType, bassRoot, spelling, isNew);
    content->onDraftChanged = [this](int root, int type, int bass)
    {
        if (auto* draft = editSession.current())
        {
            draft->root = root;
            draft->type = type;
            draft->bassRoot = bass;
        }
        repaint();
    };
    content->onCommit = [this](int root, int type, int bass) { commitChordEdit(root, type, bass); };
    content->onCancel = [this]() { cancelChordEdit(); };

    editSession.open({tick, endTick, chordRoot, chordType, bassRoot, isNew}, std::move(content),
                     localAreaToGlobal(anchorInLocal));
    repaint();
}

void ChordStrip::commitChordEdit(int chordRoot, int chordType, int bassRoot)
{
    const auto draft = editSession.finish();

    if (!sequence)
        return;

    const int bassType = (bassRoot == ChordChange::none) ? ChordChange::none : chordType;

    if (draft.isNew)
    {
        auto after = ChordTrackEdits::afterAdd(sequence->getChordChanges(), draft.tick, draft.endTick, chordRoot,
                                               chordType, bassRoot, bassType);
        performReplaceList(undoManager, sequence, "Add Chord", sequence->getChordChanges(), std::move(after));
    }
    else
    {
        const auto& existing = sequence->getChordChanges();
        auto atTick = std::ranges::find(existing, draft.tick, &ChordChange::tick);
        if (atTick != existing.end() && atTick->chordRoot == chordRoot && atTick->chordType == chordType &&
            atTick->bassRoot == bassRoot)
        {
            repaint();
            return;
        }

        auto before = sequence->getChordChanges();
        auto after = before;
        ChordTrackEdits::add(after, draft.tick, chordRoot, chordType, bassRoot, bassType);
        performReplaceList(undoManager, sequence, "Edit Chord", std::move(before), std::move(after));
    }

    const auto& changes = sequence->getChordChanges();
    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
        if (changes[static_cast<size_t>(i)].tick == draft.tick)
            selection.selectOnly(i);
    repaint();
}

void ChordStrip::cancelChordEdit()
{
    editSession.finish();
    repaint();
}
