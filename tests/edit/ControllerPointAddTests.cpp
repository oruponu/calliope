#include "edit/ControllerPointEdits.h"
#include "support/ControllerPointTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace ControllerPointTest;

TEST_CASE("an added point is a ramp at the given tick and value and is selected", "[edit][controller]")
{
    CHECK(ControllerPointEdits::add({p(0, 10), p(960, 20)}, cc1, 480, 64) ==
          result({p(0, 10), p(480, 64), p(960, 20)}, {1}));
}

TEST_CASE("an added point goes after points on the same tick", "[edit][controller]")
{
    CHECK(ControllerPointEdits::add({p(480, 127)}, cc1, 480, 0) == result({p(480, 127), p(480, 0)}, {1}));
}

TEST_CASE("an added value is kept in range", "[edit][controller]")
{
    CHECK(ControllerPointEdits::add({}, cc1, 0, 200) == result({p(0, 127)}, {0}));
    CHECK(ControllerPointEdits::add({}, ControllerKind::pitchBend(), 0, -5) ==
          result({{.type = MidiEvent::Type::PitchBend, .tick = 0, .data1 = 0, .ramp = true}}, {0}));
}

TEST_CASE("a point added for a controller that must not ramp is a step", "[edit][controller]")
{
    CHECK(ControllerPointEdits::add({}, ControllerKind::controlChange(0), 0, 3) ==
          result({{.type = MidiEvent::Type::ControlChange, .tick = 0, .data1 = 0, .data2 = 3}}, {0}));
}

TEST_CASE("adding puts events that were out of tick order in tick order", "[edit][controller]")
{
    CHECK(ControllerPointEdits::add({p(960, 20), other(0), p(0, 10)}, cc1, 480, 5) ==
          result({other(0), p(0, 10), p(480, 5), p(960, 20)}, {2}));
}
