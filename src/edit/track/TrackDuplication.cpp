#include "edit/track/TrackDuplication.h"
#include <algorithm>
#include <charconv>
#include <string>
#include <string_view>

namespace
{
struct NumberedName
{
    std::string_view base;
    int number;
};

// "Piano 3" splits into "Piano" and 3; a name without a trailing number counts as number 1.
NumberedName splitNumber(std::string_view name)
{
    const auto space = name.rfind(' ');
    if (space == std::string_view::npos || space == 0 || space + 1 == name.size())
        return {name, 1};

    const auto digits = name.substr(space + 1);
    int number = 0;
    const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), number);
    if (error != std::errc{} || end != digits.data() + digits.size() || number < 1)
        return {name, 1};
    return {name.substr(0, space), number};
}

std::string nextNumberedName(const MidiSequence& sequence, const std::string& name)
{
    const auto base = splitNumber(name).base;
    int highest = 1;
    for (int i = 0; i < sequence.getNumTracks(); ++i)
    {
        const auto other = splitNumber(sequence.getTrack(i).getName());
        if (other.base == base)
            highest = std::max(highest, other.number);
    }
    return std::string{base} + " " + std::to_string(highest + 1);
}
} // namespace

namespace TrackDuplication
{
MidiTrack makeCopy(const MidiSequence& sequence, int index)
{
    const auto& original = sequence.getTrack(index);
    MidiTrack copy = original;

    if (!original.getName().empty())
        copy.setName(nextNumberedName(sequence, original.getName()));

    if (original.getPluginAssignment() != nullptr)
    {
        copy.setPluginAssignment(nullptr);
        copy.setRouteTarget(sequence.resolveRouteTarget(index));
    }
    return copy;
}
} // namespace TrackDuplication
