#pragma once

#include "ui/eventlist/EventListItems.h"
#include <optional>
#include <vector>

namespace EditableRows
{
// Notes and channel events in the sequence.
bool isEditable(const EventListItem& item);
// The first editable row after the row (before it, when backwards); none past the last one.
std::optional<int> next(const std::vector<EventListItem>& items, int row, bool backwards);
} // namespace EditableRows
