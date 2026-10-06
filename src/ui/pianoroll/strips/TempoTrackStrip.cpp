#include "ui/pianoroll/strips/TempoTrackStrip.h"
#include "edit/TempoEdits.h"
#include "ui/theme/Theme.h"
#include "undo/ReplaceListAction.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <utility>
#include <variant>

TempoTrackStrip::TempoTrackStrip(const TimelineGeometry& geometryRef, const DisplayedTimeline& displayedTimelineRef,
                                 EditClipboard& clipboardRef, UndoHistory& undoHistoryRef)
    : TimelineStrip(geometryRef, displayedTimelineRef, "Tempo"), clipboard(clipboardRef), undoHistory(undoHistoryRef)
{
}

void TempoTrackStrip::setSequence(MidiSequence* seq)
{
    selection.clear();
    drag = Idle{};
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
    auto after = TempoEdits::afterDelete(before, selection.indices());

    if (after.size() == before.size())
        return;

    performReplaceList(undoHistory, sequence, transactionName, std::move(before), std::move(after));

    selection.clear();
    repaint();
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

    const auto& items = clipboard.getTempoPoints();
    auto before = sequence->getTimeline().getTempoChanges();
    auto after = TempoEdits::afterPaste(before, items, atTick);

    std::vector<int> pastedTicks;
    for (const auto& p : items)
        pastedTicks.push_back(p.tick + atTick);

    const bool changed = (after != before);
    if (changed)
    {
        performReplaceList(undoHistory, sequence, "Paste Tempo Changes", std::move(before), after);
    }

    selection.selectTicks(sequence->getTimeline().getTempoChanges(), pastedTicks);

    repaint();
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

    const auto& tempoChanges = displayedChanges();
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

    if (const auto* selecting = std::get_if<RangeSelecting>(&drag))
        drawRangeBand(g, selecting->gesture, accent::soft, accent::base.withAlpha(0.6f));

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

        const auto& changes = sequence->getTimeline().getTempoChanges();
        drag = PointDragging{pointIndex, changes, selection.dragGroup(pointIndex), false, changes};
        return;
    }

    int tempoTick = 0;
    double tempoBpm = 0.0;
    if (!e.mods.isShiftDown() && hitTestTempoLine(e.x, e.y, tempoTick, tempoBpm))
    {
        const auto& changes = sequence->getTimeline().getTempoChanges();
        if (!std::ranges::contains(changes, tempoTick, &TempoChange::tick))
        {
            auto before = sequence->getTimeline().getTempoChanges();
            auto after = before;
            const int addedIndex = TempoEdits::add(after, tempoTick, tempoBpm);
            performReplaceList(undoHistory, sequence, "Add Tempo Change", std::move(before), std::move(after));
            if (onSelectionTaken)
                onSelectionTaken();
            selection.selectOnly(addedIndex);
            repaint();
        }
        return;
    }

    if (e.y >= 0 && e.y < getHeight() && e.x >= viewLeftX + labelWidth())
    {
        if (onSelectionTaken)
            onSelectionTaken();
        drag = RangeSelecting{{e.x, e.x, e.mods.isShiftDown() ? selection.indices() : std::set<int>{}}};
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

    if (auto* dragging = std::get_if<PointDragging>(&drag))
    {
        const int targetTick = std::max(0, geometry.roundTickToGrid(geometry.xToTick(e.x)));
        const double targetBpm = std::round(tempoYToBpm(e.y));
        dragging->preview = TempoEdits::afterMove(dragging->before, dragging->group, dragging->index, targetTick,
                                                  targetBpm, geometry.gridTicks());
        dragging->moved = dragging->preview != dragging->before;
        repaint();
        return;
    }

    if (auto* selecting = std::get_if<RangeSelecting>(&drag))
    {
        selecting->gesture.currentX = e.x;
        const auto& changes = sequence->getTimeline().getTempoChanges();
        selection.assign(selecting->gesture.selectionFor(static_cast<int>(changes.size()), geometry,
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
    auto state = std::exchange(drag, Idle{});

    if (const auto* dragging = std::get_if<PointDragging>(&state))
    {
        if (dragging->moved)
        {
            performReplaceList(undoHistory, sequence, "Move Tempo Change", dragging->before, dragging->preview);
            selection.assign(std::set<int>(dragging->group.begin(), dragging->group.end()));
        }
        else if (dragging->index >= 0)
        {
            selection.selectOnly(dragging->index);
        }

        repaint();
        return;
    }

    if (std::holds_alternative<RangeSelecting>(state))
    {
        repaint();
        return;
    }
}

void TempoTrackStrip::mouseMove(const juce::MouseEvent& e)
{
    setMouseCursor(sequence != nullptr && hitTestTempoPoint(e.x, e.y) >= 0 ? juce::MouseCursor::DraggingHandCursor
                                                                           : juce::MouseCursor::NormalCursor);
}

void TempoTrackStrip::cancelDrag()
{
    if (std::holds_alternative<PointDragging>(drag))
        drag = Idle{};
}

const std::vector<TempoChange>& TempoTrackStrip::displayedChanges() const
{
    if (const auto* dragging = std::get_if<PointDragging>(&drag))
        return dragging->preview;
    return sequence->getTimeline().getTempoChanges();
}

float TempoTrackStrip::tempoBpmToY(double bpm) const
{
    int graphTop = 3;
    int graphBottom = getHeight() - 4;

    double range = TimelineMap::maxBpm - TimelineMap::minBpm;
    double normalized = (bpm - TimelineMap::minBpm) / range;
    return static_cast<float>(graphBottom - normalized * (graphBottom - graphTop));
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

    const auto& changes = displayedChanges();
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
