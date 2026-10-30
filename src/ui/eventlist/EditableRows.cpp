#include "ui/eventlist/EditableRows.h"

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
} // namespace

bool EditableRows::isEditable(const EventListItem& item)
{
    return !item.isRecording && !item.isDraft && item.kind != EventListItem::Meta && item.kind != EventListItem::SysEx;
}

bool EditableRows::isEventEditable(const EventListItem& item)
{
    return isEditable(item) && item.kind == EventListItem::Note;
}

std::optional<EditedCell> EditableRows::next(const std::vector<EventListItem>& items, EditedCell cell, bool backwards)
{
    const auto hasEvent = [&items](int row) { return isEventEditable(items[static_cast<size_t>(row)]); };
    if (!backwards && cell.column == EditedColumn::Position && cell.row >= 0 &&
        cell.row < static_cast<int>(items.size()) && hasEvent(cell.row))
        return EditedCell{cell.row, EditedColumn::Event};
    if (backwards && cell.column == EditedColumn::Event)
        return EditedCell{cell.row, EditedColumn::Position};

    const auto row = nextRow(items, cell.row, backwards);
    if (!row)
        return std::nullopt;
    return EditedCell{*row, backwards && hasEvent(*row) ? EditedColumn::Event : EditedColumn::Position};
}
