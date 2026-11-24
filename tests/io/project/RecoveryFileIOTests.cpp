#include "io/project/ProjectXmlNames.h"
#include "io/project/RecoveryFileIO.h"
#include "support/MidiNoteTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <juce_core/juce_core.h>
#include <vector>

TEST_CASE("a recovery file brings back the song and its original file", "[io][recovery]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.setTrackName(0, "Piano");
    seq.addNote(0, {60, 100, 0, 480});
    const auto original = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("MySong.calliope");
    juce::TemporaryFile temp(".calliope");

    REQUIRE(RecoveryFileIO::save(seq, {}, original, temp.getFile()));
    const auto recovered = RecoveryFileIO::load(temp.getFile());

    REQUIRE(recovered);
    CHECK(recovered->originalFile == original);
    REQUIRE(recovered->contents.tracks.size() == 1);
    CHECK(recovered->contents.tracks[0].getName() == "Piano");
    CHECK(recovered->contents.tracks[0].getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}});
}

TEST_CASE("a recovery file of an untitled song brings back no original file", "[io][recovery]")
{
    MidiSequence seq;
    seq.addTrack();
    juce::TemporaryFile temp(".calliope");

    REQUIRE(RecoveryFileIO::save(seq, {}, juce::File{}, temp.getFile()));
    const auto recovered = RecoveryFileIO::load(temp.getFile());

    REQUIRE(recovered);
    CHECK(recovered->originalFile == juce::File{});
}

TEST_CASE("saving creates the recovery folder when it does not exist", "[io][recovery]")
{
    MidiSequence seq;
    seq.addTrack();
    juce::TemporaryFile dir;
    const auto file = dir.getFile().getChildFile("Recovery").getChildFile("Recovery.calliope");

    REQUIRE(RecoveryFileIO::save(seq, {}, juce::File{}, file));

    CHECK(file.existsAsFile());
    dir.getFile().deleteRecursively();
}

TEST_CASE("a file that is not XML does not open as a recovery file", "[io][recovery]")
{
    juce::TemporaryFile temp(".calliope");
    REQUIRE(temp.getFile().replaceWithText("MThd"));

    CHECK_FALSE(RecoveryFileIO::load(temp.getFile()));
}

TEST_CASE("a recovery file from another project version does not open", "[io][recovery]")
{
    MidiSequence seq;
    seq.addTrack();
    juce::TemporaryFile temp(".calliope");
    REQUIRE(RecoveryFileIO::save(seq, {}, juce::File{}, temp.getFile()));
    const auto root = juce::parseXML(temp.getFile());
    REQUIRE(root != nullptr);
    root->setAttribute(ProjectXmlNames::versionAttr, "999");
    REQUIRE(root->writeTo(temp.getFile()));

    CHECK_FALSE(RecoveryFileIO::load(temp.getFile()));
}
