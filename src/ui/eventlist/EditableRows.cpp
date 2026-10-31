#include "ui/eventlist/EditableRows.h"
#include <algorithm>

namespace
{
std::optional<int> nextRow(const std::vector<EventListItem>& items, int row, bool backwards)
{
    const int step = backwards ? -1 : 1;
    for (int at = row + step; at >= 0 && at < static_cast<int>(items.size()); at += step)
        if (EditableRows::isEditable(items[static_cast<size_t>(at)]))
            return at;
    return std::nullopt;
}

// The row's editable columns in the order Tab goes through them.
std::vector<EditedColumn> columnsOf(const EventListItem& item)
{
    std::vector<EditedColumn> columns{EditedColumn::Position};
    if (EditableRows::isEventEditable(item) && item.kind == EventListItem::Note)
        columns.push_back(EditedColumn::Event);
    if (EditableRows::isLengthEditable(item))
        columns.push_back(EditedColumn::Length);
    if (EditableRows::isValueEditable(item))
        columns.push_back(EditedColumn::Value);
    return columns;
}
} // namespace

bool EditableRows::isEditable(const EventListItem& item)
{
    return !item.isRecording && !item.isDraft && item.kind != EventListItem::Meta && item.kind != EventListItem::SysEx;
}

bool EditableRows::isEventEditable(const EventListItem& item)
{
    return isEditable(item) && (item.kind == EventListItem::Note || item.kind == EventListItem::ControlChange);
}

bool EditableRows::isLengthEditable(const EventListItem& item)
{
    return isEditable(item) && item.kind == EventListItem::Note;
}

bool EditableRows::isValueEditable(const EventListItem& item)
{
    return isEditable(item);
}

std::optional<EditedCell> EditableRows::next(const std::vector<EventListItem>& items, EditedCell cell, bool backwards)
{
    if (cell.row >= 0 && cell.row < static_cast<int>(items.size()))
    {
        const auto columns = columnsOf(items[static_cast<size_t>(cell.row)]);
        const auto at = std::ranges::find(columns, cell.column);
        if (at != columns.end())
        {
            if (!backwards && at + 1 != columns.end())
                return EditedCell{cell.row, *(at + 1)};
            if (backwards && at != columns.begin())
                return EditedCell{cell.row, *(at - 1)};
        }
    }

    const auto row = nextRow(items, cell.row, backwards);
    if (!row)
        return std::nullopt;
    return EditedCell{*row, backwards ? columnsOf(items[static_cast<size_t>(*row)]).back() : EditedColumn::Position};
}
