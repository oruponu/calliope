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
// Sets the value of one event: data2 for a control change or a key pressure, data1 for the others, with a pitch bend's
// as its 14-bit value. Hold points are made again around it, and it is selected in the result as moveTo selects; a
// value that would make it a hold point is not set.
ControllerPointEditResult setValue(const std::vector<MidiEvent>& before, int index, int value);
// Sets the controller number of one control change, which moves it to that controller's points. Hold points are made
// again around it, and it is selected in the result as moveTo selects; another kind of event, or a number that would
// make it a hold point, leaves the events as they are with nothing selected.
ControllerPointEditResult setControllerNumber(const std::vector<MidiEvent>& before, int index, int number);
} // namespace ChannelEventEdits
