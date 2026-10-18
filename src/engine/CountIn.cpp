#include "engine/CountIn.h"
#include <algorithm>
#include <cstddef>

namespace
{
int floorMod(int value, int divisor)
{
    const int remainder = value % divisor;
    return remainder < 0 ? remainder + divisor : remainder;
}

struct Section
{
    TimeSignatureChange signature{0, 4, 4};
    int start = 0;
};

Section sectionAt(const std::vector<TimeSignatureChange>& timeSignatures, int tick)
{
    Section section;
    for (std::size_t i = 0; i < timeSignatures.size() && (i == 0 || timeSignatures[i].tick <= tick); ++i)
    {
        section.signature = timeSignatures[i];
        // Like metronomeClicksInRange, the first time signature counts from tick 0.
        section.start = i == 0 ? 0 : section.signature.tick;
    }
    return section;
}
} // namespace

int barStartAtOrBefore(const std::vector<TimeSignatureChange>& timeSignatures, int ticksPerQuarterNote, int tick)
{
    const auto [signature, sectionStart] = sectionAt(timeSignatures, tick);
    const int ticksPerBar = signature.numerator * (ticksPerQuarterNote * 4 / signature.denominator);
    return tick - floorMod(tick - sectionStart, ticksPerBar);
}

CountIn makeCountIn(const std::vector<TimeSignatureChange>& timeSignatures, int ticksPerQuarterNote, int startTick,
                    int bars)
{
    const auto [signature, sectionStart] = sectionAt(timeSignatures, startTick);
    const int ticksPerBeat = ticksPerQuarterNote * 4 / signature.denominator;
    const int lengthTicks = bars * signature.numerator * ticksPerBeat;
    const int countInStart = startTick - lengthTicks;
    const int firstBeatTick = floorMod(sectionStart - countInStart, ticksPerBeat);
    const int firstBeatIndex = (countInStart + firstBeatTick - sectionStart) / ticksPerBeat;
    return {lengthTicks, ticksPerBeat, signature.numerator, firstBeatTick,
            floorMod(firstBeatIndex, signature.numerator)};
}

std::vector<MetronomeClick> countInClicksInRange(const CountIn& countIn, int fromTick, int toTick)
{
    std::vector<MetronomeClick> clicks;
    const int end = std::min(toTick, countIn.lengthTicks);
    int beat = fromTick <= countIn.firstBeatTick
                   ? 0
                   : (fromTick - countIn.firstBeatTick + countIn.ticksPerBeat - 1) / countIn.ticksPerBeat;
    for (int tick = countIn.firstBeatTick + beat * countIn.ticksPerBeat; tick < end;
         tick += countIn.ticksPerBeat, ++beat)
        clicks.push_back({tick, (countIn.firstBeatInBar + beat) % countIn.beatsPerBar == 0});
    return clicks;
}
