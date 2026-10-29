#include "support/ChordTestHelpers.h"
#include "support/KeySignatureStringMaker.h"
#include "support/MetaEventStringMaker.h"
#include "support/MidiEventTestHelpers.h"
#include "support/MidiFileTestHelpers.h"
#include "support/MidiNoteTestHelpers.h"
#include "support/SysExEventStringMaker.h"
#include "support/TimeSignatureStringMaker.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdint>
#include <utility>
#include <vector>

using Catch::Matchers::WithinAbs;

namespace
{
const std::vector<std::uint8_t> gsReset{0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41};
}

TEST_CASE("notes keep their pitch, velocity, position and length", "[io][midifile]")
{
    MidiSequence seq;
    MidiTrack track;
    track.addNote({60, 100, 0, 480});
    track.addNote({64, 1, 240, 30});
    track.addNote({127, 127, 1920, 3840});
    seq.addTrack(track);

    const auto loaded = MidiFileTest::saveAndLoad(seq);

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

    const auto loaded = MidiFileTest::saveAndLoad(seq);

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

    const auto loaded = MidiFileTest::saveAndLoad(seq);

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

    const auto loaded = MidiFileTest::saveAndLoad(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getEvents() == std::vector<MidiEvent>{{MidiEvent::Type::ProgramChange, 0, 41, 0},
                                                                  {MidiEvent::Type::ControlChange, 120, 7, 90, true},
                                                                  {MidiEvent::Type::PitchBend, 240, 16383, 0, true},
                                                                  {MidiEvent::Type::ChannelPressure, 360, 64, 0},
                                                                  {MidiEvent::Type::KeyPressure, 480, 60, 33}});
}

TEST_CASE("sysex keeps its bytes, position and order", "[io][midifile][sysex]")
{
    const std::vector<std::uint8_t> parameterChange{0x41, 0x10, 0x42, 0x12, 0x40, 0x11, 0x15, 0x00, 0x1A};
    const std::vector<std::uint8_t> xgReset{0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00};
    MidiSequence seq;
    MidiTrack track;
    track.addSysEx({0, gsReset});
    track.addSysEx({0, parameterChange});
    track.addSysEx({960, xgReset});
    seq.addTrack(track);

    const auto loaded = MidiFileTest::saveAndLoad(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getSysExEvents() ==
          std::vector<SysExEvent>{{0, gsReset}, {0, parameterChange}, {960, xgReset}});
}

TEST_CASE("a long sysex survives the round trip", "[io][midifile][sysex]")
{
    std::vector<std::uint8_t> data;
    data.reserve(300);
    for (int i = 0; i < 300; ++i)
        data.push_back(static_cast<std::uint8_t>(i % 128));

    MidiSequence seq;
    MidiTrack track;
    track.addSysEx({240, data});
    seq.addTrack(track);

    const auto loaded = MidiFileTest::saveAndLoad(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getSysExEvents() == std::vector<SysExEvent>{{240, data}});
}

TEST_CASE("a track with only sysex keeps its channel through a midi file", "[io][midifile][sysex]")
{
    MidiSequence seq;
    MidiTrack track;
    track.setChannel(5);
    track.addSysEx({0, gsReset});
    seq.addTrack(track);

    const auto loaded = MidiFileTest::saveAndLoad(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getChannel() == 5);
    CHECK(loaded->tracks[0].getSysExEvents() == std::vector<SysExEvent>{{0, gsReset}});
}

TEST_CASE("at the same tick, sysex is written before channel events and notes", "[io][midifile][sysex]")
{
    MidiSequence seq;
    MidiTrack track;
    track.addEvent({MidiEvent::Type::ControlChange, 0, 7, 100});
    track.addNote({60, 100, 0, 480});
    track.addSysEx({0, gsReset});
    seq.addTrack(track);
    juce::TemporaryFile temp(".mid");

    REQUIRE(MidiFileIO::save(seq, temp.getFile()));
    juce::FileInputStream stream(temp.getFile());
    REQUIRE(stream.openedOk());
    juce::MidiFile midiFile;
    REQUIRE(midiFile.readFrom(stream));
    REQUIRE(midiFile.getNumTracks() == 2);

    const auto* events = midiFile.getTrack(1);
    const juce::MidiMessage* firstNonMeta = nullptr;
    for (int i = 0; i < events->getNumEvents(); ++i)
    {
        const auto& message = events->getEventPointer(i)->message;
        if (!message.isMetaEvent())
        {
            firstNonMeta = &message;
            break;
        }
    }

    REQUIRE(firstNonMeta != nullptr);
    CHECK(firstNonMeta->isSysEx());
}

TEST_CASE("the end of track comes after the last sysex", "[io][midifile][sysex]")
{
    MidiSequence seq;
    MidiTrack track;
    track.addNote({60, 100, 0, 480});
    track.addSysEx({1920, gsReset});
    seq.addTrack(track);
    juce::TemporaryFile temp(".mid");

    REQUIRE(MidiFileIO::save(seq, temp.getFile()));
    juce::FileInputStream stream(temp.getFile());
    REQUIRE(stream.openedOk());
    juce::MidiFile midiFile;
    REQUIRE(midiFile.readFrom(stream));
    REQUIRE(midiFile.getNumTracks() == 2);

    const auto* events = midiFile.getTrack(1);
    const juce::MidiMessage* endOfTrack = nullptr;
    for (int i = 0; i < events->getNumEvents(); ++i)
    {
        const auto& message = events->getEventPointer(i)->message;
        if (message.isEndOfTrackMetaEvent())
        {
            endOfTrack = &message;
            break;
        }
    }

    REQUIRE(endOfTrack != nullptr);
    CHECK(endOfTrack->getTimeStamp() == 1920);
}

TEST_CASE("meta events keep their bytes, position and order", "[io][midifile][meta]")
{
    const std::vector<MetaEvent> songMeta{{0, 0x03, {0x53, 0x6F}}, {0, 0x06, {0x49}}, {1920, 0x06, {0x56}}};
    const std::vector<MetaEvent> trackMeta{{0, 0x05, {0x82, 0xA0}}, {0, 0x05, {0x82, 0xA2}}, {960, 0x01, {}}};
    SequenceContents contents;
    contents.metaEvents = songMeta;
    MidiTrack track;
    for (const auto& event : trackMeta)
        track.addMetaEvent(event);
    contents.tracks.push_back(track);
    MidiSequence seq;
    seq.replaceContents(std::move(contents));

    const auto loaded = MidiFileTest::saveAndLoad(seq);

    REQUIRE(loaded);
    CHECK(loaded->metaEvents == songMeta);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getMetaEvents() == trackMeta);
}

TEST_CASE("the conductor track ends after its last meta event", "[io][midifile][meta]")
{
    SequenceContents contents;
    contents.metaEvents = {{0, 0x06, {0x49}}, {1920, 0x06, {0x56}}};
    MidiSequence seq;
    seq.replaceContents(std::move(contents));
    juce::TemporaryFile temp(".mid");

    REQUIRE(MidiFileIO::save(seq, temp.getFile()));
    juce::FileInputStream stream(temp.getFile());
    REQUIRE(stream.openedOk());
    juce::MidiFile midiFile;
    REQUIRE(midiFile.readFrom(stream));
    REQUIRE(midiFile.getNumTracks() >= 1);

    const auto* events = midiFile.getTrack(0);
    REQUIRE(events->getNumEvents() > 0);
    const auto& last = events->getEventPointer(events->getNumEvents() - 1)->message;
    CHECK(last.isEndOfTrackMetaEvent());
    CHECK(last.getTimeStamp() == 1920);
}

TEST_CASE("a long meta event survives the round trip", "[io][midifile][meta]")
{
    std::vector<std::uint8_t> data;
    data.reserve(300);
    for (int i = 0; i < 300; ++i)
        data.push_back(static_cast<std::uint8_t>(i % 256));

    MidiTrack track;
    track.addMetaEvent({240, 0x01, data});
    MidiSequence seq;
    seq.addTrack(track);

    const auto loaded = MidiFileTest::saveAndLoad(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getMetaEvents() == std::vector<MetaEvent>{{240, 0x01, data}});
}

TEST_CASE("at the same tick, meta events are written before sysex, channel events and notes", "[io][midifile][meta]")
{
    MidiSequence seq;
    MidiTrack track;
    track.setName("Piano");
    track.addEvent({MidiEvent::Type::ControlChange, 0, 7, 100});
    track.addNote({60, 100, 0, 480});
    track.addSysEx({0, gsReset});
    track.addMetaEvent({0, 0x05, {'a'}});
    seq.addTrack(track);
    juce::TemporaryFile temp(".mid");

    REQUIRE(MidiFileIO::save(seq, temp.getFile()));
    juce::FileInputStream stream(temp.getFile());
    REQUIRE(stream.openedOk());
    juce::MidiFile midiFile;
    REQUIRE(midiFile.readFrom(stream));
    REQUIRE(midiFile.getNumTracks() == 2);

    const auto* events = midiFile.getTrack(1);
    std::vector<const juce::MidiMessage*> afterTrackHeader;
    for (int i = 0; i < events->getNumEvents(); ++i)
    {
        const auto& message = events->getEventPointer(i)->message;
        if (!message.isTrackNameEvent() && !message.isMidiChannelMetaEvent())
            afterTrackHeader.push_back(&message);
    }

    REQUIRE(afterTrackHeader.size() >= 2);
    CHECK(afterTrackHeader[0]->getMetaEventType() == 0x05);
    CHECK(afterTrackHeader[1]->isSysEx());
}

TEST_CASE("the song's meta events come before the tempo at the same tick", "[io][midifile][meta]")
{
    SequenceContents contents;
    contents.metaEvents.push_back({0, 0x06, {0x49}});
    contents.tracks.emplace_back();
    MidiSequence seq;
    seq.replaceContents(std::move(contents));
    juce::TemporaryFile temp(".mid");

    REQUIRE(MidiFileIO::save(seq, temp.getFile()));
    juce::FileInputStream stream(temp.getFile());
    REQUIRE(stream.openedOk());
    juce::MidiFile midiFile;
    REQUIRE(midiFile.readFrom(stream));
    REQUIRE(midiFile.getNumTracks() == 2);

    const auto* events = midiFile.getTrack(0);
    REQUIRE(events->getNumEvents() > 0);
    CHECK(events->getEventPointer(0)->message.getMetaEventType() == 0x06);
}

TEST_CASE("the end of track comes after the last meta event", "[io][midifile][meta]")
{
    MidiSequence seq;
    MidiTrack track;
    track.addNote({60, 100, 0, 480});
    track.addMetaEvent({1920, 0x05, {'a'}});
    seq.addTrack(track);
    juce::TemporaryFile temp(".mid");

    REQUIRE(MidiFileIO::save(seq, temp.getFile()));
    juce::FileInputStream stream(temp.getFile());
    REQUIRE(stream.openedOk());
    juce::MidiFile midiFile;
    REQUIRE(midiFile.readFrom(stream));
    REQUIRE(midiFile.getNumTracks() == 2);

    const auto* events = midiFile.getTrack(1);
    const juce::MidiMessage* endOfTrack = nullptr;
    for (int i = 0; i < events->getNumEvents(); ++i)
    {
        const auto& message = events->getEventPointer(i)->message;
        if (message.isEndOfTrackMetaEvent())
        {
            endOfTrack = &message;
            break;
        }
    }

    REQUIRE(endOfTrack != nullptr);
    CHECK(endOfTrack->getTimeStamp() == 1920);
}

TEST_CASE("saving twice does not duplicate meta events", "[io][midifile][meta]")
{
    juce::MidiMessageSequence conductor;
    conductor.addEvent(MidiFileTest::at(juce::MidiMessage::textMetaEvent(0x03, "Song"), 0));
    conductor.addEvent(MidiFileTest::at(juce::MidiMessage::tempoMetaEvent(500000), 0));
    juce::MidiMessageSequence events;
    events.addEvent(MidiFileTest::at(juce::MidiMessage::textMetaEvent(0x03, "Piano"), 0));
    events.addEvent(MidiFileTest::at(juce::MidiMessage::textMetaEvent(0x05, "la"), 0));
    events.addEvent(MidiFileTest::at(juce::MidiMessage::noteOn(1, 60, juce::uint8{100}), 0));
    events.addEvent(MidiFileTest::at(juce::MidiMessage::noteOff(1, 60), 480));
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480);
    midiFile.addTrack(conductor);
    midiFile.addTrack(events);

    const auto loaded = MidiFileTest::loadBytes(MidiFileTest::toBytes(midiFile, 1));
    REQUIRE(loaded);
    MidiSequence reloaded;
    reloaded.replaceContents(*loaded);
    const auto again = MidiFileTest::saveAndLoad(reloaded);

    REQUIRE(again);
    CHECK(again->metaEvents == std::vector<MetaEvent>{{0, 0x03, {'S', 'o', 'n', 'g'}}});
    REQUIRE(again->tracks.size() == 1);
    CHECK(again->tracks[0].getMetaEvents() == std::vector<MetaEvent>{{0, 0x05, {'l', 'a'}}});
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

    const auto loaded = MidiFileTest::saveAndLoad(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 2);
    CHECK(loaded->tracks[0].getName() == "Piano");
    CHECK(loaded->tracks[0].getChannel() == 1);
    CHECK(loaded->tracks[0].getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}});
    CHECK(loaded->tracks[1].getName() == "ドラム");
    CHECK(loaded->tracks[1].getChannel() == 10);
    CHECK(loaded->tracks[1].getNotes() == std::vector<MidiNote>{{36, 100, 0, 120}});
}

TEST_CASE("tracks without notes or events keep their order, names and channels", "[io][midifile]")
{
    MidiSequence seq;
    MidiTrack named;
    named.setName("Strings");
    named.setChannel(5);
    seq.addTrack(named);
    MidiTrack piano;
    piano.setName("Piano");
    piano.setChannel(2);
    piano.addNote({60, 100, 0, 480});
    seq.addTrack(piano);
    MidiTrack unnamed;
    unnamed.setChannel(16);
    seq.addTrack(unnamed);

    const auto loaded = MidiFileTest::saveAndLoad(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 3);
    CHECK(loaded->tracks[0].getName() == "Strings");
    CHECK(loaded->tracks[0].getChannel() == 5);
    CHECK(loaded->tracks[0].getNumNotes() == 0);
    CHECK(loaded->tracks[1].getName() == "Piano");
    CHECK(loaded->tracks[1].getChannel() == 2);
    CHECK(loaded->tracks[1].getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}});
    CHECK(loaded->tracks[2].getName().empty());
    CHECK(loaded->tracks[2].getChannel() == 16);
    CHECK(loaded->tracks[2].getNumNotes() == 0);
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

    const auto loaded = MidiFileTest::saveAndLoad(seq);

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

    const auto loaded = MidiFileTest::saveAndLoad(seq);

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

    const auto loaded = MidiFileTest::saveAndLoad(seq);

    REQUIRE(loaded);
    CHECK(loaded->timeline.getTimeSignatureChanges() ==
          std::vector<TimeSignatureChange>{{0, 3, 4}, {1440, 6, 8}, {2880, 4, 4}});
}

TEST_CASE("key signature changes are kept", "[io][midifile]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.setKeySignatureChanges({{0, -3, true}, {1920, 2, false}, {3840, 0, false}});

    const auto loaded = MidiFileTest::saveAndLoad(seq);

    REQUIRE(loaded);
    CHECK(loaded->keySignatureChanges ==
          std::vector<KeySignatureChange>{{0, -3, true}, {1920, 2, false}, {3840, 0, false}});
}

TEST_CASE("chord changes keep their root, type and bass", "[io][midifile]")
{
    using namespace ChordTest;
    auto slash = chord(1920, "G");
    slash.bassRoot = chord(0, "B").chordRoot;
    slash.bassType = 0;
    const std::vector<ChordChange> chordChanges{chord(0, "C"), slash, chord(3840, "N.C."), xfNoChord(4800)};

    MidiSequence seq;
    seq.addTrack();
    seq.setChordChanges(chordChanges);

    const auto loaded = MidiFileTest::saveAndLoad(seq);

    REQUIRE(loaded);
    CHECK(loaded->chordChanges == chordChanges);
}

TEST_CASE("a sequence without tracks loads as one empty track", "[io][midifile]")
{
    MidiSequence seq;

    const auto loaded = MidiFileTest::saveAndLoad(seq);

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

TEST_CASE("ramps are written as the filled-in values", "[io][midifile]")
{
    MidiSequence seq;
    MidiTrack track;
    track.addEvent({MidiEvent::Type::ControlChange, 0, 1, 0});
    track.addEvent({MidiEvent::Type::ControlChange, 4, 1, 4, true});
    seq.addTrack(track);

    const auto loaded = MidiFileTest::saveAndLoad(seq);

    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getEvents() == std::vector<MidiEvent>{{MidiEvent::Type::ControlChange, 0, 1, 0, true},
                                                                  {MidiEvent::Type::ControlChange, 1, 1, 1, true},
                                                                  {MidiEvent::Type::ControlChange, 2, 1, 2, true},
                                                                  {MidiEvent::Type::ControlChange, 3, 1, 3, true},
                                                                  {MidiEvent::Type::ControlChange, 4, 1, 4, true}});
}

TEST_CASE("steps are loaded as points and saved back as they were", "[io][midifile]")
{
    MidiSequence seq;
    MidiTrack track;
    track.addEvent({MidiEvent::Type::ControlChange, 0, 11, 50});
    track.addEvent({MidiEvent::Type::ControlChange, 1920, 11, 100});
    seq.addTrack(track);

    const auto loaded = MidiFileTest::saveAndLoad(seq);
    REQUIRE(loaded);
    REQUIRE(loaded->tracks.size() == 1);
    CHECK(loaded->tracks[0].getEvents() ==
          std::vector<MidiEvent>{{MidiEvent::Type::ControlChange, 0, 11, 50, true},
                                 {MidiEvent::Type::ControlChange, 1920, 11, 50, true},
                                 {MidiEvent::Type::ControlChange, 1920, 11, 100, true}});

    MidiSequence reloaded;
    reloaded.replaceContents(*loaded);
    const auto again = MidiFileTest::saveAndLoad(reloaded);
    REQUIRE(again);
    CHECK(again->tracks[0].getEvents() == loaded->tracks[0].getEvents());
}
