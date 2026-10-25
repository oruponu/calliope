#include "support/KeySignatureStringMaker.h"
#include "support/MidiEventTestHelpers.h"
#include "support/MidiFileTestHelpers.h"
#include "support/MidiNoteTestHelpers.h"
#include "support/SysExEventStringMaker.h"
#include "support/TempoStringMaker.h"
#include "support/TimeSignatureStringMaker.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h> // IWYU pragma: keep
#endif

using MidiFileTest::at;

namespace
{
const std::vector<std::uint8_t> gsReset{0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41};

juce::MidiMessage sysExAt(const std::vector<std::uint8_t>& data, double tick)
{
    return at(juce::MidiMessage::createSysExMessage(data.data(), static_cast<int>(data.size())), tick);
}
} // namespace

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

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 0));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 2);
    CHECK(loaded->tracks[0].getName() == "Ch.1");
    CHECK(loaded->tracks[0].getChannel() == 1);
    CHECK(loaded->tracks[0].getNotes() == std::vector<MidiNote>{{60, 90, 0, 480}});
    CHECK(loaded->tracks[0].getEvents() == std::vector<MidiEvent>{{MidiEvent::Type::ControlChange, 240, 7, 80, true}});
    CHECK(loaded->tracks[1].getName() == "Ch.10");
    CHECK(loaded->tracks[1].getChannel() == 10);
    CHECK(loaded->tracks[1].getNotes() == std::vector<MidiNote>{{36, 100, 0, 120}, {38, 110, 480, 120}});
    CHECK(loaded->tracks[1].getEvents().empty());
}

TEST_CASE("a format 0 file puts its sysex on a SysEx track first", "[io][midifile][sysex]")
{
    juce::MidiMessageSequence events;
    events.addEvent(sysExAt(gsReset, 0));
    events.addEvent(at(juce::MidiMessage::noteOn(1, 60, juce::uint8{90}), 0));
    events.addEvent(at(juce::MidiMessage::noteOn(10, 36, juce::uint8{100}), 0));
    events.addEvent(at(juce::MidiMessage::noteOff(10, 36), 120));
    events.addEvent(at(juce::MidiMessage::noteOff(1, 60), 480));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(events);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 0));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 3);
    CHECK(loaded->tracks[0].getName() == "SysEx");
    CHECK(loaded->tracks[0].getChannel() == 1);
    CHECK(loaded->tracks[0].getSysExEvents() == std::vector<SysExEvent>{{0, gsReset}});
    CHECK(loaded->tracks[0].getNotes().empty());
    CHECK(loaded->tracks[0].getEvents().empty());
    CHECK(loaded->tracks[1].getName() == "Ch.1");
    CHECK(loaded->tracks[1].getNotes() == std::vector<MidiNote>{{60, 90, 0, 480}});
    CHECK(loaded->tracks[2].getName() == "Ch.10");
    CHECK(loaded->tracks[2].getNotes() == std::vector<MidiNote>{{36, 100, 0, 120}});
}

TEST_CASE("a format 0 file with only sysex loads as one SysEx track", "[io][midifile][sysex]")
{
    juce::MidiMessageSequence events;
    events.addEvent(sysExAt(gsReset, 0));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(events);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 0));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getName() == "SysEx");
    CHECK(loaded->tracks[0].getSysExEvents() == std::vector<SysExEvent>{{0, gsReset}});
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

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getChannel() == 3);
    CHECK(loaded->tracks[0].getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}});
    CHECK(loaded->tracks[0].getEvents() == std::vector<MidiEvent>{{MidiEvent::Type::ProgramChange, 0, 5, 0}});
}

TEST_CASE("a format 1 track keeps its sysex in file order", "[io][midifile][sysex]")
{
    const std::vector<std::uint8_t> parameterChange{0x41, 0x10, 0x42, 0x12, 0x40, 0x11, 0x15, 0x00, 0x1A};
    const std::vector<std::uint8_t> xgReset{0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00};
    juce::MidiMessageSequence conductor;
    juce::MidiMessageSequence events;
    events.addEvent(sysExAt(gsReset, 0));
    events.addEvent(sysExAt(parameterChange, 0));
    events.addEvent(sysExAt(xgReset, 480));
    events.addEvent(at(juce::MidiMessage::noteOn(2, 60, juce::uint8{100}), 0));
    events.addEvent(at(juce::MidiMessage::noteOff(2, 60), 960));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);
    midiFile.addTrack(events);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getChannel() == 2);
    CHECK(loaded->tracks[0].getSysExEvents() ==
          std::vector<SysExEvent>{{0, gsReset}, {0, parameterChange}, {480, xgReset}});
}

TEST_CASE("a format 1 first track with sysex becomes a track", "[io][midifile][sysex]")
{
    juce::MidiMessageSequence conductor;
    conductor.addEvent(at(juce::MidiMessage::tempoMetaEvent(500000), 0));
    conductor.addEvent(sysExAt(gsReset, 0));
    juce::MidiMessageSequence events;
    events.addEvent(at(juce::MidiMessage::noteOn(3, 60, juce::uint8{100}), 0));
    events.addEvent(at(juce::MidiMessage::noteOff(3, 60), 480));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);
    midiFile.addTrack(events);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 2);
    CHECK(loaded->tracks[0].getChannel() == 1);
    CHECK(loaded->tracks[0].getSysExEvents() == std::vector<SysExEvent>{{0, gsReset}});
    CHECK(loaded->tracks[1].getChannel() == 3);
    CHECK(loaded->tracks[1].getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}});
}

TEST_CASE("a sysex before the first channel event does not decide the channel", "[io][midifile][sysex]")
{
    juce::MidiMessageSequence events;
    events.addEvent(sysExAt(gsReset, 0));
    events.addEvent(at(juce::MidiMessage::noteOn(5, 60, juce::uint8{100}), 0));
    events.addEvent(at(juce::MidiMessage::noteOff(5, 60), 480));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(events);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getChannel() == 5);
    CHECK(loaded->tracks[0].getSysExEvents() == std::vector<SysExEvent>{{0, gsReset}});
}

TEST_CASE("a track with only sysex takes its channel from the channel prefix", "[io][midifile][sysex]")
{
    juce::MidiMessageSequence conductor;
    juce::MidiMessageSequence events;
    events.addEvent(at(juce::MidiMessage::midiChannelMetaEvent(4), 0));
    events.addEvent(sysExAt(gsReset, 0));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);
    midiFile.addTrack(events);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getChannel() == 4);
    CHECK(loaded->tracks[0].getSysExEvents() == std::vector<SysExEvent>{{0, gsReset}});
}

TEST_CASE("an empty sysex is dropped", "[io][midifile][sysex]")
{
    std::uint8_t unused = 0;
    juce::MidiMessageSequence conductor;
    juce::MidiMessageSequence events;
    events.addEvent(at(juce::MidiMessage::createSysExMessage(&unused, 0), 0));
    events.addEvent(at(juce::MidiMessage::noteOn(1, 60, juce::uint8{100}), 0));
    events.addEvent(at(juce::MidiMessage::noteOff(1, 60), 480));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);
    midiFile.addTrack(events);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getSysExEvents().empty());
    CHECK(loaded->tracks[0].getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}});
}

TEST_CASE("a sysex without its end byte is dropped", "[io][midifile][sysex]")
{
    const std::uint8_t raw[] = {0xF0, 0x41, 0x10};
    juce::MidiMessageSequence conductor;
    juce::MidiMessageSequence events;
    events.addEvent(at(juce::MidiMessage(raw, static_cast<int>(sizeof(raw))), 0));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);
    midiFile.addTrack(events);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    for (const auto& track : loaded->tracks)
        CHECK(track.getSysExEvents().empty());
}

TEST_CASE("format 1 tracks after the first are loaded even when empty", "[io][midifile]")
{
    juce::MidiMessageSequence conductor;
    conductor.addEvent(at(juce::MidiMessage::textMetaEvent(3, "Song Title"), 0));
    juce::MidiMessageSequence named;
    named.addEvent(at(juce::MidiMessage::textMetaEvent(3, "Empty"), 0));
    juce::MidiMessageSequence events;
    events.addEvent(at(juce::MidiMessage::noteOn(2, 60, juce::uint8{100}), 0));
    events.addEvent(at(juce::MidiMessage::noteOff(2, 60), 480));
    juce::MidiMessageSequence blank;
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);
    midiFile.addTrack(named);
    midiFile.addTrack(events);
    midiFile.addTrack(blank);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 3);
    CHECK(loaded->tracks[0].getName() == "Empty");
    CHECK(loaded->tracks[0].getNumNotes() == 0);
    CHECK(loaded->tracks[1].getChannel() == 2);
    CHECK(loaded->tracks[1].getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}});
    CHECK(loaded->tracks[2].getName().empty());
    CHECK(loaded->tracks[2].getNumNotes() == 0);
}

TEST_CASE("a channel prefix sets the channel of a track without channel events", "[io][midifile]")
{
    juce::MidiMessageSequence conductor;
    juce::MidiMessageSequence empty;
    empty.addEvent(at(juce::MidiMessage::midiChannelMetaEvent(7), 0));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);
    midiFile.addTrack(empty);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getChannel() == 7);
}

TEST_CASE("a track's first channel event takes precedence over its channel prefix", "[io][midifile]")
{
    juce::MidiMessageSequence conductor;
    juce::MidiMessageSequence events;
    events.addEvent(at(juce::MidiMessage::midiChannelMetaEvent(7), 0));
    events.addEvent(at(juce::MidiMessage::noteOn(3, 60, juce::uint8{100}), 0));
    events.addEvent(at(juce::MidiMessage::noteOff(3, 60), 480));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);
    midiFile.addTrack(events);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getChannel() == 3);
}

TEST_CASE("a first format 1 track without channel events is not loaded as a track", "[io][midifile]")
{
    juce::MidiMessageSequence conductor;
    conductor.addEvent(at(juce::MidiMessage::textMetaEvent(3, "Song Title"), 0));
    conductor.addEvent(at(juce::MidiMessage::tempoMetaEvent(500000), 0));
    juce::MidiMessageSequence events;
    events.addEvent(at(juce::MidiMessage::noteOn(1, 60, juce::uint8{100}), 0));
    events.addEvent(at(juce::MidiMessage::noteOff(1, 60), 480));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);
    midiFile.addTrack(events);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getName().empty());
    CHECK(loaded->tracks[0].getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}});
}

TEST_CASE("a note without a note-off lasts one quarter note", "[io][midifile]")
{
    juce::MidiMessageSequence events;
    events.addEvent(at(juce::MidiMessage::noteOn(1, 60, juce::uint8{100}), 192));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(96);
    midiFile.addTrack(events);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

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

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    CHECK(loaded->timeline.getTempoChanges() == std::vector<TempoChange>{{0, 120.0}, {960, 60.0}});
}

TEST_CASE("a tempo of zero microseconds per quarter note is ignored", "[io][midifile]")
{
    juce::MidiMessageSequence conductor;
    conductor.addEvent(at(juce::MidiMessage::tempoMetaEvent(0), 0));
    conductor.addEvent(at(juce::MidiMessage::tempoMetaEvent(500000), 960));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    CHECK(loaded->timeline.getTempoChanges() == std::vector<TempoChange>{{0, 120.0}, {960, 120.0}});
}

TEST_CASE("a mid-bar time signature replaces the one at its bar start", "[io][midifile]")
{
    juce::MidiMessageSequence conductor;
    conductor.addEvent(at(juce::MidiMessage::timeSignatureMetaEvent(3, 4), 0));
    conductor.addEvent(at(juce::MidiMessage::timeSignatureMetaEvent(6, 8), 960));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    CHECK(loaded->timeline.getTimeSignatureChanges() == std::vector<TimeSignatureChange>{{0, 6, 8}});
}

TEST_CASE("a time signature with a zero numerator is ignored", "[io][midifile]")
{
    juce::MidiMessageSequence conductor;
    conductor.addEvent(at(juce::MidiMessage::timeSignatureMetaEvent(3, 4), 0));
    conductor.addEvent(at(juce::MidiMessage::timeSignatureMetaEvent(0, 4), 1440));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    CHECK(loaded->timeline.getTimeSignatureChanges() == std::vector<TimeSignatureChange>{{0, 3, 4}});
}

TEST_CASE("a time signature whose beat is shorter than a tick is ignored", "[io][midifile]")
{
    juce::MidiMessageSequence conductor;
    conductor.addEvent(at(juce::MidiMessage::timeSignatureMetaEvent(3, 4), 0));
    conductor.addEvent(at(juce::MidiMessage::timeSignatureMetaEvent(4, 4096), 1440));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    CHECK(loaded->timeline.getTimeSignatureChanges() == std::vector<TimeSignatureChange>{{0, 3, 4}});
}

TEST_CASE("a time signature whose denominator does not fit in an int is ignored", "[io][midifile]")
{
    const std::uint8_t raw[] = {0xFF, 0x58, 0x04, 0x04, 0x20, 0x18, 0x08};
    juce::MidiMessageSequence conductor;
    conductor.addEvent(at(juce::MidiMessage::timeSignatureMetaEvent(3, 4), 0));
    conductor.addEvent(at(juce::MidiMessage(raw, static_cast<int>(sizeof(raw))), 1440));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    CHECK(loaded->timeline.getTimeSignatureChanges() == std::vector<TimeSignatureChange>{{0, 3, 4}});
}

TEST_CASE("a time signature with fewer than two data bytes is ignored", "[io][midifile]")
{
    const std::uint8_t raw[] = {0xFF, 0x58, 0x01, 0x02};
    juce::MidiMessageSequence conductor;
    conductor.addEvent(at(juce::MidiMessage::timeSignatureMetaEvent(3, 4), 0));
    conductor.addEvent(at(juce::MidiMessage(raw, static_cast<int>(sizeof(raw))), 1440));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    CHECK(loaded->timeline.getTimeSignatureChanges() == std::vector<TimeSignatureChange>{{0, 3, 4}});
}

TEST_CASE("a key signature beyond seven sharps or flats is ignored", "[io][midifile]")
{
    const std::uint8_t raw[] = {0xFF, 0x59, 0x02, 0x08, 0x00};
    juce::MidiMessageSequence conductor;
    conductor.addEvent(at(juce::MidiMessage::keySignatureMetaEvent(2, false), 0));
    conductor.addEvent(at(juce::MidiMessage(raw, static_cast<int>(sizeof(raw))), 1920));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    CHECK(loaded->keySignatureChanges == std::vector<KeySignatureChange>{{0, 2, false}});
}

TEST_CASE("a key signature with fewer than two data bytes is ignored", "[io][midifile]")
{
    const std::uint8_t raw[] = {0xFF, 0x59, 0x01, 0x03};
    juce::MidiMessageSequence conductor;
    conductor.addEvent(at(juce::MidiMessage::keySignatureMetaEvent(2, false), 0));
    conductor.addEvent(at(juce::MidiMessage(raw, static_cast<int>(sizeof(raw))), 1920));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    CHECK(loaded->keySignatureChanges == std::vector<KeySignatureChange>{{0, 2, false}});
}

TEST_CASE("sequencer-specific meta events that are not XF chords are ignored", "[io][midifile]")
{
    const std::uint8_t raw[] = {0xFF, 0x7F, 0x07, 0x00, 0x00, 0x41, 0x01, 0x31, 0x00, 0x7F};
    juce::MidiMessageSequence conductor;
    conductor.addEvent(at(juce::MidiMessage(raw, static_cast<int>(sizeof(raw))), 0));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

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
    const auto standard = MidiFileTest::toBytes(midiFile, 0);

    const std::uint8_t xfih[] = {'X', 'F', 'I', 'H', 0, 0, 0, 4, 1, 2, 3, 4};
    const std::uint8_t xfkm[] = {'X', 'F', 'K', 'M', 0, 0, 0, 2, 5, 6};
    constexpr size_t headerSize = 14;
    juce::MemoryBlock bytes;
    bytes.append(standard.getData(), headerSize);
    bytes.append(xfih, sizeof(xfih));
    bytes.append(static_cast<const char*>(standard.getData()) + headerSize, standard.getSize() - headerSize);
    bytes.append(xfkm, sizeof(xfkm));

    const auto loaded = MidiFileTest::loadBytes(bytes);

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

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

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

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getName() == "ピアノ");
}

TEST_CASE("data that is not a MIDI file fails to load", "[io][midifile]")
{
    const char text[] = "this is not a MIDI file";
    juce::MemoryBlock bytes(text, sizeof(text));

    CHECK_FALSE(MidiFileTest::loadBytes(bytes));
}

TEST_CASE("a missing file fails to load", "[io][midifile]")
{
    const auto missing =
        juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("calliope-missing", ".mid");

    CHECK_FALSE(MidiFileIO::load(missing));
}
