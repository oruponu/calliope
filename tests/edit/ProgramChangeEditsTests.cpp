#include "edit/ProgramChangeEdits.h"
#include <catch2/catch_test_macros.hpp>
#include <optional>

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
}

TEST_CASE("the events on a block's tick are all of its program changes", "[edit][program-change]")
{
    CHECK((ProgramChangeEdits::onTick({pc(480, 1), cc(480, 0, 8), pc(480, 2), pc(960, 3)}, 480) == std::set{0, 2}));
}
