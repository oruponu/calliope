#include "io/ProjectFileIO.h"
#include "support/MidiNoteTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <juce_core/juce_core.h>
#include <vector>

TEST_CASE("a saved project file opens with the same contents", "[io][projectfile]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.setTrackName(0, "Piano");
    seq.addNote(0, {60, 100, 0, 480});
    juce::TemporaryFile temp(".calliope");

    REQUIRE(ProjectFileIO::save(seq, {}, temp.getFile()));
    const auto loaded = ProjectFileIO::load(temp.getFile());

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getName() == "Piano");
    CHECK(loaded->tracks[0].getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}});
}

TEST_CASE("a file that is not XML does not open", "[io][projectfile]")
{
    juce::TemporaryFile temp(".calliope");
    REQUIRE(temp.getFile().replaceWithText("MThd"));

    CHECK_FALSE(ProjectFileIO::load(temp.getFile()));
}

TEST_CASE("a missing file does not open", "[io][projectfile]")
{
    juce::TemporaryFile temp(".calliope");

    CHECK_FALSE(ProjectFileIO::load(temp.getFile()));
}
