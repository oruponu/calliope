#include "edit/ControllerPointEdits.h"
#include "support/ControllerPointTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace ControllerPointTest;

namespace
{
MidiEvent bend(int value)
{
    return {.type = MidiEvent::Type::PitchBend, .tick = 0, .data1 = value, .ramp = true};
}
} // namespace

TEST_CASE("a value is set and kept in range", "[edit][controller]")
{
    CHECK(ControllerPointEdits::setValue({p(0, 0), p(480, 40)}, cc1, 1, 300) == result({p(0, 0), p(480, 127)}, {1}));
}

TEST_CASE("a pitch bend value is kept in range", "[edit][controller]")
{
    CHECK(ControllerPointEdits::setValue({bend(8192)}, ControllerKind::pitchBend(), 0, -1) == result({bend(0)}, {0}));
    CHECK(ControllerPointEdits::setValue({bend(8192)}, ControllerKind::pitchBend(), 0, 20000) ==
          result({bend(16383)}, {0}));
}
