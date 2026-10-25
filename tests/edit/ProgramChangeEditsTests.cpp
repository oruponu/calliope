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
    CHECK(ProgramChangeEdits::programAt(events, 1000) == 1);
}

TEST_CASE("the events on a block's tick are all of its program changes", "[edit][program-change]")
{
    CHECK((ProgramChangeEdits::onTick({pc(480, 1), cc(480, 0, 8), pc(480, 2), pc(960, 3)}, 480) == std::set{0, 2}));
}

TEST_CASE("the program in effect counts the tick itself", "[edit][program-change]")
{
    const auto events = std::vector{pc(480, 10), pc(480, 20), pc(960, 30)};
    CHECK(ProgramChangeEdits::programAt(events, 0) == 0);
    CHECK(ProgramChangeEdits::programAt(events, 480) == 20);
    CHECK(ProgramChangeEdits::programAt(events, 959) == 20);
    CHECK(ProgramChangeEdits::programAt(events, 960) == 30);
}

TEST_CASE("place adds a program change on an empty tick", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::place({pc(0, 1), pc(960, 3)}, 480, 1) == result({pc(0, 1), pc(480, 1), pc(960, 3)}, {1}));
}

TEST_CASE("place replaces the program on its tick", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::place({pc(0, 1), pc(480, 2)}, 480, 40) == result({pc(0, 1), pc(480, 40)}, {1}));
}

TEST_CASE("place on a tick with several program changes keeps the last one", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::place({pc(480, 1), pc(480, 2), pc(480, 3)}, 480, 3) == result({pc(480, 3)}, {0}));
}

TEST_CASE("place keeps other events on the tick", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::place({cc(480, 0, 8), pc(480, 1), cc(480, 32, 0)}, 480, 25) ==
          result({cc(480, 0, 8), pc(480, 25), cc(480, 32, 0)}, {1}));
}

TEST_CASE("place on an empty tick goes after the events there", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::place({cc(480, 0, 8)}, 480, 25) == result({cc(480, 0, 8), pc(480, 25)}, {1}));
}
