#pragma once

#include "edit/controller/ControllerPointEditResult.h"
#include "model/event/MidiEvent.h"
#include <vector>

namespace ChannelEventEdits
{
// Moves one event of a track's event list in tick order, as the event list moves it: the event alone, after the events
// already on the tick, with no points passed over deleted. Hold points are made again around the move. The moved event
// is selected in the result; a hold point or an index out of range leaves the events as they are with nothing
// selected.
ControllerPointEditResult moveTo(const std::vector<MidiEvent>& before, int index, int tick);
} // namespace ChannelEventEdits
