#include "model/SequenceContentsRules.h"
#include "support/BarBeatTickTestHelpers.h"
#include "support/MetaEventStringMaker.h"
#include "support/SysExEventStringMaker.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cmath>
#include <limits>
#include <vector>

namespace
{
constexpr int intMax = std::numeric_limits<int>::max();

SequenceContents validContents()
{
    SequenceContents contents;
    MidiTrack track;
    track.addNote({60, 100, 0, 480});
    track.addEvent({MidiEvent::Type::ControlChange, 0, 7, 100});
    contents.tracks.push_back(track);
    contents.keySignatureChanges = {{0, 0, false}};
    contents.chordChanges = {{0, 1, 0, ChordChange::none, ChordChange::none}};
    return contents;
}

SequenceContents withNote(const MidiNote& note)
{
    auto contents = validContents();
    contents.tracks[0].addNote(note);
    return contents;
}

SequenceContents withEvent(const MidiEvent& event)
{
    auto contents = validContents();
    contents.tracks[0].addEvent(event);
    return contents;
}

SequenceContents withSysEx(const SysExEvent& sysEx)
{
    auto contents = validContents();
    contents.tracks[0].addSysEx(sysEx);
    return contents;
}

SequenceContents withTrackMeta(const MetaEvent& event)
{
    auto contents = validContents();
    contents.tracks[0].addMetaEvent(event);
    return contents;
}

SequenceContents withSongMeta(const MetaEvent& event)
{
    auto contents = validContents();
    contents.metaEvents.push_back(event);
    return contents;
}

SequenceContents withTempos(std::vector<TempoChange> tempos)
{
    auto contents = validContents();
    contents.timeline.setTempoChanges(std::move(tempos));
    return contents;
}

SequenceContents withTimeSignatures(std::vector<TimeSignatureChange> signatures)
{
    auto contents = validContents();
    contents.timeline.setTimeSignatureChanges(std::move(signatures));
    return contents;
}

SequenceContents withKeySignatures(std::vector<KeySignatureChange> changes)
{
    auto contents = validContents();
    contents.keySignatureChanges = std::move(changes);
    return contents;
}

SequenceContents withChords(std::vector<ChordChange> changes)
{
    auto contents = validContents();
    contents.chordChanges = std::move(changes);
    return contents;
}
} // namespace

TEST_CASE("contents with default timeline and ordinary values are accepted", "[model][contentsrules]")
{
    CHECK(SequenceContentsRules::accepts(validContents()));
    CHECK(SequenceContentsRules::accepts(SequenceContents{}));
}

TEST_CASE("ppq must be positive", "[model][contentsrules]")
{
    const int ppq = GENERATE(0, -480);
    auto contents = validContents();
    contents.timeline.setTicksPerQuarterNote(ppq);
    CHECK_FALSE(SequenceContentsRules::accepts(contents));
}

TEST_CASE("tempos must start at tick 0", "[model][contentsrules]")
{
    CHECK_FALSE(SequenceContentsRules::accepts(withTempos({})));
    CHECK_FALSE(SequenceContentsRules::accepts(withTempos({{10, 120.0}})));
}

TEST_CASE("tempo ticks must strictly increase", "[model][contentsrules]")
{
    CHECK_FALSE(SequenceContentsRules::accepts(withTempos({{0, 120.0}, {0, 90.0}})));
    CHECK_FALSE(SequenceContentsRules::accepts(withTempos({{0, 120.0}, {960, 90.0}, {480, 100.0}})));
    CHECK(SequenceContentsRules::accepts(withTempos({{0, 120.0}, {480, 100.0}, {960, 90.0}})));
}

TEST_CASE("tempos must be finite and positive", "[model][contentsrules]")
{
    const double bpm = GENERATE(0.0, -1.0, std::numeric_limits<double>::infinity(), std::nan(""));
    CAPTURE(bpm);
    CHECK_FALSE(SequenceContentsRules::accepts(withTempos({{0, bpm}})));
}

TEST_CASE("time signatures must start at tick 0", "[model][contentsrules]")
{
    CHECK_FALSE(SequenceContentsRules::accepts(withTimeSignatures({})));
    CHECK_FALSE(SequenceContentsRules::accepts(withTimeSignatures({{480, 4, 4}})));
}

TEST_CASE("time signature ticks must not decrease", "[model][contentsrules]")
{
    CHECK_FALSE(SequenceContentsRules::accepts(withTimeSignatures({{0, 4, 4}, {960, 3, 4}, {480, 2, 4}})));
}

TEST_CASE("time signatures may share a tick", "[model][contentsrules]")
{
    CHECK(SequenceContentsRules::accepts(withTimeSignatures({{0, 4, 4}, {0, 3, 4}})));
}

TEST_CASE("time signature numerator must be at least 1", "[model][contentsrules]")
{
    CHECK_FALSE(SequenceContentsRules::accepts(withTimeSignatures({{0, 0, 4}})));
}

TEST_CASE("time signature denominator must be a power of two", "[model][contentsrules]")
{
    const int denominator = GENERATE(0, -4, 3, 6, 12);
    CAPTURE(denominator);
    CHECK_FALSE(SequenceContentsRules::accepts(withTimeSignatures({{0, 4, denominator}})));
}

TEST_CASE("time signature denominator may be a power of two down to one tick per beat", "[model][contentsrules]")
{
    const int denominator = GENERATE(1, 2, 64, 128, 1024);
    CAPTURE(denominator);
    CHECK(SequenceContentsRules::accepts(withTimeSignatures({{0, 4, denominator}})));
}

TEST_CASE("a time signature whose beat is shorter than one tick is rejected", "[model][contentsrules]")
{
    const int denominator = GENERATE(2048, 1 << 30);
    CAPTURE(denominator);
    CHECK_FALSE(SequenceContentsRules::accepts(withTimeSignatures({{0, 4, denominator}})));
}

TEST_CASE("a bar must fit in an int", "[model][contentsrules]")
{
    CHECK_FALSE(SequenceContentsRules::accepts(withTimeSignatures({{0, intMax, 4}})));
    CHECK(SequenceContentsRules::accepts(withTimeSignatures({{0, 1000000, 4}})));
}

TEST_CASE("four quarter notes of ticks must fit in an int", "[model][contentsrules]")
{
    auto contents = withTimeSignatures({{0, 1, 4}});
    contents.timeline.setTicksPerQuarterNote(intMax / 4);
    CHECK(SequenceContentsRules::accepts(contents));
    contents.timeline.setTicksPerQuarterNote(intMax / 4 + 1);
    CHECK_FALSE(SequenceContentsRules::accepts(contents));
}

TEST_CASE("accepted time signatures convert ticks to bars", "[model][contentsrules]")
{
    const auto contents = withTimeSignatures({{0, 4, 1024}});

    REQUIRE(SequenceContentsRules::accepts(contents));
    CHECK(contents.timeline.tickToBarBeatTick(9) == BarBeatTick{3, 2, 0});
}

TEST_CASE("key signatures must be in strictly increasing order from tick 0", "[model][contentsrules]")
{
    CHECK_FALSE(SequenceContentsRules::accepts(withKeySignatures({{-1, 0, false}})));
    CHECK_FALSE(SequenceContentsRules::accepts(withKeySignatures({{0, 0, false}, {0, 1, true}})));
    CHECK(SequenceContentsRules::accepts(withKeySignatures({})));
}

TEST_CASE("key signatures stay within seven accidentals", "[model][contentsrules]")
{
    CHECK_FALSE(SequenceContentsRules::accepts(withKeySignatures({{0, -8, false}})));
    CHECK_FALSE(SequenceContentsRules::accepts(withKeySignatures({{0, 8, false}})));
    CHECK(SequenceContentsRules::accepts(withKeySignatures({{0, -7, false}, {480, 7, true}})));
}

TEST_CASE("chords must be in strictly increasing order from tick 0", "[model][contentsrules]")
{
    CHECK_FALSE(SequenceContentsRules::accepts(withChords({{-1, 1, 0, 127, 127}})));
    CHECK_FALSE(SequenceContentsRules::accepts(withChords({{0, 1, 0, 127, 127}, {0, 2, 0, 127, 127}})));
}

TEST_CASE("chord fields are byte values", "[model][contentsrules]")
{
    CHECK_FALSE(SequenceContentsRules::accepts(withChords({{0, -1, 0, 127, 127}})));
    CHECK_FALSE(SequenceContentsRules::accepts(withChords({{0, 1, 256, 127, 127}})));
    CHECK_FALSE(SequenceContentsRules::accepts(withChords({{0, 1, 0, 256, 127}})));
    CHECK_FALSE(SequenceContentsRules::accepts(withChords({{0, 1, 0, 127, -1}})));
    CHECK(SequenceContentsRules::accepts(withChords({{0, 255, 255, 255, 255}})));
}

TEST_CASE("track channels are 1 to 16", "[model][contentsrules]")
{
    const int channel = GENERATE(0, 17);
    CAPTURE(channel);
    auto contents = validContents();
    contents.tracks[0].setChannel(channel);
    CHECK_FALSE(SequenceContentsRules::accepts(contents));
}

TEST_CASE("note numbers and velocities stay in MIDI range", "[model][contentsrules]")
{
    CHECK_FALSE(SequenceContentsRules::accepts(withNote({-1, 100, 0, 480})));
    CHECK_FALSE(SequenceContentsRules::accepts(withNote({128, 100, 0, 480})));
    CHECK_FALSE(SequenceContentsRules::accepts(withNote({60, -1, 0, 480})));
    CHECK_FALSE(SequenceContentsRules::accepts(withNote({60, 128, 0, 480})));
    CHECK(SequenceContentsRules::accepts(withNote({127, 0, 0, 480})));
}

TEST_CASE("notes may be zero length but must not start before 0 or run backwards", "[model][contentsrules]")
{
    CHECK(SequenceContentsRules::accepts(withNote({60, 100, 480, 0})));
    CHECK_FALSE(SequenceContentsRules::accepts(withNote({60, 100, 480, -1})));
    CHECK_FALSE(SequenceContentsRules::accepts(withNote({60, 100, -1, 480})));
}

TEST_CASE("a note must not end past the int range", "[model][contentsrules]")
{
    CHECK_FALSE(SequenceContentsRules::accepts(withNote({60, 100, intMax, 1})));
    CHECK(SequenceContentsRules::accepts(withNote({60, 100, intMax - 1, 1})));
}

TEST_CASE("event values stay in range for their type", "[model][contentsrules]")
{
    CHECK_FALSE(SequenceContentsRules::accepts(withEvent({MidiEvent::Type::ControlChange, 0, 128, 0})));
    CHECK_FALSE(SequenceContentsRules::accepts(withEvent({MidiEvent::Type::ControlChange, 0, 7, 128})));
    CHECK_FALSE(SequenceContentsRules::accepts(withEvent({MidiEvent::Type::ControlChange, -1, 7, 100})));
    CHECK_FALSE(SequenceContentsRules::accepts(withEvent({MidiEvent::Type::PitchBend, 0, 16384, 0})));
    CHECK(SequenceContentsRules::accepts(withEvent({MidiEvent::Type::PitchBend, 0, 16383, 0})));
}

TEST_CASE("a ramp on a control change or pitch bend is accepted", "[model][contentsrules]")
{
    CHECK(SequenceContentsRules::accepts(withEvent({MidiEvent::Type::ControlChange, 0, 1, 64, true})));
    CHECK(SequenceContentsRules::accepts(withEvent({MidiEvent::Type::PitchBend, 0, 8192, 0, true})));
}

TEST_CASE("a ramp on another event type is rejected", "[model][contentsrules]")
{
    CHECK_FALSE(SequenceContentsRules::accepts(withEvent({MidiEvent::Type::ProgramChange, 0, 5, 0, true})));
    CHECK_FALSE(SequenceContentsRules::accepts(withEvent({MidiEvent::Type::ChannelPressure, 0, 5, 0, true})));
    CHECK_FALSE(SequenceContentsRules::accepts(withEvent({MidiEvent::Type::KeyPressure, 0, 60, 5, true})));
}

TEST_CASE("sysex in range is accepted", "[model][contentsrules][sysex]")
{
    CHECK(SequenceContentsRules::accepts(withSysEx({0, {0x41, 0x00, 0x7F}})));
}

TEST_CASE("sysex outside its ranges is rejected", "[model][contentsrules][sysex]")
{
    const auto sysEx =
        GENERATE(SysExEvent{-1, {0x41}}, SysExEvent{0, {}}, SysExEvent{0, {0x41, 0x80}}, SysExEvent{0, {0xF7}});
    CAPTURE(sysEx);
    CHECK_FALSE(SequenceContentsRules::accepts(withSysEx(sysEx)));
}

TEST_CASE("sysex may share a tick but not go back in time", "[model][contentsrules][sysex]")
{
    auto sameTick = withSysEx({480, {0x41}});
    sameTick.tracks[0].addSysEx({480, {0x42}});
    CHECK(SequenceContentsRules::accepts(sameTick));

    auto backwards = withSysEx({480, {0x41}});
    backwards.tracks[0].addSysEx({0, {0x42}});
    CHECK_FALSE(SequenceContentsRules::accepts(backwards));
}

TEST_CASE("meta events in range are accepted", "[model][contentsrules][meta]")
{
    CHECK(SequenceContentsRules::accepts(withTrackMeta({0, 0x05, {0x82, 0xA0}})));
    CHECK(SequenceContentsRules::accepts(withTrackMeta({0, 0x00, {}})));
    CHECK(SequenceContentsRules::accepts(withTrackMeta({0, 0x7F, {0x43, 0x7B, 0x00, 0x58, 0x46, 0x30, 0x32}})));
    CHECK(SequenceContentsRules::accepts(withSongMeta({0, 0x03, {0x53}})));
    CHECK(SequenceContentsRules::accepts(withSongMeta({480, 0x06, {}})));
}

TEST_CASE("meta events the app handles itself are rejected", "[model][contentsrules][meta]")
{
    const auto event =
        GENERATE(MetaEvent{-1, 0x05, {}}, MetaEvent{0, 0x80, {}}, MetaEvent{0, 0x2F, {}}, MetaEvent{0, 0x20, {0x00}},
                 MetaEvent{0, 0x51, {0x07, 0xA1, 0x20}}, MetaEvent{0, 0x58, {0x04, 0x02, 0x18, 0x08}},
                 MetaEvent{0, 0x59, {0x00, 0x00}}, MetaEvent{0, 0x7F, {0x43, 0x7B, 0x01, 0x31, 0x00, 0x7F, 0x7F}},
                 MetaEvent{0, 0x7F, {0x43, 0x7B, 0x01}});
    CAPTURE(event);
    CHECK_FALSE(SequenceContentsRules::accepts(withTrackMeta(event)));
    CHECK_FALSE(SequenceContentsRules::accepts(withSongMeta(event)));
}

TEST_CASE("a track name meta event belongs to the song, not a track", "[model][contentsrules][meta]")
{
    CHECK_FALSE(SequenceContentsRules::accepts(withTrackMeta({0, 0x03, {0x53}})));
}

TEST_CASE("meta events may share a tick but not go back in time", "[model][contentsrules][meta]")
{
    auto sameTick = withTrackMeta({480, 0x05, {0x61}});
    sameTick.tracks[0].addMetaEvent({480, 0x05, {0x62}});
    CHECK(SequenceContentsRules::accepts(sameTick));

    auto backwards = withTrackMeta({480, 0x05, {0x61}});
    backwards.tracks[0].addMetaEvent({0, 0x05, {0x62}});
    CHECK_FALSE(SequenceContentsRules::accepts(backwards));

    auto songBackwards = withSongMeta({480, 0x06, {}});
    songBackwards.metaEvents.push_back({0, 0x06, {}});
    CHECK_FALSE(SequenceContentsRules::accepts(songBackwards));
}
