#pragma once

#include "edit/ControllerPointEditResult.h"
#include <optional>
#include <set>
#include <vector>

namespace ProgramChangeEdits
{
// The tick of the block that covers tick: the latest program change tick at or before it; none before the first.
std::optional<int> blockTickAt(const std::vector<MidiEvent>& events, int tick);

// The indices of the program changes on tick.
std::set<int> onTick(const std::vector<MidiEvent>& events, int tick);

// The program in effect at tick, counting a program change on tick; 0 when there is none.
int programAt(const std::vector<MidiEvent>& events, int tick);

// Sets the program on tick. The last program change on tick is kept with the program and the others there are
// deleted; with none there, one is added after the events on tick. The placed one is selected.
ControllerPointEditResult place(const std::vector<MidiEvent>& before, int tick, int program);
} // namespace ProgramChangeEdits
