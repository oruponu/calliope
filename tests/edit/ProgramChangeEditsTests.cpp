#include "edit/ProgramChangeEdits.h"
#include "support/ControllerPointTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <optional>

using ControllerPointTest::result;

namespace
{
MidiEvent pc(int tick, int program)
{
    return {.type = MidiEvent::Type::ProgramChange, .tick = tick, .data1 = program};
}

MidiEvent cc(int tick, int number, int value)
{
    return {.type = MidiEvent::Type::ControlChange, .tick = tick, .data1 = number, .data2 = value};
}

MidiEvent bank(int tick, int msb)
{
    return cc(tick, 0, msb);
}

MidiEvent bankLsb(int tick, int lsb)
{
    return cc(tick, 32, lsb);
}
} // namespace

TEST_CASE("a block covers ticks from its program change to the next", "[edit][program-change]")
{
    const auto events = std::vector{pc(480, 1), pc(1920, 2)};
    CHECK(ProgramChangeEdits::blockTickAt(events, 0) == std::nullopt);
    CHECK(ProgramChangeEdits::blockTickAt(events, 480) == 480);
    CHECK(ProgramChangeEdits::blockTickAt(events, 1919) == 480);
    CHECK(ProgramChangeEdits::blockTickAt(events, 1920) == 1920);
    CHECK(ProgramChangeEdits::blockTickAt(events, 99999) == 1920);
}

TEST_CASE("blocks are found by tick when events are out of order", "[edit][program-change]")
{
    const auto events = std::vector{pc(1920, 2), pc(480, 1)};
    CHECK(ProgramChangeEdits::blockTickAt(events, 1000) == 480);
    CHECK(ProgramChangeEdits::voiceAt(events, 1000).program == 1);
}

TEST_CASE("the events on a block's tick are all of its program changes", "[edit][program-change]")
{
    CHECK((ProgramChangeEdits::onTick({pc(480, 1), cc(480, 0, 8), pc(480, 2), pc(960, 3)}, 480) == std::set{0, 2}));
}

TEST_CASE("the bank selects before the last program change on its tick are its group", "[edit][program-change]")
{
    const auto events = std::vector{bank(480, 8), bankLsb(480, 0), pc(480, 1), pc(480, 2)};
    CHECK((ProgramChangeEdits::bankSelectsOf(events, 3) == std::set{0, 1}));
    CHECK(ProgramChangeEdits::bankSelectsOf(events, 2).empty());
}

TEST_CASE("bank selects after the last program change or on an earlier tick are not in the group",
          "[edit][program-change]")
{
    const auto events = std::vector{bank(0, 8), pc(480, 1), bank(480, 16)};
    CHECK(ProgramChangeEdits::bankSelectsOf(events, 1).empty());
}

TEST_CASE("every repeated bank select before the program change is in the group", "[edit][program-change]")
{
    const auto events = std::vector{bank(480, 8), bank(480, 16), pc(480, 1)};
    CHECK((ProgramChangeEdits::bankSelectsOf(events, 2) == std::set{0, 1}));
}

TEST_CASE("program changes come with their bank selects", "[edit][program-change]")
{
    const auto events = std::vector{bank(480, 8), pc(480, 1), pc(960, 2)};
    CHECK((ProgramChangeEdits::withBankSelects(events, {1, 2}) == std::set{0, 1, 2}));
}

TEST_CASE("a bank select on an earlier tick is not in the group but counts for the voice", "[edit][program-change]")
{
    const auto events = std::vector{bank(0, 8), pc(5, 40)};
    CHECK(ProgramChangeEdits::bankSelectsOf(events, 1).empty());
    CHECK((ProgramChangeEdits::voiceOf(events, 1) == ProgramVoice{{8, 0}, 40}));
}

TEST_CASE("the voice keeps the bank across program changes", "[edit][program-change]")
{
    const auto events = std::vector{bank(0, 8), bankLsb(0, 2), pc(0, 1), pc(480, 2)};
    CHECK((ProgramChangeEdits::voiceOf(events, 3) == ProgramVoice{{8, 2}, 2}));
}

TEST_CASE("the voices of all program changes are found in one pass", "[edit][program-change]")
{
    const auto events = std::vector{bank(0, 8), pc(0, 1), bankLsb(480, 2), pc(480, 2), cc(960, 7, 100)};
    CHECK((ProgramChangeEdits::voicesOf(events) == std::map<int, ProgramVoice>{{1, {{8, 0}, 1}}, {3, {{8, 2}, 2}}}));
}

TEST_CASE("the voice in effect counts the tick itself", "[edit][program-change]")
{
    const auto events = std::vector{pc(480, 10), pc(480, 20), pc(960, 30)};
    CHECK(ProgramChangeEdits::voiceAt(events, 0) == ProgramVoice{});
    CHECK(ProgramChangeEdits::voiceAt(events, 480).program == 20);
    CHECK(ProgramChangeEdits::voiceAt(events, 959).program == 20);
    CHECK(ProgramChangeEdits::voiceAt(events, 960).program == 30);
}

TEST_CASE("the bank before a tick leaves out the bank selects on the tick", "[edit][program-change]")
{
    const auto events = std::vector{bank(0, 8), pc(0, 1), bank(480, 16), pc(480, 2), bank(480, 24)};
    CHECK((ProgramChangeEdits::bankBefore(events, 480) == ProgramBank{8, 0}));
    CHECK(ProgramChangeEdits::bankBefore(events, 0) == ProgramBank{});
}

TEST_CASE("placing bank 0 with no bank before writes no bank select", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::place({pc(0, 1)}, 480, {{0, 0}, 40}) == result({pc(0, 1), pc(480, 40)}, {1}));
}

TEST_CASE("placing bank 0 removes the group's bank selects when nothing is before", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::place({bank(480, 8), pc(480, 1)}, 480, {{0, 0}, 40}) == result({pc(480, 40)}, {0}));
}

TEST_CASE("placing bank 0 after a nonzero bank writes zeros", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::place({bank(0, 8), pc(0, 1)}, 480, {{0, 0}, 40}) ==
          result({bank(0, 8), pc(0, 1), bank(480, 0), bankLsb(480, 0), pc(480, 40)}, {4}));
}

TEST_CASE("placing a bank writes CC0 and CC32 before the program change", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::place({pc(0, 1)}, 480, {{8, 0}, 40}) ==
          result({pc(0, 1), bank(480, 8), bankLsb(480, 0), pc(480, 40)}, {3}));
}

TEST_CASE("placing a bank replaces the group's bank selects", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::place({bank(480, 8), bank(480, 16), pc(480, 1)}, 480, {{24, 0}, 40}) ==
          result({bank(480, 24), bankLsb(480, 0), pc(480, 40)}, {2}));
}

TEST_CASE("placing keeps other events on the tick", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::place({cc(480, 7, 100), pc(480, 1), cc(480, 10, 64)}, 480, {{8, 0}, 25}) ==
          result({cc(480, 7, 100), bank(480, 8), bankLsb(480, 0), pc(480, 25), cc(480, 10, 64)}, {3}));
}

TEST_CASE("placing on a tick with a waiting bank select puts the group before it", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::place({pc(480, 1), bank(480, 8)}, 480, {{16, 0}, 40}) ==
          result({bank(480, 16), bankLsb(480, 0), pc(480, 40), bank(480, 8)}, {2}));
}

TEST_CASE("placing on a tick with only a bank select puts the group before it", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::place({bank(480, 8)}, 480, {{0, 0}, 40}) == result({pc(480, 40), bank(480, 8)}, {0}));
}

TEST_CASE("place on a tick with several program changes keeps the last one", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::place({pc(480, 1), pc(480, 2), pc(480, 3)}, 480, {{0, 0}, 3}) ==
          result({pc(480, 3)}, {0}));
}

TEST_CASE("place on an empty tick goes after the events there", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::place({cc(480, 7, 100)}, 480, {{0, 0}, 25}) ==
          result({cc(480, 7, 100), pc(480, 25)}, {1}));
}
