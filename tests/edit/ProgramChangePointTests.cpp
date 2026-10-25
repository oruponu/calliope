#include "edit/ControllerPointEdits.h"
#include "support/ControllerPointTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace ControllerPointTest;

namespace
{
MidiEvent pc(int tick, int program)
{
    return {.type = MidiEvent::Type::ProgramChange, .tick = tick, .data1 = program};
}

MidiEvent bank(int tick, int msb)
{
    return {.type = MidiEvent::Type::ControlChange, .tick = tick, .data1 = 0, .data2 = msb};
}
} // namespace

TEST_CASE("moving a program change passes over and lands on others", "[edit][program-change]")
{
    const auto kind = ControllerKind::programChange();
    CHECK(ControllerPointEdits::move({pc(0, 1), pc(480, 2), pc(960, 3), pc(1440, 4)}, kind, {3}, 3, 480, 4) ==
          result({pc(0, 1), pc(480, 4)}, {1}));
}

TEST_CASE("moving keeps the program", "[edit][program-change]")
{
    const auto kind = ControllerKind::programChange();
    CHECK(ControllerPointEdits::move({pc(0, 40)}, kind, {0}, 0, 960, 40) == result({pc(960, 40)}, {0}));
}

TEST_CASE("moving a program change keeps the bank select on its tick", "[edit][program-change]")
{
    const auto kind = ControllerKind::programChange();
    CHECK(ControllerPointEdits::move({bank(480, 8), pc(480, 25)}, kind, {1}, 1, 960, 25) ==
          result({bank(480, 8), pc(960, 25)}, {1}));
}
