#pragma once

#include "ui/eventlist/EditedCell.h"
#include "ui/eventlist/EventListItems.h"
#include <optional>
#include <vector>

namespace EditableRows
{
// Notes and channel events in the sequence.
bool isEditable(const EventListItem& item);
// Whether the row's event column can be edited: only a note's, for its note number.
bool isEventEditable(const EventListItem& item);
// Whether the row's length can be edited: only a note's.
bool isLengthEditable(const EventListItem& item);
// The editable cell after the cell (before it, when backwards), going through a row's position and then its event;
// none past the last one.
std::optional<EditedCell> next(const std::vector<EventListItem>& items, EditedCell cell, bool backwards);
} // namespace EditableRows
