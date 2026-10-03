#include "model/MidiSequence.h"
#include "support/TempoStringMaker.h"
#include "support/TimeSignatureStringMaker.h"
#include <catch2/catch_test_macros.hpp>
#include <utility>
#include <vector>

TEST_CASE("a new timeline starts at 120 bpm in 4/4 with 480 ticks per quarter note", "[model][timeline]")
{
    const TimelineMap timeline;
    CHECK(timeline.getTicksPerQuarterNote() == 480);
    CHECK(timeline.getTempoChanges() == std::vector<TempoChange>{{0, 120.0}});
    CHECK(timeline.getTimeSignatureChanges() == std::vector<TimeSignatureChange>{{0, 4, 4}});
}

TEST_CASE("replacing contents with empty contents restores the default timeline and empties the rest",
          "[model][sequence]")
{
    MidiSequence seq;
    SequenceContents custom;
    custom.timeline.setTicksPerQuarterNote(960);
    custom.timeline.setTempoChanges({{0, 90.0}, {1920, 140.0}});
    custom.timeline.setTimeSignatureChanges({{0, 3, 4}});
    custom.tracks.emplace_back();
    custom.keySignatureChanges = {{0, 2, false}};
    custom.chordChanges = {ChordChange::noChord(0)};
    seq.replaceContents(std::move(custom));

    seq.replaceContents({});

    CHECK(seq.getTimeline().getTicksPerQuarterNote() == 480);
    CHECK(seq.getTimeline().getTempoChanges() == std::vector<TempoChange>{{0, 120.0}});
    CHECK(seq.getTimeline().getTimeSignatureChanges() == std::vector<TimeSignatureChange>{{0, 4, 4}});
    CHECK(seq.getKeySignatureChanges().empty());
    CHECK(seq.getChordChanges().empty());
    CHECK(seq.getNumTracks() == 0);
}
