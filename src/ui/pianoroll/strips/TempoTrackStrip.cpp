#include "ui/pianoroll/strips/TempoTrackStrip.h"
#include "edit/TempoEdits.h"
#include "ui/theme/Theme.h"
#include "undo/ReplaceListAction.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

TempoTrackStrip::TempoTrackStrip(const TimelineGeometry& geometryRef, EditClipboard& clipboardRef,
                                 juce::UndoManager& undoManagerRef)
    : TimelineStrip(geometryRef, "Tempo"), clipboard(clipboardRef), undoManager(undoManagerRef)
{
}

void TempoTrackStrip::setSequence(MidiSequence* seq)
{
    selection.clear();
    isTempoRangeSelecting = false;
    TimelineStrip::setSequence(seq);
}

bool TempoTrackStrip::hasSelection() const
{
    return !selection.isEmpty();
}

void TempoTrackStrip::clearTempoSelection()
{
    if (selection.isEmpty())
        return;
    selection.clear();
    repaint();
}

void TempoTrackStrip::deleteSelectedTempoPoints()
{
    deleteSelectedTempoPointsImpl("Delete Tempo Changes");
}

void TempoTrackStrip::deleteSelectedTempoPointsImpl(const juce::String& transactionName)
{
    if (!sequence || selection.isEmpty())
        return;

    auto before = sequence->getTimeline().getTempoChanges();
    const int count = static_cast<int>(before.size());

    std::vector<TempoChange> after;
    after.reserve(before.size());
    for (int i = 0; i < count; ++i)
    {
        const bool remove = selection.contains(i) && before[i].tick != 0;
        if (!remove)
            after.push_back(before[i]);
    }

    if (after.size() == before.size())
        return;

    undoManager.beginNewTransaction(transactionName);
    undoManager.perform(new ReplaceListAction<TempoChange>(sequence, std::move(before), std::move(after)));

    selection.clear();
    repaint();
    if (onTempoChanged)
        onTempoChanged();
}

void TempoTrackStrip::copySelectedTempoPoints()
{
    if (!sequence || selection.isEmpty())
        return;

    const auto& changes = sequence->getTimeline().getTempoChanges();
    const int count = static_cast<int>(changes.size());

    std::vector<TempoChange> points;
    for (int i : selection.indices())
        if (i >= 0 && i < count)
            points.push_back(changes[i]);

    if (points.empty())
        return;

    const int minTick = points.front().tick;
    for (auto& p : points)
        p.tick -= minTick;

    clipboard.setTempoPoints(std::move(points));
}

void TempoTrackStrip::cutSelectedTempoPoints()
{
    if (!sequence || selection.isEmpty())
        return;

    copySelectedTempoPoints();
    deleteSelectedTempoPointsImpl("Cut Tempo Changes");
}

void TempoTrackStrip::pasteTempoPoints(int atTick)
{
    if (!sequence || !clipboard.hasTempoPoints())
        return;

    auto before = sequence->getTimeline().getTempoChanges();
    auto after = before;

    std::vector<int> pastedTicks;
    for (const auto& p : clipboard.getTempoPoints())
    {
        const int tick = p.tick + atTick;
        pastedTicks.push_back(tick);
        auto it = std::ranges::find(after, tick, &TempoChange::tick);
        if (it != after.end())
            it->bpm = p.bpm;
        else
            after.push_back({tick, p.bpm});
    }
    std::ranges::sort(after, {}, &TempoChange::tick);

    const bool changed = (after != before);
    if (changed)
    {
        undoManager.beginNewTransaction("Paste Tempo Changes");
        undoManager.perform(new ReplaceListAction<TempoChange>(sequence, std::move(before), after));
    }

    selection.selectTicks(sequence->getTimeline().getTempoChanges(), pastedTicks);

    repaint();
    if (changed && onTempoChanged)
        onTempoChanged();
}

float TempoTrackStrip::tempoBpmToY(double bpm) const
{
    int graphTop = 3;
    int graphBottom = getHeight() - 4;

    double range = TimelineMap::maxBpm - TimelineMap::minBpm;
    double normalized = (bpm - TimelineMap::minBpm) / range;
    return static_cast<float>(graphBottom - normalized * (graphBottom - graphTop));
}

bool TempoTrackStrip::hitTestTempoLine(int x, int y, int& outTick, double& outBpm) const
{
    if (!sequence)
        return false;

    if (y < 0 || y >= getHeight())
        return false;

    if (x < viewLeftX + labelWidth())
        return false;

    constexpr int tolerance = 5;
    double activeBpm = sequence->getTimeline().getTempoAt(geometry.xToTick(x));
    float lineY = tempoBpmToY(activeBpm);
    if (std::abs(static_cast<float>(y) - lineY) > tolerance)
        return false;

    int tick = std::max(0, geometry.roundTickToGrid(geometry.xToTick(x)));
    outTick = tick;
    outBpm = sequence->getTimeline().getTempoAt(tick);
    return true;
}

double TempoTrackStrip::tempoYToBpm(int y) const
{
    int graphTop = 3;
    int graphBottom = getHeight() - 4;

    double range = TimelineMap::maxBpm - TimelineMap::minBpm;
    double normalized = static_cast<double>(graphBottom - y) / static_cast<double>(graphBottom - graphTop);
    return TimelineMap::minBpm + normalized * range;
}

int TempoTrackStrip::hitTestTempoPoint(int x, int y) const
{
    if (!sequence)
        return -1;

    if (y < 0 || y >= getHeight())
        return -1;

    if (x < viewLeftX + labelWidth())
        return -1;

    const auto& changes = sequence->getTimeline().getTempoChanges();
    constexpr float hitRadius = 6.0f;
    float bandTop = 3.0f;
    float bandBottom = static_cast<float>(getHeight() - 4);

    for (int i = 0; i < static_cast<int>(changes.size()); ++i)
    {
        float px = static_cast<float>(geometry.tickToX(changes[i].tick));
        float py = std::clamp(tempoBpmToY(changes[i].bpm), bandTop, bandBottom);
        float dx = static_cast<float>(x) - px;
        float dy = static_cast<float>(y) - py;
        if (dx * dx + dy * dy <= hitRadius * hitRadius)
            return i;
    }
    return -1;
}

void TempoTrackStrip::paint(juce::Graphics& g)
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

    const auto& tempoChanges = sequence->getTimeline().getTempoChanges();
    juce::Colour amberColour = accent::base;
    if (tempoChanges.empty())
    {
        float y = tempoBpmToY(120.0);
        g.setColour(amberColour);
        g.drawLine(static_cast<float>(visibleLeft), y, static_cast<float>(visibleRight), y, 1.5f);

        g.setFont(font::sans(font::size2XS));
        g.drawText("120.00", viewLeftX + labelWidth() + 4, 0, 60, getHeight(), juce::Justification::centredLeft);
    }
    else
    {
        g.setColour(amberColour);

        int totalTicks = geometry.xToTick(getWidth());
        float bandTop = 3.0f;
        float bandBottom = static_cast<float>(getHeight() - 4);

        for (size_t i = 0; i < tempoChanges.size(); ++i)
        {
            int startX = geometry.tickToX(tempoChanges[i].tick);
            int endX = (i + 1 < tempoChanges.size()) ? geometry.tickToX(tempoChanges[i + 1].tick)
                                                     : geometry.tickToX(totalTicks);
            float y = std::clamp(tempoBpmToY(tempoChanges[i].bpm), bandTop, bandBottom);

            int drawStartX = std::max(startX, visibleLeft);
            int drawEndX = std::min(endX, visibleRight);

            if (drawEndX < visibleLeft || drawStartX > visibleRight)
                continue;

            bool selected = selection.contains(static_cast<int>(i));
            juce::Colour lineColour = selected ? amberColour.brighter(0.5f) : amberColour;

            g.setColour(lineColour);
            g.drawLine(static_cast<float>(drawStartX), y, static_cast<float>(drawEndX), y, 1.5f);

            if (i > 0 && startX >= visibleLeft && startX <= visibleRight)
            {
                float prevY = std::clamp(tempoBpmToY(tempoChanges[i - 1].bpm), bandTop, bandBottom);
                g.drawLine(static_cast<float>(startX), prevY, static_cast<float>(startX), y, 1.0f);
            }

            if (startX >= visibleLeft - 4 && startX <= visibleRight + 4)
            {
                float radius = selected ? 4.0f : 3.0f;
                juce::Rectangle<float> dot(static_cast<float>(startX) - radius, y - radius, radius * 2.0f,
                                           radius * 2.0f);
                g.setColour(surface::surface2);
                g.fillEllipse(dot.expanded(1.0f));
                g.setColour(lineColour);
                g.fillEllipse(dot);
                if (selected)
                {
                    g.setColour(text::t1);
                    g.drawEllipse(dot, 1.5f);
                }
            }

            if (startX + 4 >= visibleLeft - 60 && startX <= visibleRight)
            {
                g.setColour(amberColour);
                g.setFont(font::sans(font::size2XS));
                int textH = 12;
                float midY = getHeight() * 0.5f;
                int textY;
                if (y > midY)
                    textY = 1;
                else
                    textY = getHeight() - textH - 2;
                g.drawText(juce::String(tempoChanges[i].bpm, 1), startX + 4, textY, 50, textH,
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

    if (isTempoRangeSelecting)
        drawRangeBand(g, rangeSelect, accent::soft, accent::base.withAlpha(0.6f));

    drawLoopOverlay(g, 0, getHeight(), 0.12f);

    g.restoreState();

    drawLabelColumn(g);
}

void TempoTrackStrip::mouseDown(const juce::MouseEvent& e)
{
    if (sequence == nullptr || sequence->getNumTracks() == 0)
        return;
    if (e.mods.isRightButtonDown())
        return;

    int pointIndex = hitTestTempoPoint(e.x, e.y);
    if (pointIndex >= 0)
    {
        if (onSelectionTaken)
            onSelectionTaken();
        if (e.mods.isShiftDown())
        {
            selection.toggle(pointIndex);
            repaint();
            return;
        }

        tempoDragBefore = sequence->getTimeline().getTempoChanges();
        tempoDragIndex = pointIndex;
        isTempoPointDragging = true;
        tempoDragMoved = false;

        tempoDragGroup = selection.dragGroup(pointIndex);
        return;
    }

    int tempoTick = 0;
    double tempoBpm = 0.0;
    if (!e.mods.isShiftDown() && hitTestTempoLine(e.x, e.y, tempoTick, tempoBpm))
    {
        const auto& changes = sequence->getTimeline().getTempoChanges();
        bool exists = std::any_of(changes.begin(), changes.end(),
                                  [tempoTick](const TempoChange& tc) { return tc.tick == tempoTick; });
        if (!exists)
        {
            undoManager.beginNewTransaction("Add Tempo Change");
            auto before = sequence->getTimeline().getTempoChanges();
            auto after = before;
            const int addedIndex = TempoEdits::add(after, tempoTick, tempoBpm);
            undoManager.perform(new ReplaceListAction<TempoChange>(sequence, std::move(before), std::move(after)));
            if (onSelectionTaken)
                onSelectionTaken();
            selection.selectOnly(addedIndex);
            repaint();
            if (onTempoChanged)
                onTempoChanged();
        }
        return;
    }

    if (e.y >= 0 && e.y < getHeight() && e.x >= viewLeftX + labelWidth())
    {
        if (onSelectionTaken)
            onSelectionTaken();
        isTempoRangeSelecting = true;
        rangeSelect = {e.x, e.x, e.mods.isShiftDown() ? selection.indices() : std::set<int>{}};
        if (!e.mods.isShiftDown())
            selection.clear();
        repaint();
        return;
    }
}

void TempoTrackStrip::mouseDrag(const juce::MouseEvent& e)
{
    if (sequence == nullptr)
        return;

    if (isTempoPointDragging)
    {
        const int count = static_cast<int>(tempoDragBefore.size());
        if (tempoDragIndex < 0 || tempoDragIndex >= count)
            return;

        const int grid = geometry.gridTicks();

        std::set<int> moving;
        for (int i : tempoDragGroup)
            if (i >= 0 && i < count && tempoDragBefore[i].tick != 0)
                moving.insert(i);

        const TempoChange& dragOrig = tempoDragBefore[tempoDragIndex];
        int deltaTick =
            (dragOrig.tick == 0) ? 0 : (std::max(0, geometry.roundTickToGrid(geometry.xToTick(e.x))) - dragOrig.tick);
        double deltaBpm = std::round(tempoYToBpm(e.y)) - dragOrig.bpm;

        int deltaLo = std::numeric_limits<int>::min();
        int deltaHi = std::numeric_limits<int>::max();
        for (int i : moving)
            deltaLo = std::max(deltaLo, grid - tempoDragBefore[i].tick);
        for (int i = 0; i + 1 < count; ++i)
        {
            bool aMoving = moving.count(i) > 0;
            bool bMoving = moving.count(i + 1) > 0;
            int gap = tempoDragBefore[i + 1].tick - tempoDragBefore[i].tick;
            if (bMoving && !aMoving)
                deltaLo = std::max(deltaLo, grid - gap);
            else if (aMoving && !bMoving)
                deltaHi = std::min(deltaHi, gap - grid);
        }
        deltaTick = (deltaLo > deltaHi) ? 0 : std::clamp(deltaTick, deltaLo, deltaHi);

        double groupMinBpm = TimelineMap::maxBpm;
        double groupMaxBpm = TimelineMap::minBpm;
        for (int i : tempoDragGroup)
            if (i >= 0 && i < count)
            {
                groupMinBpm = std::min(groupMinBpm, tempoDragBefore[i].bpm);
                groupMaxBpm = std::max(groupMaxBpm, tempoDragBefore[i].bpm);
            }
        deltaBpm = juce::jlimit(TimelineMap::minBpm - groupMinBpm, TimelineMap::maxBpm - groupMaxBpm, deltaBpm);

        auto changes = tempoDragBefore;
        for (int i : tempoDragGroup)
            if (i >= 0 && i < count)
                changes[i].bpm = tempoDragBefore[i].bpm + deltaBpm;
        for (int i : moving)
            changes[i].tick = tempoDragBefore[i].tick + deltaTick;
        sequence->setTempoChanges(changes);

        tempoDragMoved = (deltaTick != 0) || (deltaBpm != 0.0);
        repaint();
        return;
    }

    if (isTempoRangeSelecting)
    {
        rangeSelect.currentX = e.x;
        const auto& changes = sequence->getTimeline().getTempoChanges();
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

void TempoTrackStrip::mouseUp(const juce::MouseEvent&)
{
    if (isTempoPointDragging)
    {
        isTempoPointDragging = false;
        int draggedIndex = tempoDragIndex;
        tempoDragIndex = -1;

        if (tempoDragMoved)
        {
            auto after = sequence->getTimeline().getTempoChanges();
            undoManager.beginNewTransaction("Move Tempo Change");
            undoManager.perform(new ReplaceListAction<TempoChange>(sequence, tempoDragBefore, after));
            selection.assign(std::set<int>(tempoDragGroup.begin(), tempoDragGroup.end()));
            if (onTempoChanged)
                onTempoChanged();
        }
        else if (draggedIndex >= 0)
        {
            selection.selectOnly(draggedIndex);
        }

        tempoDragBefore.clear();
        tempoDragGroup.clear();
        tempoDragMoved = false;
        repaint();
        return;
    }

    if (isTempoRangeSelecting)
    {
        isTempoRangeSelecting = false;
        rangeSelect = {};
        repaint();
        return;
    }
}

void TempoTrackStrip::mouseMove(const juce::MouseEvent& e)
{
    setMouseCursor(sequence != nullptr && hitTestTempoPoint(e.x, e.y) >= 0 ? juce::MouseCursor::DraggingHandCursor
                                                                           : juce::MouseCursor::NormalCursor);
}
