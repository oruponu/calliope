#include "engine/Chase.h"
#include "engine/ChaseTarget.h"
#include "support/MidiEventTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <utility>
#include <vector>

namespace
{
const PlaybackTrackContext deviceCh1{TrackId{1}, 1, TrackId{1}, MidiTrack::OutputDestination::MidiDevice};
const PlaybackTrackContext otherDeviceCh1{TrackId{2}, 1, TrackId{2}, MidiTrack::OutputDestination::MidiDevice};
const PlaybackTrackContext deviceCh2{TrackId{3}, 2, TrackId{3}, MidiTrack::OutputDestination::MidiDevice};
const PlaybackTrackContext pluginCh1{TrackId{4}, 1, TrackId{4}, MidiTrack::OutputDestination::Plugin};
const PlaybackTrackContext silentCh1{TrackId{5}, 1, TrackId{5}, MidiTrack::OutputDestination::None};

MidiEvent cc(int tick, int number, int value)
{
    return {.type = MidiEvent::Type::ControlChange, .tick = tick, .data1 = number, .data2 = value};
}

MidiEvent pc(int tick, int program)
{
    return {.type = MidiEvent::Type::ProgramChange, .tick = tick, .data1 = program};
}

MidiEvent pb(int tick, int value)
{
    return {.type = MidiEvent::Type::PitchBend, .tick = tick, .data1 = value};
}

MidiEvent cp(int tick, int value)
{
    return {.type = MidiEvent::Type::ChannelPressure, .tick = tick, .data1 = value};
}

MidiEvent kp(int tick, int note, int value)
{
    return {.type = MidiEvent::Type::KeyPressure, .tick = tick, .data1 = note, .data2 = value};
}

PlaybackSnapshot snapshotOf(std::vector<ScheduledEvent> events)
{
    PlaybackSnapshot snap;
    snap.events = std::move(events);
    return snap;
}

std::vector<ScheduledEvent> on(const PlaybackTrackContext& ctx, std::vector<MidiEvent> events)
{
    std::vector<ScheduledEvent> scheduled;
    for (const auto& e : events)
        scheduled.push_back({ctx, e});
    return scheduled;
}

std::vector<MidiEvent> eventsOf(const std::vector<ScheduledEvent>& scheduled)
{
    std::vector<MidiEvent> events;
    for (const auto& se : scheduled)
        events.push_back(se.event);
    return events;
}

std::vector<MidiEvent> chased(std::vector<MidiEvent> events)
{
    return eventsOf(chaseEvents(snapshotOf(on(deviceCh1, std::move(events))), 1000));
}
} // namespace

TEST_CASE("nothing is sent without events before the start", "[engine][chase]")
{
    CHECK(chased({cc(1200, 7, 80)}).empty());
}

TEST_CASE("events at the start tick are left to playback", "[engine][chase]")
{
    CHECK(chased({cc(1000, 7, 80), pc(1000, 5)}).empty());
}

TEST_CASE("the last value of each controller is sent in controller number order", "[engine][chase]")
{
    CHECK(chased({cc(0, 10, 30), cc(0, 7, 100), cc(500, 7, 80)}) ==
          std::vector<MidiEvent>{cc(1000, 7, 80), cc(1000, 10, 30)});
}

TEST_CASE("program change is sent with the bank select in effect at it", "[engine][chase]")
{
    CHECK(chased({cc(0, 0, 1), cc(0, 32, 2), pc(10, 5)}) ==
          std::vector<MidiEvent>{cc(1000, 0, 1), cc(1000, 32, 2), pc(1000, 5)});
}

TEST_CASE("bank select after the last program change is sent after it", "[engine][chase]")
{
    CHECK(chased({cc(0, 0, 1), pc(10, 5), cc(20, 0, 3)}) ==
          std::vector<MidiEvent>{cc(1000, 0, 1), pc(1000, 5), cc(1000, 0, 3)});
}

TEST_CASE("bank select without a program change is sent alone", "[engine][chase]")
{
    CHECK(chased({cc(0, 0, 3)}) == std::vector<MidiEvent>{cc(1000, 0, 3)});
}

TEST_CASE("program change is sent before controllers, pitch bend and channel pressure", "[engine][chase]")
{
    CHECK(chased({cp(0, 40), pb(0, 9000), cc(0, 7, 90), pc(0, 5)}) ==
          std::vector<MidiEvent>{pc(1000, 5), cc(1000, 7, 90), pb(1000, 9000), cp(1000, 40)});
}

TEST_CASE("data entry is sent for each parameter with its selection, then the last selection is restored",
          "[engine][chase]")
{
    CHECK(chased({cc(0, 101, 0), cc(0, 100, 0), cc(0, 6, 12), cc(0, 101, 0), cc(0, 100, 1), cc(0, 6, 64), cc(0, 38, 0),
                  cc(0, 101, 127), cc(0, 100, 127)}) ==
          std::vector<MidiEvent>{cc(1000, 101, 0), cc(1000, 100, 0), cc(1000, 6, 12), cc(1000, 101, 0),
                                 cc(1000, 100, 1), cc(1000, 6, 64), cc(1000, 38, 0), cc(1000, 101, 127),
                                 cc(1000, 100, 127)});
}

TEST_CASE("a parameter still selected at the start is selected again last", "[engine][chase]")
{
    CHECK(chased({cc(0, 101, 0), cc(0, 100, 0), cc(0, 6, 12)}) ==
          std::vector<MidiEvent>{cc(1000, 101, 0), cc(1000, 100, 0), cc(1000, 6, 12), cc(1000, 101, 0),
                                 cc(1000, 100, 0)});
}

TEST_CASE("NRPN data entry is sent with its NRPN selection after RPN", "[engine][chase]")
{
    CHECK(chased({cc(0, 99, 1), cc(0, 98, 8), cc(0, 6, 70), cc(0, 101, 0), cc(0, 100, 0), cc(0, 6, 2)}) ==
          std::vector<MidiEvent>{cc(1000, 101, 0), cc(1000, 100, 0), cc(1000, 6, 2), cc(1000, 99, 1), cc(1000, 98, 8),
                                 cc(1000, 6, 70), cc(1000, 101, 0), cc(1000, 100, 0)});
}

TEST_CASE("data entry without a parameter selection is not sent", "[engine][chase]")
{
    CHECK(chased({cc(0, 6, 12), cc(0, 38, 0)}).empty());
}

TEST_CASE("data entry after a selection with one part only is not sent", "[engine][chase]")
{
    CHECK(chased({cc(0, 101, 0), cc(0, 6, 12)}) == std::vector<MidiEvent>{cc(1000, 101, 0)});
}

TEST_CASE("switching from NRPN to RPN forgets the other part", "[engine][chase]")
{
    CHECK(chased({cc(0, 99, 1), cc(0, 98, 8), cc(0, 101, 0), cc(0, 6, 12)}) ==
          std::vector<MidiEvent>{cc(1000, 101, 0)});
}

TEST_CASE("data increment and decrement are not sent", "[engine][chase]")
{
    CHECK(chased({cc(0, 101, 0), cc(0, 100, 0), cc(0, 96, 0), cc(0, 97, 0)}) ==
          std::vector<MidiEvent>{cc(1000, 101, 0), cc(1000, 100, 0)});
}

TEST_CASE("reset all controllers resets the controllers it covers and keeps volume and program", "[engine][chase]")
{
    CHECK(chased({cc(0, 7, 100), cc(0, 11, 90), cc(0, 64, 127), pb(0, 0), cp(0, 50), pc(0, 3), cc(0, 101, 0),
                  cc(0, 100, 0), cc(0, 6, 12), cc(100, 121, 0), cc(200, 1, 20)}) ==
          std::vector<MidiEvent>{pc(1000, 3), cc(1000, 1, 20), cc(1000, 7, 100), cc(1000, 11, 127), cc(1000, 64, 0),
                                 cc(1000, 65, 0), cc(1000, 66, 0), cc(1000, 67, 0), cc(1000, 101, 0), cc(1000, 100, 0),
                                 cc(1000, 6, 12), cc(1000, 101, 127), cc(1000, 100, 127), pb(1000, 8192), cp(1000, 0)});
}

TEST_CASE("polyphonic key pressure and channel mode messages are not sent", "[engine][chase]")
{
    CHECK(chased({kp(0, 60, 30), cc(0, 120, 0), cc(0, 123, 0), cc(0, 126, 1)}).empty());
}

TEST_CASE("tracks sharing a channel give the value of the later event", "[engine][chase]")
{
    auto events = on(deviceCh1, {pc(100, 5)});
    events.push_back({otherDeviceCh1, pc(200, 9)});
    const auto result = chaseEvents(snapshotOf(events), 1000);

    REQUIRE(eventsOf(result) == std::vector<MidiEvent>{pc(1000, 9)});
    CHECK(result[0].ctx == deviceCh1);
}

TEST_CASE("events at the same tick on a shared channel give the value later in the snapshot", "[engine][chase]")
{
    auto events = on(deviceCh1, {cc(100, 7, 50)});
    events.push_back({otherDeviceCh1, cc(100, 7, 60)});

    CHECK(eventsOf(chaseEvents(snapshotOf(events), 1000)) == std::vector<MidiEvent>{cc(1000, 7, 60)});
}

TEST_CASE("each output and channel is chased separately in order of first event", "[engine][chase]")
{
    const auto result =
        chaseEvents(snapshotOf({{deviceCh2, pc(0, 2)}, {pluginCh1, pc(10, 3)}, {deviceCh1, pc(20, 4)}}), 1000);

    REQUIRE(result.size() == 3);
    CHECK(result[0].ctx == deviceCh2);
    CHECK(result[0].event == pc(1000, 2));
    CHECK(result[1].ctx == pluginCh1);
    CHECK(result[1].event == pc(1000, 3));
    CHECK(result[2].ctx == deviceCh1);
    CHECK(result[2].event == pc(1000, 4));
}

TEST_CASE("tracks without output are not chased", "[engine][chase]")
{
    CHECK(chaseEvents(snapshotOf({{silentCh1, pc(0, 5)}}), 1000).empty());
}

TEST_CASE("a MIDI device target ignores the route target and a plugin target keeps it", "[engine][chase]")
{
    using Destination = MidiTrack::OutputDestination;
    CHECK(chaseTargetOf({TrackId{1}, 1, TrackId{8}, Destination::MidiDevice}) ==
          chaseTargetOf({TrackId{2}, 1, TrackId{9}, Destination::MidiDevice}));
    CHECK_FALSE(chaseTargetOf({TrackId{1}, 1, TrackId{8}, Destination::Plugin}) ==
                chaseTargetOf({TrackId{2}, 1, TrackId{9}, Destination::Plugin}));
}
