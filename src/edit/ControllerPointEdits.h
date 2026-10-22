#pragma once

#include "edit/ControllerPointEditResult.h"
#include "model/ControllerKind.h"
#include "model/MidiEvent.h"
#include <set>
#include <vector>

// Each edit takes a track's whole event list and returns it in tick order. Events not of the kind are kept as they are.
namespace ControllerPointEdits
{
ControllerPointEditResult add(const std::vector<MidiEvent>& before, const ControllerKind& kind, int tick, int value);
// moved and anchorIndex index into before.
ControllerPointEditResult move(const std::vector<MidiEvent>& before, const ControllerKind& kind,
                               const std::set<int>& moved, int anchorIndex, int targetTick, int targetValue);
ControllerPointEditResult remove(const std::vector<MidiEvent>& before, const std::set<int>& removed);
} // namespace ControllerPointEdits
