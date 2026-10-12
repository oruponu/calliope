#include "edit/EditableNotes.h"
#include "support/MidiNoteTestHelpers.h"
#include "support/NoteRefStringMaker.h"
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

MidiTrack trackWith(std::vector<MidiNote> notes)
{
    MidiTrack track;
    for (const auto& note : notes)
        track.addNote(note);
    return track;
}

std::vector<NoteRef> refsOnTrack(const EditableNotes& notes, int trackIndex)
{
    return notes.onTrack(trackIndex) | std::views::keys | std::ranges::to<std::vector>();
}

constexpr NoteRef recorded(int trackIndex, int id)
{
    return {trackIndex, id, NoteLayer::Recording};
}
} // namespace

TEST_CASE("a sequence ref reads the note on the track", "[edit][editable-notes]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 0, 480}}));
    const EditableNotes notes(seq, nullptr, 0);
    REQUIRE(notes.find({0, 0}).has_value());
    CHECK(*notes.find({0, 0}) == MidiNote{60, 100, 0, 480});
}

TEST_CASE("a recorded ref reads the recording layer by id", "[edit][editable-notes]")
{
    MidiSequence seq;
    const TrackId id = seq.addTrack().getId();
    FakeLayer layer;
    layer.take[id] = {{{62, 80, 240, 120}}, {}, {7}};
    const EditableNotes notes(seq, &layer, 0);
    REQUIRE(notes.find(recorded(0, 7)).has_value());
    CHECK(*notes.find(recorded(0, 7)) == MidiNote{62, 80, 240, 120});
}

TEST_CASE("a held note ends at the playhead", "[edit][editable-notes]")
{
    MidiSequence seq;
    const TrackId id = seq.addTrack().getId();
    FakeLayer layer;
    layer.held = {{id, 64, 70, 480, 9}};
    const EditableNotes notes(seq, &layer, 900);
    REQUIRE(notes.find(recorded(0, 9)).has_value());
    CHECK(*notes.find(recorded(0, 9)) == MidiNote{64, 70, 480, 420});
}

TEST_CASE("a held note at the playhead is one tick long", "[edit][editable-notes]")
{
    MidiSequence seq;
    const TrackId id = seq.addTrack().getId();
    FakeLayer layer;
    layer.held = {{id, 64, 70, 900, 9}};
    const EditableNotes notes(seq, &layer, 900);
    REQUIRE(notes.find(recorded(0, 9)).has_value());
    CHECK(*notes.find(recorded(0, 9)) == MidiNote{64, 70, 900, 1});
}

TEST_CASE("refs to missing notes are not found", "[edit][editable-notes]")
{
    MidiSequence seq;
    const TrackId id = seq.addTrack(trackWith({{60, 100, 0, 480}})).getId();
    FakeLayer layer;
    layer.take[id] = {{{62, 80, 240, 120}}, {}, {7}};
    const EditableNotes notes(seq, &layer, 0);
    CHECK_FALSE(notes.find({0, 1}).has_value());
    CHECK_FALSE(notes.find(recorded(0, 8)).has_value());
    CHECK_FALSE(notes.find({1, 0}).has_value());
    CHECK_FALSE(EditableNotes(seq, nullptr, 0).find(recorded(0, 7)).has_value());
}

TEST_CASE("a track lists its sequence notes, then its recorded notes, then its held notes", "[edit][editable-notes]")
{
    MidiSequence seq;
    const TrackId id = seq.addTrack(trackWith({{60, 100, 0, 480}})).getId();
    FakeLayer layer;
    layer.take[id] = {{{62, 80, 240, 120}}, {}, {7}};
    layer.held = {{id, 64, 70, 480, 9}};
    const EditableNotes notes(seq, &layer, 900);
    CHECK(refsOnTrack(notes, 0) == std::vector<NoteRef>{{0, 0}, recorded(0, 7), recorded(0, 9)});
}

TEST_CASE("another track's recorded notes are not listed", "[edit][editable-notes]")
{
    MidiSequence seq;
    seq.addTrack();
    const TrackId other = seq.addTrack().getId();
    FakeLayer layer;
    layer.take[other] = {{{62, 80, 240, 120}}, {}, {7}};
    layer.held = {{other, 64, 70, 480, 9}};
    const EditableNotes notes(seq, &layer, 900);
    CHECK(refsOnTrack(notes, 0).empty());
    CHECK(refsOnTrack(notes, 1) == std::vector<NoteRef>{recorded(1, 7), recorded(1, 9)});
}

TEST_CASE("notes are ordered by start, then pitch, then ref across both layers", "[edit][editable-notes]")
{
    MidiSequence seq;
    const TrackId id = seq.addTrack(trackWith({{60, 100, 480, 240}, {62, 100, 0, 240}})).getId();
    FakeLayer layer;
    layer.take[id] = {{{60, 80, 480, 120}}, {}, {7}};
    layer.held = {{id, 55, 70, 240, 9}};
    const EditableNotes notes(seq, &layer, 1000);
    CHECK(notes.inTimeOrder(0) == std::vector<NoteRef>{{0, 1}, recorded(0, 9), {0, 0}, recorded(0, 7)});
}

TEST_CASE("only a recorded note whose key is still down is held", "[edit][editable-notes]")
{
    MidiSequence seq;
    const TrackId id = seq.addTrack(trackWith({{60, 100, 0, 480}})).getId();
    const TrackId other = seq.addTrack().getId();
    FakeLayer layer;
    layer.take[id] = {{{62, 80, 240, 120}}, {}, {7}};
    layer.held = {{id, 64, 70, 480, 9}, {other, 65, 70, 480, 10}};
    const EditableNotes notes(seq, &layer, 900);
    CHECK(notes.isHeld(recorded(0, 9)));
    CHECK_FALSE(notes.isHeld(recorded(0, 7)));
    CHECK_FALSE(notes.isHeld({0, 0}));
    CHECK_FALSE(notes.isHeld(recorded(0, 10)));
    CHECK_FALSE(EditableNotes(seq, nullptr, 900).isHeld(recorded(0, 9)));
}
