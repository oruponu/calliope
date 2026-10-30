#include "ui/eventlist/EditableRows.h"

bool EditableRows::isEditable(const EventListItem& item)
{
    return !item.isRecording && !item.isDraft && item.kind != EventListItem::Meta && item.kind != EventListItem::SysEx;
}

std::optional<int> EditableRows::next(const std::vector<EventListItem>& items, int row, bool backwards)
{
    const int step = backwards ? -1 : 1;
    for (int at = row + step; at >= 0 && at < static_cast<int>(items.size()); at += step)
        if (isEditable(items[static_cast<size_t>(at)]))
            return at;
    return std::nullopt;
}
