#include "support/NoteRefStringMaker.h"
#include "ui/EventListItems.h"
#include <catch2/catch_test_macros.hpp>
#include <ranges>
#include <vector>

namespace
{
class FakeLayer : public RecordingLayer
{
public:
    Take take;
    std::vector<HeldTakeNote> held;

    const Take& getTake() const override { return take; }
    std::vector<HeldTakeNote> getHeldNotes() const override { return held; }
};

MidiTrack trackWith(std::vector<MidiNote> notes, std::vector<MidiEvent> events = {})
{
    MidiTrack track;
    for (const auto& note : notes)
        track.addNote(note);
    for (const auto& event : events)
        track.addEvent(event);
    return track;
}

MidiEvent cc(int tick, int number, int value)
{
    return {MidiEvent::Type::ControlChange, tick, number, value};
}

std::vector<int> ticksOf(const std::vector<EventListItem>& items)
{
    return items | std::views::transform(&EventListItem::tick) | std::ranges::to<std::vector>();
}

std::vector<int> tracksOf(const std::vector<EventListItem>& items)
{
    return items | std::views::transform(&EventListItem::trackIndex) | std::ranges::to<std::vector>();
}

std::vector<int> sourcesOf(const std::vector<EventListItem>& items)
{
    return items | std::views::transform(&EventListItem::sourceIndex) | std::ranges::to<std::vector>();
}

std::vector<EventListItem> itemsAt(std::vector<int> ticks)
{
    return ticks | std::views::transform([](int tick) { return EventListItem{.tick = tick}; }) |
           std::ranges::to<std::vector>();
}

EventListItem noteRow(int tick, int sourceIndex, bool isRecording = false)
{
    return {.kind = EventListItem::Note, .tick = tick, .sourceIndex = sourceIndex, .isRecording = isRecording};
}

EventListItem ccRow(int tick, int sourceIndex, int value)
{
    return {.kind = EventListItem::ControlChange, .tick = tick, .sourceIndex = sourceIndex, .data1 = 7, .data2 = value};
}
} // namespace

TEST_CASE("notes and events are listed by tick", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 960, 480}, {62, 100, 0, 480}}, {cc(480, 7, 100)}));
    const auto items = EventListItems::build(seq, {0}, nullptr);
    CHECK(ticksOf(items) == std::vector{0, 480, 960});
    CHECK(sourcesOf(items) == std::vector{1, 0, 0});
    CHECK(items[0].kind == EventListItem::Note);
    CHECK(items[1].kind == EventListItem::ControlChange);
    CHECK(items[2].noteNumber == 60);
}

TEST_CASE("rows at the same tick go by track, then notes before events", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({}, {cc(0, 7, 100)}));
    seq.addTrack(trackWith({{60, 100, 0, 480}}, {cc(0, 10, 64)}));
    const auto items = EventListItems::build(seq, {0, 1}, nullptr);
    CHECK(tracksOf(items) == std::vector{0, 1, 1});
    CHECK(items[1].kind == EventListItem::Note);
    CHECK(items[2].kind == EventListItem::ControlChange);
}

TEST_CASE("only the shown tracks in range are listed", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 0, 480}}));
    seq.addTrack(trackWith({{62, 100, 0, 480}, {64, 100, 480, 480}}));
    CHECK(tracksOf(EventListItems::build(seq, {1}, nullptr)) == std::vector{1, 1});
    CHECK(tracksOf(EventListItems::build(seq, {0, 5}, nullptr)) == std::vector{0});
}

TEST_CASE("recorded notes and events carry the recording mark", "[ui][event-list]")
{
    MidiSequence seq;
    const TrackId id = seq.addTrack(trackWith({{60, 100, 0, 480}})).getId();
    FakeLayer layer;
    layer.take[id] = {{{62, 80, 240, 120}}, {cc(120, 64, 127)}, {7}};
    const auto items = EventListItems::build(seq, {0}, &layer);
    REQUIRE(items.size() == 3);
    CHECK(ticksOf(items) == std::vector{0, 120, 240});
    CHECK_FALSE(items[0].isRecording);
    CHECK(items[1].isRecording);
    CHECK(items[1].sourceIndex == -1);
    CHECK(items[2].isRecording);
    CHECK(EventListItems::noteRefOf(items[2]) == NoteRef{0, 7, NoteLayer::Recording});
}

TEST_CASE("held notes are not listed", "[ui][event-list]")
{
    MidiSequence seq;
    const TrackId id = seq.addTrack().getId();
    FakeLayer layer;
    layer.held = {{id, 64, 70, 480, 9}};
    CHECK(EventListItems::build(seq, {0}, &layer).empty());
}

TEST_CASE("a sequence note row refers to its index on the track", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 480, 480}, {62, 100, 0, 480}}));
    const auto items = EventListItems::build(seq, {0}, nullptr);
    CHECK(EventListItems::noteRefOf(items[0]) == NoteRef{0, 1});
    CHECK(EventListItems::noteRefOf(items[1]) == NoteRef{0, 0});
}

TEST_CASE("the playhead row is the first row at its tick", "[ui][event-list]")
{
    const auto items = itemsAt({480, 960, 960, 1440});
    CHECK(EventListItems::playheadRow(items, 960) == 1);
    CHECK(EventListItems::playheadRow(items, 480) == 0);
}

TEST_CASE("between rows the playhead row is the last row before it", "[ui][event-list]")
{
    const auto items = itemsAt({480, 960, 960, 1440});
    CHECK(EventListItems::playheadRow(items, 1200) == 2);
    CHECK(EventListItems::playheadRow(items, 2000) == 3);
}

TEST_CASE("before the first row the playhead row is the first row", "[ui][event-list]")
{
    CHECK(EventListItems::playheadRow(itemsAt({480, 960}), 0) == 0);
}

TEST_CASE("an empty list has no playhead row", "[ui][event-list]")
{
    CHECK(EventListItems::playheadRow({}, 0) == -1);
}

TEST_CASE("an event with the same values is the same row whatever its index", "[ui][event-list]")
{
    const EventListItem before{
        .kind = EventListItem::ControlChange, .tick = 480, .trackIndex = 1, .sourceIndex = 0, .data1 = 7, .data2 = 100};
    auto after = before;
    after.sourceIndex = 3;
    CHECK(EventListItems::isSameEvent(before, after));
}

TEST_CASE("an event with other values is another row", "[ui][event-list]")
{
    const EventListItem event{
        .kind = EventListItem::ControlChange, .tick = 480, .trackIndex = 1, .data1 = 7, .data2 = 100};
    auto otherValue = event;
    otherValue.data2 = 90;
    auto otherTick = event;
    otherTick.tick = 960;
    auto otherTrack = event;
    otherTrack.trackIndex = 0;
    auto otherKind = event;
    otherKind.kind = EventListItem::ProgramChange;
    auto recorded = event;
    recorded.isRecording = true;
    CHECK_FALSE(EventListItems::isSameEvent(event, otherValue));
    CHECK_FALSE(EventListItems::isSameEvent(event, otherTick));
    CHECK_FALSE(EventListItems::isSameEvent(event, otherTrack));
    CHECK_FALSE(EventListItems::isSameEvent(event, otherKind));
    CHECK_FALSE(EventListItems::isSameEvent(event, recorded));
}

TEST_CASE("note rows are never the same event", "[ui][event-list]")
{
    const EventListItem note{.kind = EventListItem::Note, .tick = 0, .trackIndex = 0, .sourceIndex = 0};
    CHECK_FALSE(EventListItems::isSameEvent(note, note));
}

TEST_CASE("a selected note is reselected at its new row", "[ui][event-list]")
{
    const std::vector before{noteRow(0, 0), noteRow(480, 1)};
    const std::vector after{noteRow(480, 1), noteRow(960, 0)};
    CHECK(EventListItems::reselectedRows(before, {0}, after) == std::vector{1});
}

TEST_CASE("a recorded note is told apart from the sequence note with the same number", "[ui][event-list]")
{
    const std::vector before{noteRow(0, 3)};
    const std::vector after{noteRow(0, 3, true), noteRow(0, 3)};
    CHECK(EventListItems::reselectedRows(before, {0}, after) == std::vector{1});
}

TEST_CASE("a selected event is reselected by its values whatever its index", "[ui][event-list]")
{
    const std::vector before{ccRow(480, 0, 100)};
    const std::vector after{noteRow(0, 0), ccRow(480, 5, 100)};
    CHECK(EventListItems::reselectedRows(before, {0}, after) == std::vector{1});
}

TEST_CASE("of two equal events the second stays selected", "[ui][event-list]")
{
    const std::vector before{ccRow(480, 0, 100), ccRow(480, 1, 100)};
    const std::vector after{noteRow(0, 0), ccRow(480, 0, 100), ccRow(480, 1, 100)};
    CHECK(EventListItems::reselectedRows(before, {1}, after) == std::vector{2});
    CHECK(EventListItems::reselectedRows(before, {0, 1}, after) == std::vector{1, 2});
}

TEST_CASE("a removed note or a changed event is no longer selected", "[ui][event-list]")
{
    const std::vector before{noteRow(0, 0), ccRow(480, 0, 100)};
    const std::vector after{ccRow(480, 0, 90)};
    CHECK(EventListItems::reselectedRows(before, {0, 1}, after).empty());
}

TEST_CASE("selected rows past the old rows are ignored", "[ui][event-list]")
{
    const std::vector before{noteRow(0, 0)};
    CHECK(EventListItems::reselectedRows(before, {5}, before).empty());
}
