#include "model/MidiNote.h"
#include "model/MidiSequence.h"
#include "model/PluginAssignment.h"
#include "model/TrackDuplication.h"
#include "support/MidiNoteTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>
#include <vector>

namespace
{
void addNamedTracks(MidiSequence& seq, const std::vector<std::string>& names)
{
    for (const auto& name : names)
    {
        seq.addTrack();
        seq.setTrackName(seq.getNumTracks() - 1, name);
    }
}
} // namespace

TEST_CASE("a track copy keeps the notes and the channel", "[model][track]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.setTrackChannel(0, 10);
    seq.addNote(0, {36, 100, 0, 240});
    seq.addNote(0, {38, 90, 480, 240});

    const MidiTrack copy = TrackDuplication::makeCopy(seq, 0);
    CHECK(copy.getChannel() == 10);
    CHECK(copy.getNotes() == std::vector<MidiNote>{{36, 100, 0, 240}, {38, 90, 480, 240}});
}

TEST_CASE("a copy of a named track gets number 2", "[model][track]")
{
    MidiSequence seq;
    addNamedTracks(seq, {"Piano"});
    CHECK(TrackDuplication::makeCopy(seq, 0).getName() == "Piano 2");
}

TEST_CASE("a track copy gets the number after the highest one in use", "[model][track]")
{
    MidiSequence seq;
    addNamedTracks(seq, {"Piano", "Piano 3", "Bass 5"});
    CHECK(TrackDuplication::makeCopy(seq, 0).getName() == "Piano 4");
}

TEST_CASE("a copy of a numbered track continues the numbering of its base name", "[model][track]")
{
    MidiSequence seq;
    addNamedTracks(seq, {"Piano", "Piano 2"});
    CHECK(TrackDuplication::makeCopy(seq, 1).getName() == "Piano 3");
}

TEST_CASE("a name ending in a word gets a number appended", "[model][track]")
{
    MidiSequence seq;
    addNamedTracks(seq, {"Bass Synth", "Synth 2"});
    CHECK(TrackDuplication::makeCopy(seq, 0).getName() == "Bass Synth 2");
}

TEST_CASE("a copy of an unnamed track stays unnamed", "[model][track]")
{
    MidiSequence seq;
    seq.addTrack();
    CHECK(TrackDuplication::makeCopy(seq, 0).getName().empty());
}

TEST_CASE("a copy of a track with a plugin plays through the original's plugin", "[model][track]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.setTrackPluginAssignment(0, std::make_shared<const PluginAssignment>());
    seq.setTrackOutputDestination(0, MidiTrack::OutputDestination::Plugin);

    const MidiTrack copy = TrackDuplication::makeCopy(seq, 0);
    CHECK(copy.getPluginAssignment() == nullptr);
    CHECK(copy.getRouteTarget() == seq.getTrack(0).getId());
    CHECK(copy.getOutputDestination() == MidiTrack::OutputDestination::Plugin);
}

TEST_CASE("a copy of a track with a plugin routed elsewhere keeps its route target", "[model][track]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    const TrackId target = seq.getTrack(0).getId();
    seq.setTrackPluginAssignment(1, std::make_shared<const PluginAssignment>());
    seq.setTrackRouteTarget(1, target);
    seq.setTrackOutputDestination(1, MidiTrack::OutputDestination::Plugin);

    const MidiTrack copy = TrackDuplication::makeCopy(seq, 1);
    CHECK(copy.getPluginAssignment() == nullptr);
    CHECK(copy.getRouteTarget() == target);
}

TEST_CASE("a copy of a routed track keeps its route target", "[model][track]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    const TrackId target = seq.getTrack(0).getId();
    seq.setTrackRouteTarget(1, target);
    seq.setTrackOutputDestination(1, MidiTrack::OutputDestination::Plugin);

    CHECK(TrackDuplication::makeCopy(seq, 1).getRouteTarget() == target);
}
