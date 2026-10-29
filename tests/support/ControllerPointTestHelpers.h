#pragma once

#include "edit/ControllerPointEditResult.h"
#include "model/controller/ControllerKind.h"
#include "support/MidiEventTestHelpers.h"
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ControllerPointTest
{
using Events = std::vector<MidiEvent>;

inline const ControllerKind cc1 = ControllerKind::controlChange(1);

inline MidiEvent p(int tick, int value)
{
    return {.type = MidiEvent::Type::ControlChange, .tick = tick, .data1 = 1, .data2 = value, .ramp = true};
}

inline MidiEvent other(int tick)
{
    return {.type = MidiEvent::Type::ControlChange, .tick = tick, .data1 = 7, .data2 = 100};
}

inline ControllerPointEditResult result(Events events, std::set<int> selected)
{
    return {std::move(events), std::move(selected)};
}
} // namespace ControllerPointTest

namespace Catch
{
template <> struct StringMaker<ControllerPointEditResult>
{
    static std::string convert(const ControllerPointEditResult& r)
    {
        return StringMaker<std::vector<MidiEvent>>::convert(r.events) + " selected " +
               StringMaker<std::set<int>>::convert(r.selected);
    }
};
} // namespace Catch
