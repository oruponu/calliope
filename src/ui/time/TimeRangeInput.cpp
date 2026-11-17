#include "ui/time/TimeRangeInput.h"
#include "ui/eventlist/EventListPosition.h"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <system_error>

namespace
{
std::optional<int> parseWhole(std::string_view text, int minimum)
{
    const auto isSpace = [](char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; };
    while (!text.empty() && isSpace(text.front()))
        text.remove_prefix(1);
    while (!text.empty() && isSpace(text.back()))
        text.remove_suffix(1);
    // from_chars would accept a leading minus sign.
    if (text.empty() || !std::ranges::all_of(text, [](char c) { return c >= '0' && c <= '9'; }))
        return std::nullopt;

    int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || value < minimum)
        return std::nullopt;
    return value;
}
} // namespace

std::optional<BarBeatTick> TimeRangeInput::parse(std::string_view bar, std::string_view beat, std::string_view tick)
{
    const auto parsedBar = parseWhole(bar, 1);
    const auto parsedBeat = parseWhole(beat, 1);
    const auto parsedTick = parseWhole(tick, 0);
    if (!parsedBar || !parsedBeat || !parsedTick || *parsedBar > maxBar)
        return std::nullopt;
    return BarBeatTick{*parsedBar, *parsedBeat, *parsedTick};
}

BarBeatTick TimeRangeInput::keptInside(const BarBeatTick& position, const TimelineMap& timeline)
{
    return timeline.tickToBarBeatTick(EventListPosition::toTick(position, timeline));
}

BarBeatTick TimeRangeInput::initialEnd(const BarBeatTick& start, const TimelineMap& timeline)
{
    return timeline.tickToBarBeatTick(EventListPosition::toTick({start.bar + 1, start.beat, start.tick}, timeline));
}

BarBeatTick TimeRangeInput::initialStart(int tick, const TimelineMap& timeline)
{
    return timeline.tickToBarBeatTick(EventListPosition::toTick(timeline.tickToBarBeatTick(tick), timeline));
}
