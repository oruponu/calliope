#include "edit/ControllerPointEdits.h"
#include "support/ControllerPointTestHelpers.h"
#include "support/CopiedControllerPointStringMaker.h"
#include <catch2/catch_test_macros.hpp>

using namespace ControllerPointTest;

TEST_CASE("copy counts ticks from the first selected point", "[edit][controller][clipboard]")
{
    const Events events{p(0, 10), p(480, 20), p(960, 30), p(1440, 40)};
    CHECK(ControllerPointEdits::copy(events, cc1, {1, 2}) ==
          CopiedControllerPoints{cc1, {{0, 20, true}, {480, 30, true}}});
}

TEST_CASE("copy keeps two points on one tick in order", "[edit][controller][clipboard]")
{
    CHECK(ControllerPointEdits::copy({p(480, 100), p(480, 0)}, cc1, {0, 1}).points ==
          std::vector<CopiedControllerPoint>{{0, 100, true}, {0, 0, true}});
}

TEST_CASE("copy skips indices that are not points of the kind", "[edit][controller][clipboard]")
{
    CHECK(ControllerPointEdits::copy({other(0), p(480, 20)}, cc1, {0, 1, 5}).points ==
          std::vector<CopiedControllerPoint>{{0, 20, true}});
}

TEST_CASE("pasted points start at the paste tick and are selected", "[edit][controller][clipboard]")
{
    CHECK(ControllerPointEdits::paste({p(0, 10)}, cc1, {{0, 20, true}, {480, 30, true}}, 960) ==
          result({p(0, 10), p(960, 20), p(1440, 30)}, {1, 2}));
}

TEST_CASE("paste replaces points of the kind from its first tick to its last", "[edit][controller][clipboard]")
{
    CHECK(ControllerPointEdits::paste({p(0, 10), p(960, 99), p(1200, 99), p(1440, 99), p(1920, 50)}, cc1,
                                      {{0, 20, true}, {480, 30, true}},
                                      960) == result({p(0, 10), p(960, 20), p(1440, 30), p(1920, 50)}, {1, 2}));
}

TEST_CASE("a single pasted point replaces only the points on its tick", "[edit][controller][clipboard]")
{
    CHECK(ControllerPointEdits::paste({p(480, 99), p(480, 98), p(960, 50)}, cc1, {{0, 20, true}}, 480) ==
          result({p(480, 20), p(960, 50)}, {0}));
}

TEST_CASE("paste keeps other kinds inside the range", "[edit][controller][clipboard]")
{
    CHECK(ControllerPointEdits::paste({other(600)}, cc1, {{0, 20, true}, {480, 30, true}}, 480) ==
          result({p(480, 20), other(600), p(960, 30)}, {0, 2}));
}

TEST_CASE("pasted points take the target controller number", "[edit][controller][clipboard]")
{
    const auto cc11 = ControllerKind::controlChange(11);
    CHECK(ControllerPointEdits::paste({}, cc11, {{0, 20, true}}, 0).events == Events{cc11.point(0, 20)});
}

TEST_CASE("pasted points do not ramp where the target cannot", "[edit][controller][clipboard]")
{
    const auto bank = ControllerKind::controlChange(0);
    CHECK_FALSE(ControllerPointEdits::paste({}, bank, {{0, 20, true}}, 0).events.front().ramp);
}

TEST_CASE("steps pasted where the target can ramp become hold points and ramps", "[edit][controller][clipboard]")
{
    CHECK(ControllerPointEdits::paste({}, cc1, {{0, 0, false}, {1920, 127, false}, {3840, 0, false}}, 0) ==
          result({p(0, 0), p(1920, 0), p(1920, 127), p(3840, 127), p(3840, 0)}, {0, 1, 2, 3, 4}));
}

TEST_CASE("a pasted step holds the value before the paste", "[edit][controller][clipboard]")
{
    CHECK(ControllerPointEdits::paste({p(0, 64)}, cc1, {{0, 0, false}}, 480) ==
          result({p(0, 64), p(480, 64), p(480, 0)}, {1, 2}));
}

TEST_CASE("a pasted step to the same value or to the next tick gets no hold point", "[edit][controller][clipboard]")
{
    CHECK(ControllerPointEdits::paste({}, cc1, {{0, 10, false}, {480, 10, false}, {481, 20, false}}, 0) ==
          result({p(0, 10), p(480, 10), p(481, 20)}, {0, 1, 2}));
}

TEST_CASE("hold points pasted where the target cannot ramp leave only the steps", "[edit][controller][clipboard]")
{
    const auto bank = ControllerKind::controlChange(0);
    CHECK(ControllerPointEdits::paste(
              {}, bank, {{0, 0, true}, {1920, 0, true}, {1920, 127, true}, {3840, 127, true}, {3840, 0, true}}, 0) ==
          result({bank.point(0, 0), bank.point(1920, 127), bank.point(3840, 0)}, {0, 1, 2}));
}

TEST_CASE("a first pasted hold point is judged by the value before the paste", "[edit][controller][clipboard]")
{
    const auto bank = ControllerKind::controlChange(0);
    CHECK(ControllerPointEdits::paste({bank.point(0, 64)}, bank, {{0, 64, true}, {0, 0, true}}, 480) ==
          result({bank.point(0, 64), bank.point(480, 0)}, {1}));
    CHECK(ControllerPointEdits::paste({bank.point(0, 10)}, bank, {{0, 64, true}, {0, 0, true}}, 480) ==
          result({bank.point(0, 10), bank.point(480, 64), bank.point(480, 0)}, {1, 2}));
}

TEST_CASE("events copied where the controller cannot ramp are all pasted", "[edit][controller][clipboard]")
{
    const auto increment = ControllerKind::controlChange(96);
    CHECK(ControllerPointEdits::paste({}, increment, {{0, 0, false}, {480, 0, false}, {480, 0, false}}, 0) ==
          result({increment.point(0, 0), increment.point(480, 0), increment.point(480, 0)}, {0, 1, 2}));
}

TEST_CASE("pasting the same points leaves the events as they are", "[edit][controller][clipboard]")
{
    const Events events{p(0, 10), p(480, 20)};
    CHECK(ControllerPointEdits::paste(events, cc1, {{0, 20, true}}, 480).events == events);
}

TEST_CASE("pasted pitch bend keeps its value", "[edit][controller][clipboard]")
{
    const auto bend = ControllerKind::pitchBend();
    CHECK(ControllerPointEdits::paste({}, bend, {{0, 16383, true}}, 0).events == Events{bend.point(0, 16383)});
}

TEST_CASE("pasting a point back in place keeps its place among other events on its tick",
          "[edit][controller][clipboard]")
{
    const Events events{p(480, 20), other(480)};
    CHECK(ControllerPointEdits::paste(events, cc1, {{0, 20, true}}, 480) == result(events, {0}));
}
