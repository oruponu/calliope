#include "edit/ProgramChangeEdits.h"
#include "support/ControllerPointTestHelpers.h"
#include "support/CopiedControllerPointStringMaker.h"
#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <vector>

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

TEST_CASE("moving a program change carries its bank selects", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::move({bank(480, 8), bankLsb(480, 0), pc(480, 40)}, {2}, 2, 960) ==
          result({bank(960, 8), bankLsb(960, 0), pc(960, 40)}, {2}));
}

TEST_CASE("moving leaves a bank select on an earlier tick", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::move({bank(0, 8), pc(5, 40)}, {1}, 1, 960) == result({bank(0, 8), pc(960, 40)}, {1}));
}

TEST_CASE("moving over a group deletes it with its bank selects", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::move({pc(0, 1), bank(480, 8), pc(480, 2), pc(960, 3)}, {3}, 3, 240) ==
          result({pc(0, 1), pc(240, 3)}, {1}));
}

TEST_CASE("moving onto a group deletes it with its bank selects", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::move({bank(480, 8), pc(480, 2), pc(960, 3)}, {2}, 2, 480) == result({pc(480, 3)}, {0}));
}

TEST_CASE("moving keeps the other events on the old tick", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::move({cc(480, 7, 100), bank(480, 8), pc(480, 40)}, {2}, 2, 960) ==
          result({cc(480, 7, 100), bank(960, 8), pc(960, 40)}, {2}));
}

TEST_CASE("moving stops at tick 0 with the group", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::move({bank(480, 8), pc(480, 1), pc(960, 2)}, {1, 2}, 2, 0) ==
          result({bank(0, 8), pc(0, 1), pc(480, 2)}, {1, 2}));
}

TEST_CASE("removing a program change removes its bank selects", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::remove({bank(480, 8), bankLsb(480, 0), pc(480, 40), cc(480, 7, 100)}, {2}) ==
          result({cc(480, 7, 100)}, {}));
}

TEST_CASE("removing an earlier program change on the tick keeps the bank selects", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::remove({bank(480, 8), bankLsb(480, 0), pc(480, 1), pc(480, 2)}, {2}) ==
          result({bank(480, 8), bankLsb(480, 0), pc(480, 2)}, {}));
}

TEST_CASE("removing the last program change on the tick removes the bank selects", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::remove({bank(480, 8), bankLsb(480, 0), pc(480, 1), pc(480, 2)}, {3}) ==
          result({pc(480, 1)}, {}));
}

TEST_CASE("moving onto a waiting bank select puts the group before it", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::move({bank(480, 8), pc(960, 40)}, {1}, 1, 480) ==
          result({pc(480, 40), bank(480, 8)}, {0}));
    CHECK(ProgramChangeEdits::move({pc(0, 40), bank(480, 8)}, {0}, 0, 480) == result({pc(480, 40), bank(480, 8)}, {0}));
}

TEST_CASE("pasting onto a waiting bank select puts the group before it", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::paste({bank(960, 8)}, {{0, 40, false, 16, 0}}, 960) ==
          result({bank(960, 16), bankLsb(960, 0), pc(960, 40), bank(960, 8)}, {2}));
}

TEST_CASE("copied groups keep their bank selects", "[edit][program-change]")
{
    const auto copied = ProgramChangeEdits::copy({bank(0, 8), bankLsb(0, 1), pc(0, 40), pc(480, 41)}, {2, 3});
    CHECK(copied.kind == ControllerKind::programChange());
    CHECK(copied.points ==
          std::vector<CopiedControllerPoint>{{0, 40, false, 8, 1}, {480, 41, false, std::nullopt, std::nullopt}});
}

TEST_CASE("copied groups keep the program change kind", "[edit][program-change]")
{
    const auto copied = ProgramChangeEdits::copy({bank(0, 8), bankLsb(0, 1), pc(0, 40), pc(480, 41)}, {2, 3});
    CHECK_FALSE(ControllerKind::controlChange(0).acceptsPointsOf(copied.kind));
}

TEST_CASE("pasted groups put their bank selects before the program change", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::paste({pc(0, 1)}, {{0, 40, false, 8, 1}, {480, 41}}, 960) ==
          result({pc(0, 1), bank(960, 8), bankLsb(960, 1), pc(960, 40), pc(1440, 41)}, {3, 4}));
}

TEST_CASE("a group with one bank select pastes that one", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::paste({}, {{0, 40, false, std::nullopt, 3}}, 0) ==
          result({bankLsb(0, 3), pc(0, 40)}, {1}));
}

TEST_CASE("pasting over groups removes their bank selects", "[edit][program-change]")
{
    CHECK(ProgramChangeEdits::paste({bank(960, 8), pc(960, 7), cc(960, 7, 100)}, {{0, 40}}, 960) ==
          result({pc(960, 40), cc(960, 7, 100)}, {0}));
}
