#include "model/TimelineMap.h"
#include <algorithm>
#include <cmath>
#include <ranges>
#include <utility>

namespace
{
// tick counts converted back from seconds can land just below an integer (e.g. 1920.9999999)
// when one tick is not a binary-exact number of seconds; absorb that before truncating
int floorTicks(double ticks)
{
    return static_cast<int>(std::floor(ticks + 1e-6));
}
} // namespace

TimelineMap::TimelineMap() : tempoChanges{{0, 120.0}}, timeSignatureChanges{{0, 4, 4}} {}

int TimelineMap::getTicksPerQuarterNote() const
{
    return ticksPerQuarterNote;
}

void TimelineMap::setTicksPerQuarterNote(int ppq)
{
    ticksPerQuarterNote = ppq;
}

const std::vector<TempoChange>& TimelineMap::getTempoChanges() const
{
    return tempoChanges;
}

void TimelineMap::setTempoChanges(std::vector<TempoChange> changes)
{
    tempoChanges = std::move(changes);
}

const std::vector<TimeSignatureChange>& TimelineMap::getTimeSignatureChanges() const
{
    return timeSignatureChanges;
}

void TimelineMap::setTimeSignatureChanges(std::vector<TimeSignatureChange> changes)
{
    timeSignatureChanges = std::move(changes);
}

double TimelineMap::getTempoAt(int tick) const
{
    auto reversed = std::views::reverse(tempoChanges);
    auto it = std::ranges::find_if(reversed, [tick](const TempoChange& tc) { return tc.tick <= tick; });
    return it != reversed.end() ? it->bpm : 120.0;
}

TempoChange TimelineMap::getTempoChangeAt(int tick) const
{
    auto reversed = std::views::reverse(tempoChanges);
    auto it = std::ranges::find_if(reversed, [tick](const TempoChange& tc) { return tc.tick <= tick; });
    return it != reversed.end() ? *it : TempoChange{0, 120.0};
}

TimeSignatureChange TimelineMap::getTimeSignatureAt(int tick) const
{
    auto reversed = std::views::reverse(timeSignatureChanges);
    auto it = std::ranges::find_if(reversed, [tick](const TimeSignatureChange& ts) { return ts.tick <= tick; });
    return it != reversed.end() ? *it : TimeSignatureChange{0, 4, 4};
}

double TimelineMap::ticksToSeconds(int ticks) const
{
    double seconds = 0.0;
    int prevTick = 0;
    double currentBpm = 120.0;

    for (const auto& tc : tempoChanges)
    {
        if (tc.tick >= ticks)
            break;

        if (tc.tick > prevTick)
        {
            double ticksPerSecond = (currentBpm / 60.0) * ticksPerQuarterNote;
            seconds += (tc.tick - prevTick) / ticksPerSecond;
            prevTick = tc.tick;
        }

        currentBpm = tc.bpm;
    }

    double ticksPerSecond = (currentBpm / 60.0) * ticksPerQuarterNote;
    seconds += (ticks - prevTick) / ticksPerSecond;

    return seconds;
}

int TimelineMap::secondsToTicks(double seconds) const
{
    double accSeconds = 0.0;
    int prevTick = 0;
    double currentBpm = 120.0;

    for (const auto& tc : tempoChanges)
    {
        double ticksPerSecond = (currentBpm / 60.0) * ticksPerQuarterNote;
        double segmentSeconds = (tc.tick - prevTick) / ticksPerSecond;

        if (accSeconds + segmentSeconds >= seconds)
        {
            double remainingSeconds = seconds - accSeconds;
            return prevTick + floorTicks(remainingSeconds * ticksPerSecond);
        }

        accSeconds += segmentSeconds;
        prevTick = tc.tick;
        currentBpm = tc.bpm;
    }

    double ticksPerSecond = (currentBpm / 60.0) * ticksPerQuarterNote;
    double remainingSeconds = seconds - accSeconds;
    return prevTick + floorTicks(remainingSeconds * ticksPerSecond);
}

BarBeatTick TimelineMap::tickToBarBeatTick(int tick) const
{
    int bar = 1;
    int pos = 0;

    for (size_t i = 0; i < timeSignatureChanges.size(); ++i)
    {
        const auto& ts = timeSignatureChanges[i];
        int ticksPerBeat = ticksPerQuarterNote * 4 / ts.denominator;
        int ticksPerBar = ticksPerBeat * ts.numerator;

        int nextChangeTick = (i + 1 < timeSignatureChanges.size()) ? timeSignatureChanges[i + 1].tick : tick + 1;

        if (tick < nextChangeTick)
        {
            int ticksInThisSection = tick - pos;
            int barsInSection = ticksInThisSection / ticksPerBar;
            int remainder = ticksInThisSection % ticksPerBar;
            bar += barsInSection;
            int beat = remainder / ticksPerBeat + 1;
            int tickInBeat = remainder % ticksPerBeat;
            return {bar, beat, tickInBeat};
        }

        int sectionTicks = nextChangeTick - pos;
        int barsInSection = sectionTicks / ticksPerBar;
        bar += barsInSection;
        pos = nextChangeTick;
    }

    return {bar, 1, 0};
}

int TimelineMap::barStartToTick(int targetBar) const
{
    if (targetBar <= 1)
        return 0;

    int bar = 1;
    int pos = 0;

    for (size_t i = 0; i < timeSignatureChanges.size(); ++i)
    {
        const auto& ts = timeSignatureChanges[i];
        int ticksPerBeat = ticksPerQuarterNote * 4 / ts.denominator;
        int ticksPerBar = ticksPerBeat * ts.numerator;

        if (i + 1 < timeSignatureChanges.size())
        {
            int nextChangeTick = timeSignatureChanges[i + 1].tick;
            int sectionTicks = nextChangeTick - pos;
            int barsInSection = sectionTicks / ticksPerBar;

            if (bar + barsInSection >= targetBar)
                return pos + (targetBar - bar) * ticksPerBar;

            bar += barsInSection;
            pos = nextChangeTick;
        }
        else
        {
            return pos + (targetBar - bar) * ticksPerBar;
        }
    }

    return 0;
}

int TimelineMap::barBeatTickToTick(int bar, int beat, int tickInBeat) const
{
    bar = std::max(1, bar);

    int barStart = barStartToTick(bar);

    auto ts = getTimeSignatureAt(barStart);
    int ticksPerBeat = ticksPerQuarterNote * 4 / ts.denominator;

    return std::max(0, barStart + (beat - 1) * ticksPerBeat + tickInBeat);
}
