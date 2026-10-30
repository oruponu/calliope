#include "ui/eventlist/EditableRows.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
EventListItem itemOf(EventListItem::Kind kind)
{
    EventListItem item;
    item.kind = kind;
    return item;
}
} // namespace

TEST_CASE("notes and channel events can be edited", "[ui][event-list]")
{
    for (const auto kind : {EventListItem::Note, EventListItem::ControlChange, EventListItem::ProgramChange,
                            EventListItem::PitchBend, EventListItem::ChannelPressure, EventListItem::KeyPressure})
        CHECK(EditableRows::isEditable(itemOf(kind)));
}

TEST_CASE("meta events, system exclusive messages, recorded rows and the draft note cannot be edited",
          "[ui][event-list]")
{
    auto recorded = itemOf(EventListItem::Note);
    recorded.isRecording = true;
    auto draft = itemOf(EventListItem::Note);
    draft.isDraft = true;
    for (const auto& item : {itemOf(EventListItem::Meta), itemOf(EventListItem::SysEx), recorded, draft})
        CHECK_FALSE(EditableRows::isEditable(item));
}
