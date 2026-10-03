#include "support/MidiEventTestHelpers.h"
#include "support/MidiFileTestHelpers.h"
#include "support/MidiNoteTestHelpers.h"
#include "support/TempoStringMaker.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h> // IWYU pragma: keep
#endif

using midifiletest::at;

TEST_CASE("a format 0 file is split into one track per channel", "[io][midifile]")
{
    juce::MidiMessageSequence events;
    events.addEvent(at(juce::MidiMessage::noteOn(10, 36, juce::uint8{100}), 0));
    events.addEvent(at(juce::MidiMessage::noteOn(1, 60, juce::uint8{90}), 0));
    events.addEvent(at(juce::MidiMessage::noteOff(10, 36), 120));
    events.addEvent(at(juce::MidiMessage::controllerEvent(1, 7, 80), 240));
    events.addEvent(at(juce::MidiMessage::noteOn(10, 38, juce::uint8{110}), 480));
    events.addEvent(at(juce::MidiMessage::noteOff(1, 60), 480));
    events.addEvent(at(juce::MidiMessage::noteOff(10, 38), 600));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(events);

    const auto loaded = midifiletest::loadBytes(midifiletest::toBytes(midiFile, 0));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 2);
    CHECK(loaded->tracks[0].getName() == "Ch.1");
    CHECK(loaded->tracks[0].getChannel() == 1);
    CHECK(loaded->tracks[0].getNotes() == std::vector<MidiNote>{{60, 90, 0, 480}});
    CHECK(loaded->tracks[0].getEvents() == std::vector<MidiEvent>{{MidiEvent::Type::ControlChange, 240, 7, 80}});
    CHECK(loaded->tracks[1].getName() == "Ch.10");
    CHECK(loaded->tracks[1].getChannel() == 10);
    CHECK(loaded->tracks[1].getNotes() == std::vector<MidiNote>{{36, 100, 0, 120}, {38, 110, 480, 120}});
    CHECK(loaded->tracks[1].getEvents().empty());
}

TEST_CASE("a format 1 track takes its channel from its first channel event", "[io][midifile]")
{
    juce::MidiMessageSequence conductor;
    conductor.addEvent(at(juce::MidiMessage::tempoMetaEvent(500000), 0));
    juce::MidiMessageSequence events;
    events.addEvent(at(juce::MidiMessage::programChange(3, 5), 0));
    events.addEvent(at(juce::MidiMessage::noteOn(3, 60, juce::uint8{100}), 0));
    events.addEvent(at(juce::MidiMessage::noteOff(3, 60), 480));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);
    midiFile.addTrack(events);

    const auto loaded = midifiletest::loadBytes(midifiletest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getChannel() == 3);
    CHECK(loaded->tracks[0].getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}});
    CHECK(loaded->tracks[0].getEvents() == std::vector<MidiEvent>{{MidiEvent::Type::ProgramChange, 0, 5, 0}});
}

TEST_CASE("a note without a note-off lasts one quarter note", "[io][midifile]")
{
    juce::MidiMessageSequence events;
    events.addEvent(at(juce::MidiMessage::noteOn(1, 60, juce::uint8{100}), 192));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(96);
    midiFile.addTrack(events);

    const auto loaded = midifiletest::loadBytes(midifiletest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getNotes() == std::vector<MidiNote>{{60, 100, 192, 96}});
}

TEST_CASE("a file without a tempo at tick 0 starts at 120 bpm", "[io][midifile]")
{
    juce::MidiMessageSequence conductor;
    conductor.addEvent(at(juce::MidiMessage::tempoMetaEvent(1000000), 960));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);

    const auto loaded = midifiletest::loadBytes(midifiletest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    CHECK(loaded->timeline.getTempoChanges() == std::vector<TempoChange>{{0, 120.0}, {960, 60.0}});
}

TEST_CASE("sequencer-specific meta events that are not XF chords are ignored", "[io][midifile]")
{
    const std::uint8_t raw[] = {0xFF, 0x7F, 0x07, 0x00, 0x00, 0x41, 0x01, 0x31, 0x00, 0x7F};
    juce::MidiMessageSequence conductor;
    conductor.addEvent(at(juce::MidiMessage(raw, static_cast<int>(sizeof(raw))), 0));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);

    const auto loaded = midifiletest::loadBytes(midifiletest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    CHECK(loaded->chordChanges.empty());
}

TEST_CASE("non-standard chunks are skipped", "[io][midifile]")
{
    juce::MidiMessageSequence events;
    events.addEvent(at(juce::MidiMessage::noteOn(1, 60, juce::uint8{100}), 0));
    events.addEvent(at(juce::MidiMessage::noteOff(1, 60), 480));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(events);
    const auto standard = midifiletest::toBytes(midiFile, 0);

    const std::uint8_t xfih[] = {'X', 'F', 'I', 'H', 0, 0, 0, 4, 1, 2, 3, 4};
    const std::uint8_t xfkm[] = {'X', 'F', 'K', 'M', 0, 0, 0, 2, 5, 6};
    constexpr size_t headerSize = 14;
    juce::MemoryBlock bytes;
    bytes.append(standard.getData(), headerSize);
    bytes.append(xfih, sizeof(xfih));
    bytes.append(static_cast<const char*>(standard.getData()) + headerSize, standard.getSize() - headerSize);
    bytes.append(xfkm, sizeof(xfkm));

    const auto loaded = midifiletest::loadBytes(bytes);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}});
}

TEST_CASE("a file with SMPTE timing falls back to 480 ticks per quarter note", "[io][midifile]")
{
    juce::MidiMessageSequence events;
    events.addEvent(at(juce::MidiMessage::noteOn(1, 60, juce::uint8{100}), 0));
    events.addEvent(at(juce::MidiMessage::noteOff(1, 60), 40));
    juce::MidiFile midiFile;
    midiFile.setSmpteTimeFormat(25, 40);
    midiFile.addTrack(events);

    const auto loaded = midifiletest::loadBytes(midifiletest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    CHECK(loaded->timeline.getTicksPerQuarterNote() == 480);
}

TEST_CASE("a track name in the system code page is decoded", "[io][midifile]")
{
#ifdef _WIN32
    if (GetACP() != 932)
        SKIP("the system code page is not Shift_JIS");
#else
    SKIP("the system code page is only checked on Windows");
#endif

    const std::uint8_t name[] = {0xFF, 0x03, 0x06, 0x83, 0x73, 0x83, 0x41, 0x83, 0x6D};
    juce::MidiMessageSequence events;
    events.addEvent(at(juce::MidiMessage(name, static_cast<int>(sizeof(name))), 0));
    events.addEvent(at(juce::MidiMessage::noteOn(1, 60, juce::uint8{100}), 0));
    events.addEvent(at(juce::MidiMessage::noteOff(1, 60), 480));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(events);

    const auto loaded = midifiletest::loadBytes(midifiletest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getName() == "ピアノ");
}

TEST_CASE("data that is not a MIDI file fails to load", "[io][midifile]")
{
    const char text[] = "this is not a MIDI file";
    juce::MemoryBlock bytes(text, sizeof(text));

    CHECK_FALSE(midifiletest::loadBytes(bytes));
}

TEST_CASE("a missing file fails to load", "[io][midifile]")
{
    const auto missing =
        juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("calliope-missing", ".mid");

    CHECK_FALSE(MidiFileIO::load(missing));
}
