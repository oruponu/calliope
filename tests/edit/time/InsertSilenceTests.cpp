#include "edit/time/InsertSilence.h"
#include "model/SequenceContentsRules.h"
#include "support/ChordStringMaker.h"
#include "support/KeySignatureStringMaker.h"
#include "support/MetaEventStringMaker.h"
#include "support/MidiEventTestHelpers.h"
#include "support/MidiNoteTestHelpers.h"
#include "support/SysExEventStringMaker.h"
#include "support/TempoStringMaker.h"
#include "support/TimeSignatureStringMaker.h"
#include "support/TimedContentsTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <limits>

namespace
{
constexpr int intMax = std::numeric_limits<int>::max();

TimedContents song()
{
    TimedContents contents;
    contents.ticksPerQuarterNote = 480;
    contents.tempoChanges = {{0, 120.0}};
    contents.timeSignatureChanges = {{0, 4, 4}};
    contents.tracks.push_back({});
    return contents;
}

MidiEvent cc(int tick, int number, int value, bool ramp = true)
{
    return {MidiEvent::Type::ControlChange, tick, number, value, ramp};
}

MidiEvent pb(int tick, int value)
{
    return {MidiEvent::Type::PitchBend, tick, value, 0, true};
}

MidiEvent pc(int tick, int program)
{
    return {MidiEvent::Type::ProgramChange, tick, program, 0, false};
}
} // namespace

TEST_CASE("items at and after the start move back by the inserted length", "[time][insert]")
{
    auto before = song();
    before.tempoChanges = {{0, 120.0}, {1920, 100.0}};
    before.timeSignatureChanges = {{0, 4, 4}, {1920, 3, 4}};
    before.keySignatureChanges = {{1920, 2, false}};
    before.chordChanges = {ChordChange::noChord(1920)};
    before.metaEvents = {{1919, 0x01, {'a'}}, {1920, 0x01, {'b'}}};
    auto& track = before.tracks[0];
    track.notes = {{60, 1, 1919, 1}, {62, 100, 1920, 100}};
    track.events = {pc(1919, 1), pc(1920, 2)};
    track.sysExEvents = {{1919, {0x01}}, {1920, {0x02}}};
    track.metaEvents = {{1919, 0x06, {'x'}}, {1920, 0x06, {'y'}}};

    const auto after = insertSilence(before, 1920, 3840);

    REQUIRE(after);
    CHECK(after->tempoChanges == std::vector<TempoChange>{{0, 120.0}, {3840, 100.0}});
    CHECK(after->timeSignatureChanges == std::vector<TimeSignatureChange>{{0, 4, 4}, {3840, 3, 4}});
    CHECK(after->keySignatureChanges == std::vector<KeySignatureChange>{{3840, 2, false}});
    CHECK(after->chordChanges == std::vector<ChordChange>{ChordChange::noChord(3840)});
    CHECK(after->metaEvents == std::vector<MetaEvent>{{1919, 0x01, {'a'}}, {3840, 0x01, {'b'}}});
    const auto& moved = after->tracks[0];
    CHECK(moved.notes == std::vector<MidiNote>{{60, 1, 1919, 1}, {62, 100, 3840, 100}});
    CHECK(moved.events == std::vector<MidiEvent>{pc(1919, 1), pc(3840, 2)});
    CHECK(moved.sysExEvents == std::vector<SysExEvent>{{1919, {0x01}}, {3840, {0x02}}});
    CHECK(moved.metaEvents == std::vector<MetaEvent>{{1919, 0x06, {'x'}}, {3840, 0x06, {'y'}}});
    CHECK(SequenceContentsRules::accepts(TimedContentsTest::toSequenceContents(*after)));
}

TEST_CASE("a note across the start ends at the start", "[time][insert]")
{
    auto before = song();
    before.tracks[0].notes = {{60, 100, 960, 1920}, {62, 100, 960, 960}};

    const auto after = insertSilence(before, 1920, 3840);

    REQUIRE(after);
    CHECK(after->tracks[0].notes == std::vector<MidiNote>{{60, 100, 960, 960}, {62, 100, 960, 960}});
}

TEST_CASE("the inserted span keeps the sustain pedal down", "[time][insert]")
{
    auto before = song();
    before.tracks[0].events = {cc(0, 64, 127, false), cc(2400, 64, 0, false)};

    const auto after = insertSilence(before, 1920, 3840);

    REQUIRE(after);
    CHECK(after->tracks[0].events == std::vector<MidiEvent>{cc(0, 64, 127, false), cc(4320, 64, 0, false)});
}

TEST_CASE("a line across the start is held flat through the inserted span", "[time][insert]")
{
    auto before = song();
    before.tracks[0].events = {cc(0, 7, 0), cc(3840, 7, 100), pb(0, 0), pb(3840, 16000)};

    const auto after = insertSilence(before, 1920, 3840);

    REQUIRE(after);
    std::vector<MidiEvent> volume;
    std::vector<MidiEvent> bend;
    for (const auto& e : after->tracks[0].events)
        (e.type == MidiEvent::Type::ControlChange ? volume : bend).push_back(e);
    CHECK(volume == std::vector<MidiEvent>{cc(0, 7, 0), cc(1920, 7, 50), cc(3840, 7, 50), cc(5760, 7, 100)});
    CHECK(bend == std::vector<MidiEvent>{pb(0, 0), pb(1920, 8000), pb(3840, 8000), pb(5760, 16000)});
}

TEST_CASE("a line ending at the start is not stretched", "[time][insert]")
{
    auto before = song();
    before.tracks[0].events = {cc(0, 7, 0), cc(1920, 7, 100)};

    const auto after = insertSilence(before, 1920, 3840);

    REQUIRE(after);
    CHECK(after->tracks[0].events == std::vector<MidiEvent>{cc(0, 7, 0), cc(1920, 7, 100), cc(3840, 7, 100)});
}

TEST_CASE("a line starting at the start begins after the inserted span", "[time][insert]")
{
    auto before = song();
    before.tracks[0].events = {cc(0, 7, 30), cc(1920, 7, 30), cc(3840, 7, 100)};

    const auto after = insertSilence(before, 1920, 3840);

    REQUIRE(after);
    CHECK(after->tracks[0].events == std::vector<MidiEvent>{cc(0, 7, 30), cc(3840, 7, 30), cc(5760, 7, 100)});
}

TEST_CASE("a flat line across the start gets no extra points", "[time][insert]")
{
    auto before = song();
    before.tracks[0].events = {cc(0, 7, 40), cc(3840, 7, 40)};

    const auto after = insertSilence(before, 1920, 3840);

    REQUIRE(after);
    CHECK(after->tracks[0].events == std::vector<MidiEvent>{cc(0, 7, 40), cc(5760, 7, 40)});
}

TEST_CASE("inserting at the song start keeps the settings of tick 0 there", "[time][insert]")
{
    auto before = song();
    before.keySignatureChanges = {{0, 2, false}};
    before.chordChanges = {ChordChange::noChord(0)};
    before.metaEvents = {{0, 0x02, {'c'}}, {0, 0x05, {'l'}}};
    auto& track = before.tracks[0];
    track.notes = {{60, 100, 0, 480}};
    track.events = {pc(0, 5)};
    track.sysExEvents = {{0, {0x01}}};
    track.metaEvents = {{0, 0x04, {'n'}}, {0, 0x06, {'m'}}};

    const auto after = insertSilence(before, 0, 1920);

    REQUIRE(after);
    CHECK(after->tempoChanges == std::vector<TempoChange>{{0, 120.0}, {1920, 120.0}});
    CHECK(after->timeSignatureChanges == std::vector<TimeSignatureChange>{{0, 4, 4}, {1920, 4, 4}});
    CHECK(after->keySignatureChanges == std::vector<KeySignatureChange>{{1920, 2, false}});
    CHECK(after->chordChanges == std::vector<ChordChange>{ChordChange::noChord(1920)});
    CHECK(after->metaEvents == std::vector<MetaEvent>{{0, 0x02, {'c'}}, {1920, 0x05, {'l'}}});
    const auto& moved = after->tracks[0];
    CHECK(moved.notes == std::vector<MidiNote>{{60, 100, 1920, 480}});
    CHECK(moved.events == std::vector<MidiEvent>{pc(0, 5)});
    CHECK(moved.sysExEvents == std::vector<SysExEvent>{{0, {0x01}}});
    CHECK(moved.metaEvents == std::vector<MetaEvent>{{0, 0x04, {'n'}}, {1920, 0x06, {'m'}}});
    CHECK(SequenceContentsRules::accepts(TimedContentsTest::toSequenceContents(*after)));
}

TEST_CASE("inserting at the song start keeps meta events in tick order", "[time][insert]")
{
    auto before = song();
    before.metaEvents = {{0, 0x05, {'l'}}, {0, 0x02, {'c'}}};
    before.tracks[0].metaEvents = {{0, 0x06, {'m'}}, {0, 0x04, {'i'}}};

    const auto after = insertSilence(before, 0, 1920);

    REQUIRE(after);
    CHECK(after->metaEvents == std::vector<MetaEvent>{{0, 0x02, {'c'}}, {1920, 0x05, {'l'}}});
    CHECK(after->tracks[0].metaEvents == std::vector<MetaEvent>{{0, 0x04, {'i'}}, {1920, 0x06, {'m'}}});
    CHECK(SequenceContentsRules::accepts(TimedContentsTest::toSequenceContents(*after)));
}

TEST_CASE("inserting at the song start restarts a line from tick 0 after the inserted span", "[time][insert]")
{
    auto before = song();
    before.tracks[0].events = {cc(0, 7, 0), cc(1920, 7, 100)};

    const auto after = insertSilence(before, 0, 1920);

    REQUIRE(after);
    CHECK(after->tracks[0].events == std::vector<MidiEvent>{cc(0, 7, 0), cc(1920, 7, 0), cc(3840, 7, 100)});
}

TEST_CASE("a whole-bar insert inside one time signature is accepted", "[time][insert]")
{
    auto before = song();
    before.timeSignatureChanges = {{0, 4, 4}, {3840, 3, 4}};

    const auto after = insertSilence(before, 1920, 3840);

    REQUIRE(after);
    CHECK(after->timeSignatureChanges == std::vector<TimeSignatureChange>{{0, 4, 4}, {5760, 3, 4}});
}

TEST_CASE("an insert across a time signature change is refused", "[time][insert]")
{
    auto before = song();
    before.timeSignatureChanges = {{0, 4, 4}, {3840, 3, 4}};

    const auto after = insertSilence(before, 1920, 5280);

    REQUIRE_FALSE(after);
    CHECK(after.error() == TimeEditError::TimeSignatureOffBar);
}

TEST_CASE("a mid-bar insert is accepted without a later time signature", "[time][insert]")
{
    CHECK(insertSilence(song(), 480, 960));
}

TEST_CASE("a mid-bar insert before a time signature change is refused", "[time][insert]")
{
    auto before = song();
    before.timeSignatureChanges = {{0, 4, 4}, {1920, 3, 4}};

    const auto after = insertSilence(before, 480, 960);

    REQUIRE_FALSE(after);
    CHECK(after.error() == TimeEditError::TimeSignatureOffBar);
}

TEST_CASE("an empty range is refused", "[time][insert]")
{
    const auto empty = insertSilence(song(), 1920, 1920);
    const auto reversed = insertSilence(song(), 1920, 960);

    REQUIRE_FALSE(empty);
    REQUIRE_FALSE(reversed);
    CHECK(empty.error() == TimeEditError::EmptyRange);
    CHECK(reversed.error() == TimeEditError::EmptyRange);
}

TEST_CASE("a shifted note may end exactly at the last tick", "[time][insert]")
{
    auto before = song();
    before.tracks[0].notes = {{60, 100, 1920, intMax - 3840}};

    const auto after = insertSilence(before, 1920, 3840);

    REQUIRE(after);
    CHECK(after->tracks[0].notes == std::vector<MidiNote>{{60, 100, 3840, intMax - 3840}});
}

TEST_CASE("a shifted note past the last tick is refused", "[time][insert]")
{
    auto before = song();
    before.tracks[0].notes = {{60, 100, 1920, intMax - 3840 + 1}};

    const auto after = insertSilence(before, 1920, 3840);

    REQUIRE_FALSE(after);
    CHECK(after.error() == TimeEditError::TickOverflow);
}
