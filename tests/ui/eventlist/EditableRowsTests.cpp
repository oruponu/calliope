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

EditedCell positionOf(int row)
{
    return {row, EditedColumn::Position};
}

EditedCell eventOf(int row)
{
    return {row, EditedColumn::Event};
}

EditedCell lengthOf(int row)
{
    return {row, EditedColumn::Length};
}

EditedCell valueOf(int row)
{
    return {row, EditedColumn::Value};
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

TEST_CASE("only a note's length can be edited", "[ui][event-list]")
{
    auto draft = itemOf(EventListItem::Note);
    draft.isDraft = true;
    CHECK(EditableRows::isLengthEditable(itemOf(EventListItem::Note)));
    CHECK_FALSE(EditableRows::isLengthEditable(itemOf(EventListItem::PitchBend)));
    CHECK_FALSE(EditableRows::isLengthEditable(draft));
}

TEST_CASE("a note's and a channel event's value can be edited", "[ui][event-list]")
{
    auto recorded = itemOf(EventListItem::ControlChange);
    recorded.isRecording = true;
    CHECK(EditableRows::isValueEditable(itemOf(EventListItem::Note)));
    CHECK(EditableRows::isValueEditable(itemOf(EventListItem::PitchBend)));
    CHECK_FALSE(EditableRows::isValueEditable(itemOf(EventListItem::Meta)));
    CHECK_FALSE(EditableRows::isValueEditable(recorded));
}

TEST_CASE("a note goes through its position, event, length and value before the next row", "[ui][event-list]")
{
    const std::vector items{itemOf(EventListItem::Note), itemOf(EventListItem::Note)};
    CHECK(EditableRows::next(items, positionOf(0), false) == eventOf(0));
    CHECK(EditableRows::next(items, eventOf(0), false) == lengthOf(0));
    CHECK(EditableRows::next(items, lengthOf(0), false) == valueOf(0));
    CHECK(EditableRows::next(items, valueOf(0), false) == positionOf(1));
    CHECK(EditableRows::next(items, positionOf(1), true) == valueOf(0));
    CHECK(EditableRows::next(items, valueOf(0), true) == lengthOf(0));
    CHECK(EditableRows::next(items, lengthOf(0), true) == eventOf(0));
    CHECK(EditableRows::next(items, eventOf(0), true) == positionOf(0));
}

TEST_CASE("a channel event goes through its position and value before the next row", "[ui][event-list]")
{
    const std::vector items{itemOf(EventListItem::ControlChange), itemOf(EventListItem::PitchBend)};
    CHECK(EditableRows::next(items, positionOf(0), false) == valueOf(0));
    CHECK(EditableRows::next(items, valueOf(0), false) == positionOf(1));
    CHECK(EditableRows::next(items, positionOf(1), true) == valueOf(0));
    CHECK(EditableRows::next(items, valueOf(0), true) == positionOf(0));
}

TEST_CASE("the next editable cell skips rows that cannot be edited", "[ui][event-list]")
{
    const std::vector items{itemOf(EventListItem::Note), itemOf(EventListItem::Meta), itemOf(EventListItem::PitchBend)};
    CHECK(EditableRows::next(items, valueOf(0), false) == positionOf(2));
    CHECK(EditableRows::next(items, positionOf(2), true) == valueOf(0));
}

TEST_CASE("there is no editable cell past the last one", "[ui][event-list]")
{
    const std::vector items{itemOf(EventListItem::SysEx), itemOf(EventListItem::Note), itemOf(EventListItem::SysEx)};
    CHECK_FALSE(EditableRows::next(items, valueOf(1), false).has_value());
    CHECK_FALSE(EditableRows::next(items, positionOf(1), true).has_value());
}
