#include "engine/MetronomeClicks.h"
#include <algorithm>
#include <cstddef>
#include <limits>

std::vector<MetronomeClick> metronomeClicksInRange(const std::vector<TimeSignatureChange>& timeSignatures,
                                                   int ticksPerQuarterNote, int fromTick, int toTick)
{
    std::vector<MetronomeClick> clicks;
    if (fromTick >= toTick)
        return clicks;

    static const std::vector<TimeSignatureChange> fourFour{{0, 4, 4}};
    const auto& sigs = timeSignatures.empty() ? fourFour : timeSignatures;

    for (std::size_t i = 0; i < sigs.size(); ++i)
    {
        const int sectionStart = i == 0 ? 0 : sigs[i].tick;
        const int sectionEnd = i + 1 < sigs.size() ? sigs[i + 1].tick : std::numeric_limits<int>::max();
        const int begin = std::max(fromTick, sectionStart);
        const int end = std::min(toTick, sectionEnd);
        if (begin >= end)
            continue;

        const int ticksPerBeat = ticksPerQuarterNote * 4 / sigs[i].denominator;
        int beatIndex = (begin - sectionStart + ticksPerBeat - 1) / ticksPerBeat;
        for (int tick = sectionStart + beatIndex * ticksPerBeat; tick < end; tick += ticksPerBeat, ++beatIndex)
            clicks.push_back({tick, beatIndex % sigs[i].numerator == 0});
    }
    return clicks;
}
