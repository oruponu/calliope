#pragma once

#include "edit/ControllerPointEditResult.h"
#include "edit/CopiedControllerPoints.h"
#include "model/ControllerKind.h"
#include "model/MidiEvent.h"
#include <optional>
#include <set>
#include <vector>

// Each edit takes a track's whole event list and returns it in tick order. Events not of the kind are kept as they are.
namespace ControllerPointEdits
{
ControllerPointEditResult add(const std::vector<MidiEvent>& before, const ControllerKind& kind, int tick, int value);
// moved and anchorIndex index into before.
ControllerPointEditResult move(const std::vector<MidiEvent>& before, const ControllerKind& kind,
                               const std::set<int>& moved, int anchorIndex, int targetTick, int targetValue);
// The point the anchor becomes in move(), or none when anchorIndex is not one of the moved points of the kind.
std::optional<MidiEvent> movedAnchor(const std::vector<MidiEvent>& before, const ControllerKind& kind,
                                     const std::set<int>& moved, int anchorIndex, int targetTick, int targetValue);
ControllerPointEditResult remove(const std::vector<MidiEvent>& before, const std::set<int>& removed);
ControllerPointEditResult setValue(const std::vector<MidiEvent>& before, const ControllerKind& kind, int index,
                                   int value);
std::set<int> allPoints(const std::vector<MidiEvent>& events, const ControllerKind& kind);
// selected indexes into events; indices that are not points of the kind are skipped.
CopiedControllerPoints copy(const std::vector<MidiEvent>& events, const ControllerKind& kind,
                            const std::set<int>& selected);
// Points of the kind from the first pasted tick to the last are replaced by the pasted ones.
ControllerPointEditResult paste(const std::vector<MidiEvent>& before, const ControllerKind& kind,
                                const std::vector<CopiedControllerPoint>& points, int atTick);
} // namespace ControllerPointEdits
