#pragma once

#include "model/MidiEvent.h"
#include <optional>
#include <set>
#include <vector>

namespace ProgramChangeEdits
{
// The tick of the block that covers tick: the latest program change tick at or before it; none before the first.
std::optional<int> blockTickAt(const std::vector<MidiEvent>& events, int tick);

// The indices of the program changes on tick.
std::set<int> onTick(const std::vector<MidiEvent>& events, int tick);

} // namespace ProgramChangeEdits
