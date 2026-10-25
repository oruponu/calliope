#include "model/ControllerKind.h"
#include "support/MidiEventTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

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

TEST_CASE("a program change is its own controller", "[model][program-change]")
{
    const auto kind = ControllerKind::programChange();
    CHECK(ControllerKind::of(pc(480, 40)) == kind);
    CHECK(kind.matches(pc(480, 40)));
    CHECK_FALSE(kind.matches(cc(480, 0, 8)));
    CHECK(kind.valueOf(pc(480, 40)) == 40);
    CHECK(kind.maxValue() == 127);
    CHECK_FALSE(kind.canRamp());
}

TEST_CASE("a program change point holds the program in data1", "[model][program-change]")
{
    CHECK(ControllerKind::programChange().point(480, 200) ==
          MidiEvent{.type = MidiEvent::Type::ProgramChange, .tick = 480, .data1 = 127, .data2 = 0, .ramp = false});
}

TEST_CASE("points go only into a controller of the same type", "[model][program-change]")
{
    CHECK(ControllerKind::controlChange(1).acceptsPointsOf(ControllerKind::controlChange(11)));
    CHECK(ControllerKind::pitchBend().acceptsPointsOf(ControllerKind::pitchBend()));
    CHECK_FALSE(ControllerKind::programChange().acceptsPointsOf(ControllerKind::controlChange(1)));
    CHECK_FALSE(ControllerKind::controlChange(1).acceptsPointsOf(ControllerKind::programChange()));
    CHECK_FALSE(ControllerKind::controlChange(1).acceptsPointsOf(ControllerKind::pitchBend()));
}
