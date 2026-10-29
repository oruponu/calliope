#pragma once

#include "edit/ControllerPointEditResult.h"
#include "edit/CopiedControllerPoints.h"
#include "model/controller/ProgramVoice.h"
#include <map>
#include <optional>
#include <set>
#include <vector>

namespace ProgramChangeEdits
{
// The tick of the block that covers tick: the latest program change tick at or before it; none before the first.
std::optional<int> blockTickAt(const std::vector<MidiEvent>& events, int tick);

// The indices of the program changes on tick.
std::set<int> onTick(const std::vector<MidiEvent>& events, int tick);

// The CC0 and CC32 on the tick of the program change at pcIndex that come before it, when it is the last program
// change on its tick; empty otherwise.
std::set<int> bankSelectsOf(const std::vector<MidiEvent>& events, int pcIndex);

std::set<int> withBankSelects(const std::vector<MidiEvent>& events, const std::set<int>& pcIndices);

// The voice of every program change, keyed by its index, found in one pass in tick order.
std::map<int, ProgramVoice> voicesOf(const std::vector<MidiEvent>& events);

// The program and the latest CC0 and CC32 before it in tick order (0 for none), as the instrument has them.
ProgramVoice voiceOf(const std::vector<MidiEvent>& events, int pcIndex);

// The voice of the last program change at or before tick; {} when there is none.
ProgramVoice voiceAt(const std::vector<MidiEvent>& events, int tick);

// The latest CC0 and CC32 on ticks before tick; those on tick itself are left out, since a group placed there
// replaces its own and goes before the others.
ProgramBank bankBefore(const std::vector<MidiEvent>& events, int tick);

// Sets the voice on tick. The last program change on tick is kept and the others there are deleted. The placed
// program change is selected.
ControllerPointEditResult place(const std::vector<MidiEvent>& before, int tick, const ProgramVoice& voice);

// These indices refer to program changes. Bank selects in their groups move or are removed with them, while only
// program changes are selected in the result.
ControllerPointEditResult move(const std::vector<MidiEvent>& before, const std::set<int>& moved, int anchorIndex,
                               int targetTick);
ControllerPointEditResult remove(const std::vector<MidiEvent>& before, const std::set<int>& removed);
CopiedControllerPoints copy(const std::vector<MidiEvent>& events, const std::set<int>& selected);
ControllerPointEditResult paste(const std::vector<MidiEvent>& before, const std::vector<CopiedControllerPoint>& points,
                                int atTick);
} // namespace ProgramChangeEdits
