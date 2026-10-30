#include "ui/eventlist/EditableRows.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

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

TEST_CASE("only a note's event can be edited", "[ui][event-list]")
{
    auto recorded = itemOf(EventListItem::Note);
    recorded.isRecording = true;
    CHECK(EditableRows::isEventEditable(itemOf(EventListItem::Note)));
    CHECK_FALSE(EditableRows::isEventEditable(itemOf(EventListItem::ControlChange)));
    CHECK_FALSE(EditableRows::isEventEditable(recorded));
}

TEST_CASE("the next editable row skips rows that cannot be edited", "[ui][event-list]")
{
    const std::vector items{itemOf(EventListItem::Note), itemOf(EventListItem::Meta), itemOf(EventListItem::PitchBend)};
    CHECK(EditableRows::next(items, 0, false) == 2);
    CHECK(EditableRows::next(items, 2, true) == 0);
}

TEST_CASE("there is no editable row past the last one", "[ui][event-list]")
{
    const std::vector items{itemOf(EventListItem::SysEx), itemOf(EventListItem::Note), itemOf(EventListItem::SysEx)};
    CHECK_FALSE(EditableRows::next(items, 1, false).has_value());
    CHECK_FALSE(EditableRows::next(items, 1, true).has_value());
}
