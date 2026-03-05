#include "ControllerLaneComponent.h"
#include "TrackColours.h"
#include <algorithm>

void ControllerLaneComponent::setSequence(MidiSequence* seq)
{
    sequence = seq;

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
        contentBeats = std::max(contentBeats, lastTick / sequence->getTicksPerQuarterNote() + 4);
    }

    updateSize();
    repaint();
}

void ControllerLaneComponent::setSelectedTracks(int activeIndex, const std::set<int>& selectedIndices)
{
    activeTrackIndex = activeIndex;
    selectedTrackIndices = selectedIndices;
    repaint();
}

void ControllerLaneComponent::setDisplayMode(DisplayMode mode)
{
    displayMode = mode;
    repaint();
}

void ControllerLaneComponent::setCCNumber(int cc)
{
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

void ControllerLaneComponent::setContentBeats(int beats)
{
    contentBeats = beats;
    updateSize();
}

void ControllerLaneComponent::updateSize()
{
    if (!sequence)
        return;

    int width = leftPanelWidth + contentBeats * beatWidth;

    if (auto* vp = findParentComponentOfClass<juce::Viewport>())
        width = std::max(width, vp->getMaximumVisibleWidth());

    setSize(width, getHeight());
}

int ControllerLaneComponent::tickToX(int tick) const
{
    if (!sequence)
        return leftPanelWidth;

    double beatsFromTick = static_cast<double>(tick) / sequence->getTicksPerQuarterNote();
    return leftPanelWidth + static_cast<int>(beatsFromTick * beatWidth);
}

int ControllerLaneComponent::xToTick(int x) const
{
    if (!sequence)
        return 0;

    double beats = static_cast<double>(x - leftPanelWidth) / beatWidth;
    return static_cast<int>(beats * sequence->getTicksPerQuarterNote());
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

void ControllerLaneComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(25, 25, 35));

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
    }

    drawPlayhead(g);
    drawLeftPanel(g);
}

void ControllerLaneComponent::drawLeftPanel(juce::Graphics& g)
{
    auto leftRect = juce::Rectangle<int>(0, 0, leftPanelWidth, getHeight());

    auto* vp = findParentComponentOfClass<juce::Viewport>();
    if (vp)
        leftRect.setX(vp->getViewPositionX());

    g.setColour(juce::Colour(35, 35, 48));
    g.fillRect(leftRect);

    g.setColour(juce::Colour(55, 55, 65));
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
    }

    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText(label, leftRect.reduced(4, 0), juce::Justification::centred, true);

    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.setColour(juce::Colours::white.withAlpha(0.4f));

    int lx = leftRect.getX() + 2;
    int lw = leftPanelWidth - 6;

    if (displayMode == DisplayMode::PitchBend)
    {
        g.drawText("+8191", lx, getDrawAreaTop() - 5, lw, 10, juce::Justification::right);
        g.drawText("0", lx, (getDrawAreaTop() + getDrawAreaBottom()) / 2 - 5, lw, 10, juce::Justification::right);
        g.drawText("-8192", lx, getDrawAreaBottom() - 5, lw, 10, juce::Justification::right);
    }
    else
    {
        g.drawText("127", lx, getDrawAreaTop() - 5, lw, 10, juce::Justification::right);
        g.drawText("0", lx, getDrawAreaBottom() - 5, lw, 10, juce::Justification::right);
    }
}

void ControllerLaneComponent::drawGrid(juce::Graphics& g)
{
    if (!sequence)
        return;

    auto* vp = findParentComponentOfClass<juce::Viewport>();
    int visibleLeft = vp ? vp->getViewPositionX() : 0;
    int visibleRight = vp ? visibleLeft + vp->getViewWidth() : getWidth();

    if (displayMode == DisplayMode::PitchBend)
    {
        int centerY = (getDrawAreaTop() + getDrawAreaBottom()) / 2;
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.drawHorizontalLine(centerY, static_cast<float>(leftPanelWidth), static_cast<float>(getWidth()));
    }
    else
    {
        for (int v : {32, 64, 96})
        {
            int y = valueToY(v);
            float alpha = (v == 64) ? 0.12f : 0.06f;
            g.setColour(juce::Colours::white.withAlpha(alpha));
            g.drawHorizontalLine(y, static_cast<float>(leftPanelWidth), static_cast<float>(getWidth()));
        }
    }

    int ppq = sequence->getTicksPerQuarterNote();
    int startTick = xToTick(std::max(visibleLeft, leftPanelWidth));
    int firstBeat = std::max(0, startTick / ppq);

    for (int beat = firstBeat;; ++beat)
    {
        int tick = beat * ppq;
        int x = tickToX(tick);
        if (x > visibleRight)
            break;
        if (x < leftPanelWidth)
            continue;

        auto bbt = sequence->tickToBarBeatTick(tick);

        if (bbt.beat == 1 && bbt.tick == 0)
            g.setColour(juce::Colours::white.withAlpha(0.18f));
        else
            g.setColour(juce::Colours::white.withAlpha(0.06f));

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

    for (int trackIdx : selectedTrackIndices)
    {
        if (trackIdx < 0 || trackIdx >= sequence->getNumTracks())
            continue;

        const auto& track = sequence->getTrack(trackIdx);
        auto colour = TrackColours::getColour(trackIdx);
        bool isActive = (trackIdx == activeTrackIndex);
        float alpha = isActive ? 0.85f : 0.3f;

        for (int i = 0; i < track.getNumNotes(); ++i)
        {
            const auto& note = track.getNote(i);
            int x = tickToX(note.startTick);

            if (x + velocityBarWidth < visibleLeft || x > visibleRight)
                continue;

            int barHeight = static_cast<int>(static_cast<float>(note.velocity) / 127.0f * getDrawAreaHeight());
            int barTop = bottom - barHeight;

            g.setColour(colour.withAlpha(alpha));
            g.fillRect(x, barTop, velocityBarWidth, barHeight);

            if (isActive)
            {
                g.setColour(colour.brighter(0.3f).withAlpha(0.6f));
                g.drawRect(x, barTop, velocityBarWidth, barHeight, 1);
            }
        }
    }
}

void ControllerLaneComponent::drawControlChange(juce::Graphics& g)
{
    if (!sequence)
        return;

    for (int trackIdx : selectedTrackIndices)
    {
        if (trackIdx < 0 || trackIdx >= sequence->getNumTracks())
            continue;

        const auto& track = sequence->getTrack(trackIdx);

        std::vector<std::pair<int, int>> events;
        for (int i = 0; i < track.getNumEvents(); ++i)
        {
            const auto& event = track.getEvent(i);
            if (event.type == MidiEvent::Type::ControlChange && event.data1 == ccNumber)
                events.push_back({event.tick, event.data2});
        }

        if (events.empty())
            continue;

        auto colour = TrackColours::getColour(trackIdx);
        float alpha = (trackIdx == activeTrackIndex) ? 0.85f : 0.3f;
        drawStepGraph(g, events, colour, alpha, [this](int v) { return valueToY(v); });
    }
}

void ControllerLaneComponent::drawPitchBend(juce::Graphics& g)
{
    if (!sequence)
        return;

    int centerY = (getDrawAreaTop() + getDrawAreaBottom()) / 2;
    int halfHeight = getDrawAreaHeight() / 2;

    for (int trackIdx : selectedTrackIndices)
    {
        if (trackIdx < 0 || trackIdx >= sequence->getNumTracks())
            continue;

        const auto& track = sequence->getTrack(trackIdx);

        std::vector<std::pair<int, int>> events;
        for (int i = 0; i < track.getNumEvents(); ++i)
        {
            const auto& event = track.getEvent(i);
            if (event.type == MidiEvent::Type::PitchBend)
                events.push_back({event.tick, event.data1});
        }

        if (events.empty())
            continue;

        auto colour = TrackColours::getColour(trackIdx);
        float alpha = (trackIdx == activeTrackIndex) ? 0.85f : 0.3f;
        drawStepGraph(g, events, colour, alpha,
                      [centerY, halfHeight](int v)
                      {
                          float signed_ = static_cast<float>(v - 8192) / 8192.0f;
                          return centerY - static_cast<int>(signed_ * halfHeight);
                      });
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
    if (!sequence)
        return;

    int x = tickToX(static_cast<int>(playheadTick));
    if (x < leftPanelWidth)
        return;

    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawVerticalLine(x, 0.0f, static_cast<float>(getHeight()));
}

void ControllerLaneComponent::mouseDown(const juce::MouseEvent& e)
{
    if (!sequence)
        return;
    if (activeTrackIndex < 0 || activeTrackIndex >= sequence->getNumTracks())
        return;

    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Velocity", true, displayMode == DisplayMode::Velocity);

        juce::PopupMenu ccMenu;
        struct CCEntry
        {
            int cc;
            const char* name;
        };
        CCEntry commonCCs[] = {
            {1, "CC 1 (Modulation)"}, {7, "CC 7 (Volume)"},   {10, "CC 10 (Pan)"},    {11, "CC 11 (Expression)"},
            {64, "CC 64 (Sustain)"},  {91, "CC 91 (Reverb)"}, {93, "CC 93 (Chorus)"},
        };
        for (const auto& cc : commonCCs)
            ccMenu.addItem(100 + cc.cc, cc.name, true, displayMode == DisplayMode::ControlChange && ccNumber == cc.cc);
        menu.addSubMenu("Control Change", ccMenu);

        menu.addItem(2, "Pitch Bend", true, displayMode == DisplayMode::PitchBend);

        auto screenPos = e.getScreenPosition();
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({screenPos.x, screenPos.y, 1, 1}),
                           [this](int result)
                           {
                               if (result == 1)
                                   setDisplayMode(DisplayMode::Velocity);
                               else if (result == 2)
                                   setDisplayMode(DisplayMode::PitchBend);
                               else if (result >= 100)
                               {
                                   setCCNumber(result - 100);
                                   setDisplayMode(DisplayMode::ControlChange);
                               }
                           });
        return;
    }

    if (e.x < leftPanelWidth)
        return;

    if (displayMode != DisplayMode::Velocity)
        return;

    auto& track = sequence->getTrack(activeTrackIndex);
    int newVelocity = yToValue(e.y);

    int bestIdx = -1;
    int bestDist = INT_MAX;
    for (int i = 0; i < track.getNumNotes(); ++i)
    {
        int nx = tickToX(track.getNote(i).startTick);
        if (e.x >= nx && e.x < nx + velocityBarWidth)
        {
            int dist = std::abs(nx - e.x);
            if (dist < bestDist)
            {
                bestDist = dist;
                bestIdx = i;
            }
        }
    }

    if (bestIdx >= 0)
    {
        track.getNote(bestIdx).velocity = newVelocity;
        isDragging = true;
        lastDragX = e.x;
        repaint();
        if (onDataChanged)
            onDataChanged();
    }
}

void ControllerLaneComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging || !sequence)
        return;
    if (activeTrackIndex < 0 || activeTrackIndex >= sequence->getNumTracks())
        return;

    if (displayMode != DisplayMode::Velocity)
        return;

    auto& track = sequence->getTrack(activeTrackIndex);
    int newVelocity = yToValue(e.y);

    int startX = std::min(lastDragX, e.x);
    int endX = std::max(lastDragX, e.x);

    bool changed = false;
    for (int i = 0; i < track.getNumNotes(); ++i)
    {
        int nx = tickToX(track.getNote(i).startTick);
        if (nx + velocityBarWidth >= startX && nx <= endX)
        {
            track.getNote(i).velocity = newVelocity;
            changed = true;
        }
    }

    lastDragX = e.x;

    if (changed)
    {
        repaint();
        if (onDataChanged)
            onDataChanged();
    }
}

void ControllerLaneComponent::mouseUp(const juce::MouseEvent&)
{
    isDragging = false;
    lastDragX = -1;
}

void ControllerLaneComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (onMouseWheel)
        onMouseWheel(e, w);
    else
        Component::mouseWheelMove(e, w);
}
