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

EventListItem eventRow(EventListItem::Kind kind, int trackIndex, int sourceIndex, int data1 = 7,
                       bool isRecording = false)
{
    return {
        .kind = kind, .trackIndex = trackIndex, .sourceIndex = sourceIndex, .data1 = data1, .isRecording = isRecording};
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

TEST_CASE("sysex rows come first at their tick on their track", "[ui][event-list][sysex]")
{
    MidiSequence seq;
    auto firstTrack = trackWith({{60, 100, 0, 480}}, {cc(0, 7, 100)});
    firstTrack.addSysEx({0, {0x41}});
    seq.addTrack(firstTrack);
    auto secondTrack = trackWith({});
    secondTrack.addSysEx({0, {0x42}});
    seq.addTrack(secondTrack);

    const auto items = EventListItems::build(seq, {0, 1}, nullptr);

    REQUIRE(items.size() == 4);
    CHECK(items[0].kind == EventListItem::SysEx);
    CHECK(items[0].trackIndex == 0);
    CHECK(items[0].sourceIndex == 0);
    CHECK(items[0].sysExData == std::vector<std::uint8_t>{0x41});
    CHECK(items[1].kind == EventListItem::Note);
    CHECK(items[2].kind == EventListItem::ControlChange);
    CHECK(items[3].kind == EventListItem::SysEx);
    CHECK(items[3].trackIndex == 1);
}

TEST_CASE("sysex rows are not event rows", "[ui][event-list][sysex]")
{
    MidiSequence seq;
    auto track = trackWith({}, {cc(0, 7, 100)});
    track.addSysEx({0, {0x41}});
    seq.addTrack(track);

    const auto items = EventListItems::build(seq, {0}, nullptr);

    CHECK(EventListItems::eventRows(items, 0, {0}) == std::vector{1});
}

TEST_CASE("sysex rows have no controller", "[ui][event-list][sysex]")
{
    EventListItem item;
    item.kind = EventListItem::SysEx;
    item.sysExData = {0x41};
    CHECK_FALSE(EventListItems::controllerOf(item).has_value());
}

TEST_CASE("selected sysex rows stay selected after a rebuild", "[ui][event-list][sysex]")
{
    MidiSequence seq;
    auto track = trackWith({});
    track.addSysEx({0, {0x41}});
    track.addSysEx({0, {0x42}});
    seq.addTrack(track);

    const auto before = EventListItems::build(seq, {0}, nullptr);
    MidiSequence rebuiltSeq;
    auto rebuiltTrack = trackWith({{60, 100, 0, 480}});
    rebuiltTrack.addSysEx({0, {0x42}});
    rebuiltTrack.addSysEx({0, {0x41}});
    rebuiltSeq.addTrack(rebuiltTrack);
    const auto after = EventListItems::build(rebuiltSeq, {0}, nullptr);

    CHECK(EventListItems::reselectedRows(before, {1}, after) == std::vector{0});
    CHECK(after[0].sysExData == std::vector<std::uint8_t>{0x42});
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

TEST_CASE("a moved note is listed where the drag shows it", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 0, 480}, {62, 100, 480, 480}, {64, 100, 960, 480}}));
    NoteDragPreview preview{.dragging = true};
    preview.changedNotes[{0, 0}] = {67, 100, 1200, 480};
    const auto items = EventListItems::build(seq, {0}, nullptr, preview);
    CHECK(ticksOf(items) == std::vector{480, 960, 1200});
    CHECK(sourcesOf(items) == std::vector{1, 2, 0});
    CHECK(items[2].noteNumber == 67);
}

TEST_CASE("a resized note shows the drag's start and length", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 0, 480}}));
    NoteDragPreview preview{.dragging = true};
    preview.changedNotes[{0, 0}] = {60, 100, 240, 240};
    const auto items = EventListItems::build(seq, {0}, nullptr, preview);
    REQUIRE(items.size() == 1);
    CHECK(items[0].tick == 240);
    CHECK(items[0].duration == 240);
}

TEST_CASE("a velocity drag changes only the value", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 0, 480}}));
    NoteDragPreview preview{.dragging = true};
    preview.changedNotes[{0, 0}] = {60, 30, 0, 480};
    const auto items = EventListItems::build(seq, {0}, nullptr, preview);
    REQUIRE(items.size() == 1);
    CHECK(items[0].velocity == 30);
    CHECK(items[0].tick == 0);
    CHECK(items[0].duration == 480);
}

TEST_CASE("a recorded note takes the drag's values", "[ui][event-list]")
{
    MidiSequence seq;
    const TrackId id = seq.addTrack(trackWith({{60, 100, 480, 480}})).getId();
    FakeLayer layer;
    layer.take[id] = {{{62, 80, 240, 120}}, {}, {7}};
    NoteDragPreview preview{.dragging = true};
    preview.changedNotes[{0, 7, NoteLayer::Recording}] = {62, 80, 720, 120};
    const auto items = EventListItems::build(seq, {0}, &layer, preview);
    CHECK(ticksOf(items) == std::vector{480, 720});
    CHECK(items[1].isRecording);
}

TEST_CASE("a drawn note is added as a draft row in tick order", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 0, 480}, {62, 100, 960, 480}}));
    NoteDragPreview preview{.dragging = true};
    preview.draftNote = DraftNote{0, {64, 100, 480, 240}};
    const auto items = EventListItems::build(seq, {0}, nullptr, preview);
    CHECK(ticksOf(items) == std::vector{0, 480, 960});
    CHECK(items[1].isDraft);
    CHECK(items[1].duration == 240);
    CHECK_FALSE(items[0].isDraft);
    CHECK_FALSE(items[2].isDraft);
}

TEST_CASE("a drawn note on a hidden track is not listed", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 0, 480}}));
    seq.addTrack();
    NoteDragPreview preview{.dragging = true};
    preview.draftNote = DraftNote{1, {64, 100, 480, 240}};
    CHECK(EventListItems::build(seq, {0}, nullptr, preview).size() == 1);
}

TEST_CASE("an empty preview lists the sequence as it is", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 960, 480}, {62, 100, 0, 480}}, {cc(480, 7, 100)}));
    const auto items = EventListItems::build(seq, {0}, nullptr, NoteDragPreview{});
    CHECK(ticksOf(items) == std::vector{0, 480, 960});
    CHECK(sourcesOf(items) == std::vector{1, 0, 0});
}

TEST_CASE("a draft row is never reselected", "[ui][event-list]")
{
    const std::vector rows{EventListItem{.kind = EventListItem::Note, .tick = 480, .sourceIndex = -1, .isDraft = true}};
    CHECK(EventListItems::reselectedRows(rows, {0}, rows).empty());
}

TEST_CASE("the focus row is the grabbed note's row after the move", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 0, 480}, {62, 100, 480, 480}}));
    NoteDragPreview preview{.dragging = true, .focus = {0, 0}};
    preview.changedNotes[{0, 0}] = {60, 100, 960, 480};
    const auto items = EventListItems::build(seq, {0}, nullptr, preview);
    CHECK(EventListItems::focusRow(items, preview) == 1);
}

TEST_CASE("the focus row is the draft row while drawing", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 0, 480}, {62, 100, 960, 480}}));
    NoteDragPreview preview{.dragging = true};
    preview.draftNote = DraftNote{0, {64, 100, 480, 240}};
    const auto items = EventListItems::build(seq, {0}, nullptr, preview);
    CHECK(EventListItems::focusRow(items, preview) == 1);
}

TEST_CASE("there is no focus row without a focus note or one that is listed", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 0, 480}}));
    const auto items = EventListItems::build(seq, {0}, nullptr);
    CHECK_FALSE(EventListItems::focusRow(items, NoteDragPreview{.dragging = true}).has_value());
    CHECK_FALSE(EventListItems::focusRow(items, NoteDragPreview{.dragging = true, .focus = {0, 5}}).has_value());
}

TEST_CASE("dragged points are listed where the drag shows them", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 480, 480}}, {cc(0, 7, 10), cc(960, 7, 20)}));
    NoteDragPreview preview{.dragging = true};
    preview.draggedEvents = DraggedEvents{0, {cc(960, 7, 20), cc(1440, 7, 50)}, {1}};
    const auto items = EventListItems::build(seq, {0}, nullptr, preview);
    CHECK(ticksOf(items) == std::vector{480, 960, 1440});
    CHECK(sourcesOf(items) == std::vector{0, 0, 1});
    CHECK(items[2].data2 == 50);
}

TEST_CASE("a point drag leaves the other tracks' events as they are", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({}, {cc(0, 7, 10)}));
    seq.addTrack(trackWith({}, {cc(480, 7, 20)}));
    NoteDragPreview preview{.dragging = true};
    preview.draggedEvents = DraggedEvents{1, {cc(960, 7, 30)}, {0}};
    const auto items = EventListItems::build(seq, {0, 1}, nullptr, preview);
    CHECK(ticksOf(items) == std::vector{0, 960});
    CHECK(tracksOf(items) == std::vector{0, 1});
}

TEST_CASE("hold points the drag shows that are not sent are not listed", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack();
    NoteDragPreview preview{.dragging = true};
    preview.draggedEvents = DraggedEvents{0,
                                          {{MidiEvent::Type::ControlChange, 0, 11, 50, true},
                                           {MidiEvent::Type::ControlChange, 1920, 11, 50, true},
                                           {MidiEvent::Type::ControlChange, 1920, 11, 100, true}},
                                          {2}};
    CHECK(sourcesOf(EventListItems::build(seq, {0}, nullptr, preview)) == std::vector{0, 2});
}

TEST_CASE("the dragged rows are the rows of the dragged points", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 480, 480}}, {cc(0, 7, 10), cc(960, 7, 20)}));
    NoteDragPreview preview{.dragging = true};
    preview.draggedEvents = DraggedEvents{0, {cc(0, 7, 10), cc(240, 7, 15), cc(1440, 7, 20)}, {1, 2}};
    const auto items = EventListItems::build(seq, {0}, nullptr, preview);
    CHECK(EventListItems::draggedRows(items, preview) == std::vector{1, 3});
}

TEST_CASE("there are no dragged rows without a point drag", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({}, {cc(0, 7, 10)}));
    const auto items = EventListItems::build(seq, {0}, nullptr);
    CHECK(EventListItems::draggedRows(items, NoteDragPreview{.dragging = true}).empty());
}

TEST_CASE("the focus row is the grabbed point's row", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 480, 480}}, {cc(0, 7, 10), cc(960, 7, 20)}));
    NoteDragPreview preview{.dragging = true};
    preview.draggedEvents = DraggedEvents{0, {cc(0, 7, 10), cc(720, 7, 15), cc(1440, 7, 20)}, {1, 2}, 2};
    const auto items = EventListItems::build(seq, {0}, nullptr, preview);
    CHECK(EventListItems::focusRow(items, preview) == 3);
}

TEST_CASE("without a grabbed point the focus row is the first dragged point's row", "[ui][event-list]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 480, 480}}, {cc(0, 7, 10), cc(960, 7, 20)}));
    NoteDragPreview preview{.dragging = true};
    preview.draggedEvents = DraggedEvents{0, {cc(0, 7, 10), cc(720, 7, 15), cc(1440, 7, 20)}, {1, 2}};
    const auto items = EventListItems::build(seq, {0}, nullptr, preview);
    CHECK(EventListItems::focusRow(items, preview) == 2);
}

TEST_CASE("hold points that are not sent are not listed", "[ui][event-list]")
{
    MidiSequence seq;
    MidiTrack track;
    track.addEvent({MidiEvent::Type::ControlChange, 0, 11, 50, true});
    track.addEvent({MidiEvent::Type::ControlChange, 1920, 11, 50, true});
    track.addEvent({MidiEvent::Type::ControlChange, 1920, 11, 100, true});
    seq.addTrack(track);

    const auto items = EventListItems::build(seq, {0}, nullptr);
    CHECK(ticksOf(items) == std::vector{0, 1920});
    CHECK(sourcesOf(items) == std::vector{0, 2});
}

TEST_CASE("the rows of a track's events are found by their indices", "[ui][event-list]")
{
    const std::vector items{noteRow(0, 0), ccRow(0, 0, 10), ccRow(480, 1, 20), ccRow(960, 2, 30)};
    CHECK(EventListItems::eventRows(items, 0, {0, 2}) == std::vector{1, 3});
}

TEST_CASE("event rows of another track or of the recording are not found", "[ui][event-list]")
{
    const std::vector items{eventRow(EventListItem::ControlChange, 1, 0),
                            eventRow(EventListItem::ControlChange, 0, 0, 7, true),
                            eventRow(EventListItem::ControlChange, 0, 0)};
    CHECK(EventListItems::eventRows(items, 0, {0}) == std::vector{2});
}

TEST_CASE("a control change, pitch bend or program change row tells its controller", "[ui][event-list]")
{
    CHECK(EventListItems::controllerOf(eventRow(EventListItem::ControlChange, 0, 0, 11)) ==
          ControllerKind::controlChange(11));
    CHECK(EventListItems::controllerOf(eventRow(EventListItem::PitchBend, 0, 0, 8192)) == ControllerKind::pitchBend());
    CHECK(EventListItems::controllerOf(eventRow(EventListItem::ProgramChange, 0, 0, 40)) ==
          ControllerKind::programChange());
}

TEST_CASE("a note or recorded row tells no controller", "[ui][event-list]")
{
    CHECK_FALSE(EventListItems::controllerOf(noteRow(0, 0)).has_value());
    CHECK_FALSE(EventListItems::controllerOf(eventRow(EventListItem::ControlChange, 0, 0, 7, true)).has_value());
}

TEST_CASE("the clicked row's controller picks the selected events of it on its track", "[ui][event-list]")
{
    const std::vector items{
        eventRow(EventListItem::ControlChange, 0, 0, 7), eventRow(EventListItem::ControlChange, 0, 1, 10),
        eventRow(EventListItem::ControlChange, 0, 2, 7), eventRow(EventListItem::ControlChange, 1, 0, 7)};
    CHECK(EventListItems::sameControllerEvents(items, {0, 1, 2, 3}, 2) == std::set<int>{0, 2});
    CHECK(EventListItems::sameControllerEvents(items, {1, 2}, 2) == std::set<int>{2});
}

TEST_CASE("pitch bend rows pick pitch bends whatever their values", "[ui][event-list]")
{
    const std::vector items{eventRow(EventListItem::PitchBend, 0, 0, 100),
                            eventRow(EventListItem::ControlChange, 0, 1, 100),
                            eventRow(EventListItem::PitchBend, 0, 2, 8000)};
    CHECK(EventListItems::sameControllerEvents(items, {0, 1, 2}, 0) == std::set<int>{0, 2});
}

TEST_CASE("a clicked row with no controller picks no events", "[ui][event-list]")
{
    const std::vector items{noteRow(0, 0), eventRow(EventListItem::ControlChange, 0, 0)};
    CHECK(EventListItems::sameControllerEvents(items, {0, 1}, 0).empty());
    CHECK(EventListItems::sameControllerEvents(items, {0, 1}, 5).empty());
}
