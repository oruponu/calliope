#include "edit/time/DeleteTime.h"
#include "model/SequenceContentsRules.h"
#include "model/controller/ControllerPointConversion.h"
#include "support/MetaEventStringMaker.h"
#include "support/MidiEventTestHelpers.h"
#include "support/MidiNoteTestHelpers.h"
#include "support/SysExEventStringMaker.h"
#include "support/TempoStringMaker.h"
#include "support/TimeSignatureStringMaker.h"
#include "support/TimedContentsTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <utility>
#include <vector>

namespace
{
TimedContents song()
{
    TimedContents contents;
    contents.ticksPerQuarterNote = 480;
    contents.tempoChanges = {{0, 120.0}};
    contents.timeSignatureChanges = {{0, 4, 4}};
    contents.tracks.push_back({});
    return contents;
}

TimedContents songWithEvents(std::vector<MidiEvent> events)
{
    auto contents = song();
    contents.tracks[0].events = std::move(events);
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

MidiEvent kp(int tick, int note, int value)
{
    return {MidiEvent::Type::KeyPressure, tick, note, value, false};
}

std::vector<MidiEvent> eventsAfterDelete(const std::vector<MidiEvent>& events)
{
    const auto after = deleteTime(songWithEvents(events), 1920, 5760);
    REQUIRE(after);
    const auto& result = after->tracks[0].events;
    CHECK(convertStepsToPoints(result) == result);
    return result;
}
} // namespace

TEST_CASE("items in the range are removed and later ones move forward", "[time][delete]")
{
    auto before = song();
    before.metaEvents = {{1919, 0x01, {'a'}}, {1920, 0x01, {'b'}}, {5759, 0x01, {'c'}}, {5760, 0x01, {'d'}}};
    auto& track = before.tracks[0];
    track.notes = {{60, 100, 1919, 1}, {61, 100, 1920, 1}, {62, 100, 5759, 1}, {63, 100, 5760, 1}};
    track.events = {kp(1919, 60, 1), kp(1920, 61, 2), kp(5759, 62, 3), kp(5760, 63, 4)};
    track.sysExEvents = {{1919, {0x01}}, {1920, {0x02}}, {5759, {0x03}}, {5760, {0x04}}};
    track.metaEvents = {{1919, 0x06, {'w'}}, {1920, 0x06, {'x'}}, {5759, 0x06, {'y'}}, {5760, 0x06, {'z'}}};

    const auto after = deleteTime(before, 1920, 5760);

    REQUIRE(after);
    CHECK(after->metaEvents == std::vector<MetaEvent>{{1919, 0x01, {'a'}}, {1920, 0x01, {'d'}}});
    const auto& moved = after->tracks[0];
    CHECK(moved.notes == std::vector<MidiNote>{{60, 100, 1919, 1}, {63, 100, 1920, 1}});
    CHECK(moved.events == std::vector<MidiEvent>{kp(1919, 60, 1), kp(1920, 63, 4)});
    CHECK(moved.sysExEvents == std::vector<SysExEvent>{{1919, {0x01}}, {1920, {0x04}}});
    CHECK(moved.metaEvents == std::vector<MetaEvent>{{1919, 0x06, {'w'}}, {1920, 0x06, {'z'}}});
    CHECK(SequenceContentsRules::accepts(TimedContentsTest::toSequenceContents(*after)));
}

TEST_CASE("a note across the start ends at the start", "[time][delete]")
{
    auto before = song();
    before.tracks[0].notes = {{60, 100, 960, 1920}, {62, 100, 960, 9600}, {64, 100, 5760, 100}};

    const auto after = deleteTime(before, 1920, 5760);

    REQUIRE(after);
    CHECK(after->tracks[0].notes ==
          std::vector<MidiNote>{{60, 100, 960, 960}, {62, 100, 960, 960}, {64, 100, 1920, 100}});
}

TEST_CASE("a controller changed in the range is carried to the start", "[time][delete]")
{
    const auto after = eventsAfterDelete({cc(0, 7, 100), cc(3000, 7, 100), cc(3000, 7, 50)});

    CHECK(after == std::vector<MidiEvent>{cc(0, 7, 100), cc(1920, 7, 100), cc(1920, 7, 50)});
}

TEST_CASE("program and bank select in the range are carried to the start", "[time][delete]")
{
    const auto after = eventsAfterDelete({pc(0, 3), cc(3000, 0, 1, false), pc(3000, 10)});

    CHECK(after == std::vector<MidiEvent>{pc(0, 3), cc(1920, 0, 1, false), pc(1920, 10)});
}

TEST_CASE("a changed program brings every value with it", "[time][delete]")
{
    const auto after = eventsAfterDelete({pc(0, 3), cc(0, 7, 100), pc(3000, 10)});

    CHECK(after == std::vector<MidiEvent>{pc(0, 3), cc(0, 7, 100), pc(1920, 10), cc(1920, 7, 100)});
}

TEST_CASE("values unchanged across the range are not repeated", "[time][delete]")
{
    const auto after = eventsAfterDelete({cc(0, 10, 64)});

    CHECK(after == std::vector<MidiEvent>{cc(0, 10, 64)});
}

TEST_CASE("carried events come before the events moved to the start", "[time][delete]")
{
    const auto after = eventsAfterDelete({pc(3000, 10), pc(5760, 20)});

    CHECK(after == std::vector<MidiEvent>{pc(1920, 10), pc(1920, 20)});
}

TEST_CASE("polyphonic key pressure is not carried", "[time][delete]")
{
    const auto after = eventsAfterDelete({kp(3000, 60, 50)});

    CHECK(after.empty());
}

TEST_CASE("a line across the start ends there and steps to its value at the end", "[time][delete]")
{
    const auto after = eventsAfterDelete({cc(0, 7, 0), cc(7680, 7, 100)});

    CHECK(after == std::vector<MidiEvent>{cc(0, 7, 0), cc(1920, 7, 25), cc(1920, 7, 75), cc(3840, 7, 100)});
}

TEST_CASE("a line ending exactly at the end of the range", "[time][delete]")
{
    const auto after = eventsAfterDelete({cc(0, 7, 0), cc(5760, 7, 100)});

    CHECK(after == std::vector<MidiEvent>{cc(0, 7, 0), cc(1920, 7, 33), cc(1920, 7, 100)});
}

TEST_CASE("a line across the end continues from its value at the end", "[time][delete]")
{
    const auto after = eventsAfterDelete({cc(3840, 7, 0), cc(7680, 7, 100)});

    CHECK(after == std::vector<MidiEvent>{cc(1920, 7, 50), cc(3840, 7, 100)});
}

TEST_CASE("a value held before the start stays held up to a line across the end", "[time][delete]")
{
    const auto after = eventsAfterDelete({cc(0, 7, 50), cc(2880, 7, 50), cc(3840, 7, 0), cc(7680, 7, 100)});

    CHECK(after == std::vector<MidiEvent>{cc(0, 7, 50), cc(1920, 7, 50), cc(3840, 7, 100)});
}

TEST_CASE("a value held before the start stays held up to a line ending exactly at the end", "[time][delete]")
{
    const auto after = eventsAfterDelete({cc(0, 7, 50), cc(2880, 7, 50), cc(5760, 7, 100)});

    CHECK(after == std::vector<MidiEvent>{cc(0, 7, 50), cc(1920, 7, 50), cc(1920, 7, 100)});
}

TEST_CASE("a line across the end after reset all controllers keeps the lane held up to the start", "[time][delete]")
{
    const auto after =
        eventsAfterDelete({pb(0, 100), cc(1000, 121, 0, false), pb(2880, 100), pb(3840, 4096), pb(7680, 12288)});

    CHECK(after ==
          std::vector<MidiEvent>{pb(0, 100), cc(1000, 121, 0, false), pb(1920, 100), pb(1920, 8192), pb(3840, 12288)});
}

TEST_CASE("a step exactly at the end of the range is not repeated", "[time][delete]")
{
    const auto after =
        eventsAfterDelete({cc(0, 7, 10), cc(3000, 7, 10), cc(3000, 7, 50), cc(5760, 7, 50), cc(5760, 7, 90)});

    CHECK(after == std::vector<MidiEvent>{cc(0, 7, 10), cc(1920, 7, 10), cc(1920, 7, 50), cc(1920, 7, 90)});
}

TEST_CASE("a pitch bend line across the start ends there and steps to its value at the end", "[time][delete]")
{
    const auto after = eventsAfterDelete({pb(0, 0), pb(7680, 16000)});

    CHECK(after == std::vector<MidiEvent>{pb(0, 0), pb(1920, 4000), pb(1920, 12000), pb(3840, 16000)});
}

TEST_CASE("deleting from the song start keeps tick 0 SysEx and attribute meta events", "[time][delete]")
{
    auto before = song();
    before.tempoChanges = {{0, 120.0}, {960, 90.0}};
    before.metaEvents = {{0, 0x03, {'n'}}, {0, 0x06, {'m'}}, {0, 0x02, {'c'}}};
    auto& track = before.tracks[0];
    track.events = {pc(0, 5)};
    track.sysExEvents = {{0, {0x01}}, {100, {0x02}}};
    track.metaEvents = {{0, 0x04, {'i'}}, {0, 0x05, {'l'}}};

    const auto after = deleteTime(before, 0, 1920);

    REQUIRE(after);
    CHECK(after->tempoChanges == std::vector<TempoChange>{{0, 90.0}});
    CHECK(after->metaEvents == std::vector<MetaEvent>{{0, 0x03, {'n'}}, {0, 0x02, {'c'}}});
    const auto& moved = after->tracks[0];
    CHECK(moved.events == std::vector<MidiEvent>{pc(0, 5)});
    CHECK(moved.sysExEvents == std::vector<SysExEvent>{{0, {0x01}}});
    CHECK(moved.metaEvents == std::vector<MetaEvent>{{0, 0x04, {'i'}}});
    CHECK(SequenceContentsRules::accepts(TimedContentsTest::toSequenceContents(*after)));
}

TEST_CASE("tempo in the range is carried to the start", "[time][delete]")
{
    auto inRange = song();
    inRange.tempoChanges = {{0, 120.0}, {3000, 90.0}};
    auto atEnd = song();
    atEnd.tempoChanges = {{0, 120.0}, {3000, 90.0}, {5760, 80.0}};

    const auto carried = deleteTime(inRange, 1920, 5760);
    const auto replaced = deleteTime(atEnd, 1920, 5760);

    REQUIRE(carried);
    REQUIRE(replaced);
    CHECK(carried->tempoChanges == std::vector<TempoChange>{{0, 120.0}, {1920, 90.0}});
    CHECK(replaced->tempoChanges == std::vector<TempoChange>{{0, 120.0}, {1920, 80.0}});
}

TEST_CASE("deleting whole bars across a time signature change keeps the bars after it", "[time][delete]")
{
    auto before = song();
    before.timeSignatureChanges = {{0, 4, 4}, {3840, 3, 4}};

    const auto after = deleteTime(before, 1920, 5280);

    REQUIRE(after);
    CHECK(after->timeSignatureChanges == std::vector<TimeSignatureChange>{{0, 4, 4}, {1920, 3, 4}});
    CHECK(SequenceContentsRules::accepts(TimedContentsTest::toSequenceContents(*after)));
}

TEST_CASE("a mid-bar delete before a time signature change is refused", "[time][delete]")
{
    auto before = song();
    before.timeSignatureChanges = {{0, 4, 4}, {3840, 3, 4}};

    const auto after = deleteTime(before, 480, 960);

    REQUIRE_FALSE(after);
    CHECK(after.error() == TimeEditError::TimeSignatureOffBar);
}

TEST_CASE("an empty range is refused", "[time][delete]")
{
    const auto after = deleteTime(song(), 1920, 1920);

    REQUIRE_FALSE(after);
    CHECK(after.error() == TimeEditError::EmptyRange);
}

TEST_CASE("a range past the end of the song changes nothing", "[time][delete]")
{
    auto before = song();
    before.tempoChanges = {{0, 120.0}, {960, 90.0}};
    before.tracks[0].notes = {{60, 100, 0, 480}};
    before.tracks[0].events = {cc(0, 7, 100), cc(960, 7, 50)};

    const auto after = deleteTime(before, 19200, 21120);

    REQUIRE(after);
    CHECK(*after == before);
}
