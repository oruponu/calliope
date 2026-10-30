#include "ui/eventlist/EditableRows.h"

bool EditableRows::isEditable(const EventListItem& item)
{
    return !item.isRecording && !item.isDraft && item.kind != EventListItem::Meta && item.kind != EventListItem::SysEx;
}
