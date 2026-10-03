#include "support/ChordTestHelpers.h"
#include "support/KeySignatureTestHelpers.h"
#include "support/MidiEventTestHelpers.h"
#include "support/MidiFileTestHelpers.h"
#include "support/MidiNoteTestHelpers.h"
#include "support/TimeSignatureTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <utility>
#include <vector>

using Catch::Matchers::WithinAbs;

TEST_CASE("notes keep their pitch, velocity, position and length", "[io][midifile]")
{
    MidiSequence seq;
    MidiTrack track;
    track.addNote({60, 100, 0, 480});
    track.addNote({64, 1, 240, 30});
    track.addNote({127, 127, 1920, 3840});
    seq.addTrack(track);

    const auto loaded = midifiletest::saveAndLoad(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getNotes() ==
          std::vector<MidiNote>{{60, 100, 0, 480}, {64, 1, 240, 30}, {127, 127, 1920, 3840}});
}

TEST_CASE("notes are loaded in time order", "[io][midifile]")
{
    MidiSequence seq;
    MidiTrack track;
    track.addNote({64, 100, 960, 480});
    track.addNote({60, 100, 0, 480});
    seq.addTrack(track);

    const auto loaded = midifiletest::saveAndLoad(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}, {64, 100, 960, 480}});
}

TEST_CASE("back-to-back notes of the same pitch keep their lengths", "[io][midifile]")
{
    MidiSequence seq;
    MidiTrack track;
    track.addNote({60, 100, 0, 480});
    track.addNote({60, 90, 480, 240});
    seq.addTrack(track);

    const auto loaded = midifiletest::saveAndLoad(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}, {60, 90, 480, 240}});
}

TEST_CASE("channel events keep their type, position and values", "[io][midifile]")
{
    MidiSequence seq;
    MidiTrack track;
    track.addEvent({MidiEvent::Type::ProgramChange, 0, 41, 0});
    track.addEvent({MidiEvent::Type::ControlChange, 120, 7, 90});
    track.addEvent({MidiEvent::Type::PitchBend, 240, 16383, 0});
    track.addEvent({MidiEvent::Type::ChannelPressure, 360, 64, 0});
    track.addEvent({MidiEvent::Type::KeyPressure, 480, 60, 33});
    seq.addTrack(track);

    const auto loaded = midifiletest::saveAndLoad(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getEvents() == std::vector<MidiEvent>{{MidiEvent::Type::ProgramChange, 0, 41, 0},
                                                                  {MidiEvent::Type::ControlChange, 120, 7, 90},
                                                                  {MidiEvent::Type::PitchBend, 240, 16383, 0},
                                                                  {MidiEvent::Type::ChannelPressure, 360, 64, 0},
                                                                  {MidiEvent::Type::KeyPressure, 480, 60, 33}});
}

TEST_CASE("tracks keep their order, names and channels", "[io][midifile]")
{
    MidiSequence seq;
    MidiTrack piano;
    piano.setName("Piano");
    piano.setChannel(1);
    piano.addNote({60, 100, 0, 480});
    seq.addTrack(piano);
    MidiTrack drums;
    drums.setName("ドラム");
    drums.setChannel(10);
    drums.addNote({36, 100, 0, 120});
    seq.addTrack(drums);

    const auto loaded = midifiletest::saveAndLoad(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 2);
    CHECK(loaded->tracks[0].getName() == "Piano");
    CHECK(loaded->tracks[0].getChannel() == 1);
    CHECK(loaded->tracks[0].getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}});
    CHECK(loaded->tracks[1].getName() == "ドラム");
    CHECK(loaded->tracks[1].getChannel() == 10);
    CHECK(loaded->tracks[1].getNotes() == std::vector<MidiNote>{{36, 100, 0, 120}});
}

TEST_CASE("the resolution is kept", "[io][midifile]")
{
    MidiSequence seq;
    SequenceContents contents;
    contents.timeline.setTicksPerQuarterNote(960);
    MidiTrack track;
    track.addNote({60, 100, 960, 960});
    contents.tracks.push_back(track);
    seq.replaceContents(std::move(contents));

    const auto loaded = midifiletest::saveAndLoad(seq);

    REQUIRE(loaded);
    CHECK(loaded->timeline.getTicksPerQuarterNote() == 960);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getNotes() == std::vector<MidiNote>{{60, 100, 960, 960}});
}

TEST_CASE("tempo changes keep their positions and tempos", "[io][midifile]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.setTempoChanges({{0, 120.0}, {1920, 140.0}, {3840, 72.5}});

    const auto loaded = midifiletest::saveAndLoad(seq);

    REQUIRE(loaded);
    const auto& tempos = loaded->timeline.getTempoChanges();
    REQUIRE(tempos.size() == 3);
    CHECK(tempos[0].tick == 0);
    CHECK_THAT(tempos[0].bpm, WithinAbs(120.0, 1e-3));
    CHECK(tempos[1].tick == 1920);
    CHECK_THAT(tempos[1].bpm, WithinAbs(140.0, 1e-3));
    CHECK(tempos[2].tick == 3840);
    CHECK_THAT(tempos[2].bpm, WithinAbs(72.5, 1e-3));
}

TEST_CASE("time signature changes are kept", "[io][midifile]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.setTimeSignatureChanges({{0, 3, 4}, {1440, 6, 8}, {2880, 4, 4}});

    const auto loaded = midifiletest::saveAndLoad(seq);

    REQUIRE(loaded);
    CHECK(loaded->timeline.getTimeSignatureChanges() ==
          std::vector<TimeSignatureChange>{{0, 3, 4}, {1440, 6, 8}, {2880, 4, 4}});
}

TEST_CASE("key signature changes are kept", "[io][midifile]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.setKeySignatureChanges({{0, -3, true}, {1920, 2, false}, {3840, 0, false}});

    const auto loaded = midifiletest::saveAndLoad(seq);

    REQUIRE(loaded);
    CHECK(loaded->keySignatureChanges ==
          std::vector<KeySignatureChange>{{0, -3, true}, {1920, 2, false}, {3840, 0, false}});
}

TEST_CASE("chord changes keep their root, type and bass", "[io][midifile]")
{
    using namespace chordtest;
    auto slash = chord(1920, "G");
    slash.bassRoot = chord(0, "B").chordRoot;
    slash.bassType = 0;
    const std::vector<ChordChange> chordChanges{chord(0, "C"), slash, chord(3840, "N.C."), xfNoChord(4800)};

    MidiSequence seq;
    seq.addTrack();
    seq.setChordChanges(chordChanges);

    const auto loaded = midifiletest::saveAndLoad(seq);

    REQUIRE(loaded);
    CHECK(loaded->chordChanges == chordChanges);
}

TEST_CASE("a sequence without tracks loads as one empty track", "[io][midifile]")
{
    MidiSequence seq;

    const auto loaded = midifiletest::saveAndLoad(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getNumNotes() == 0);
    CHECK(loaded->tracks[0].getNumEvents() == 0);
}

TEST_CASE("saving to a file that cannot be written fails", "[io][midifile]")
{
    MidiSequence seq;
    seq.addTrack();
    const auto missingDir =
        juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("calliope-missing", "");

    CHECK_FALSE(MidiFileIO::save(seq, missingDir.getChildFile("song.mid")));
}
