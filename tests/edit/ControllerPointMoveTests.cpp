#include "edit/ControllerPointEdits.h"
#include "support/ControllerPointTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace ControllerPointTest;

namespace
{
const Events spread{p(0, 0), p(480, 40), p(960, 80), p(1440, 40)};
} // namespace

TEST_CASE("a dragged point moves to the target tick and value", "[edit][controller]")
{
    CHECK(ControllerPointEdits::move(spread, cc1, {1}, 1, 600, 50) ==
          result({p(0, 0), p(600, 50), p(960, 80), p(1440, 40)}, {1}));
}

TEST_CASE("points dragged together keep their differences", "[edit][controller]")
{
    CHECK(ControllerPointEdits::move(spread, cc1, {1, 2}, 1, 720, 60) ==
          result({p(0, 0), p(720, 60), p(1200, 100), p(1440, 40)}, {1, 2}));
}

TEST_CASE("a group stops at the value limit of any point", "[edit][controller]")
{
    CHECK(ControllerPointEdits::move(spread, cc1, {1, 2}, 1, 480, 120) ==
          result({p(0, 0), p(480, 87), p(960, 127), p(1440, 40)}, {1, 2}));
}

TEST_CASE("a group stops at tick 0", "[edit][controller]")
{
    CHECK(ControllerPointEdits::move(spread, cc1, {1, 2}, 1, -1000, 40) ==
          result({p(0, 0), p(0, 40), p(480, 80), p(1440, 40)}, {1, 2}));
}

TEST_CASE("a passed point is deleted", "[edit][controller]")
{
    CHECK(ControllerPointEdits::move(spread, cc1, {1}, 1, 1200, 40) ==
          result({p(0, 0), p(1200, 40), p(1440, 40)}, {1}));
}

TEST_CASE("a point landing on the tick of another point from the left goes before it", "[edit][controller]")
{
    CHECK(ControllerPointEdits::move(spread, cc1, {1}, 1, 960, 40) ==
          result({p(0, 0), p(960, 40), p(960, 80), p(1440, 40)}, {1}));
}

TEST_CASE("a point landing on the tick of another point from the right goes after it", "[edit][controller]")
{
    CHECK(ControllerPointEdits::move(spread, cc1, {2}, 2, 480, 80) ==
          result({p(0, 0), p(480, 40), p(480, 80), p(1440, 40)}, {2}));
}

TEST_CASE("a point dragged away from its pair leaves the other point", "[edit][controller]")
{
    const Events before{p(0, 0), p(480, 127), p(480, 0), p(960, 0)};
    CHECK(ControllerPointEdits::move(before, cc1, {2}, 2, 720, 0) ==
          result({p(0, 0), p(480, 127), p(720, 0), p(960, 0)}, {2}));
    CHECK(ControllerPointEdits::move(before, cc1, {1}, 1, 240, 127) ==
          result({p(0, 0), p(240, 127), p(480, 0), p(960, 0)}, {1}));
}

TEST_CASE("dragging the last point of a pair to the left takes the first along", "[edit][controller]")
{
    const Events before{p(0, 127), p(480, 127), p(480, 64)};
    CHECK(ControllerPointEdits::move(before, cc1, {2}, 2, 240, 64) ==
          result({p(0, 127), p(240, 127), p(240, 64)}, {2}));
    CHECK(ControllerPointEdits::move(before, cc1, {2}, 2, 240, 30) ==
          result({p(0, 127), p(240, 127), p(240, 30)}, {2}));
}

TEST_CASE("dragging the first point of a pair to the right takes the last along", "[edit][controller]")
{
    const Events before{p(480, 127), p(480, 64), p(960, 0)};
    CHECK(ControllerPointEdits::move(before, cc1, {0}, 0, 720, 127) ==
          result({p(720, 127), p(720, 64), p(960, 0)}, {0}));
}

TEST_CASE("a pair dragged as a whole deletes the pairs it passes", "[edit][controller]")
{
    const Events before{p(0, 40), p(240, 100), p(240, 0), p(480, 127), p(480, 64)};
    CHECK(ControllerPointEdits::move(before, cc1, {4}, 4, 120, 64) == result({p(0, 40), p(120, 127), p(120, 64)}, {2}));
}

TEST_CASE("passing a pair to the right deletes its first point and carries its last after", "[edit][controller]")
{
    const Events before{p(0, 40), p(480, 127), p(480, 0), p(1440, 20)};
    CHECK(ControllerPointEdits::move(before, cc1, {0}, 0, 960, 40) ==
          result({p(960, 40), p(960, 0), p(1440, 20)}, {0}));
}

TEST_CASE("passing a pair to the left deletes its last point and carries its first before", "[edit][controller]")
{
    const Events before{p(0, 40), p(480, 127), p(480, 0), p(1440, 20)};
    CHECK(ControllerPointEdits::move(before, cc1, {3}, 3, 240, 20) == result({p(0, 40), p(240, 127), p(240, 20)}, {2}));
}

TEST_CASE("landing on a pair from the right replaces its last point", "[edit][controller]")
{
    const Events before{p(0, 127), p(480, 127), p(480, 64), p(960, 0)};
    CHECK(ControllerPointEdits::move(before, cc1, {3}, 3, 480, 0) == result({p(0, 127), p(480, 127), p(480, 0)}, {2}));
    CHECK(ControllerPointEdits::move(before, cc1, {3}, 3, 240, 0) == result({p(0, 127), p(240, 127), p(240, 0)}, {2}));
}

TEST_CASE("landing on a pair from the left replaces its first point", "[edit][controller]")
{
    const Events before{p(0, 0), p(480, 127), p(480, 64), p(960, 0)};
    CHECK(ControllerPointEdits::move(before, cc1, {0}, 0, 480, 0) == result({p(480, 0), p(480, 64), p(960, 0)}, {0}));
}

TEST_CASE("passing three points on one tick carries only the far end", "[edit][controller]")
{
    const Events before{p(0, 40), p(480, 127), p(480, 64), p(480, 0), p(1440, 20)};
    CHECK(ControllerPointEdits::move(before, cc1, {0}, 0, 960, 40) ==
          result({p(960, 40), p(960, 0), p(1440, 20)}, {0}));
}

TEST_CASE("only the pair nearest the destination is carried", "[edit][controller]")
{
    const Events before{p(0, 40), p(240, 100), p(240, 0), p(480, 127), p(480, 64), p(1440, 20)};
    CHECK(ControllerPointEdits::move(before, cc1, {0}, 0, 960, 40) ==
          result({p(960, 40), p(960, 64), p(1440, 20)}, {0}));
}

TEST_CASE("a group drag deletes every passed point", "[edit][controller]")
{
    const Events before{p(0, 40), p(240, 60), p(480, 127), p(480, 0), p(1440, 20)};
    CHECK(ControllerPointEdits::move(before, cc1, {0, 1}, 0, 720, 40) ==
          result({p(720, 40), p(960, 60), p(1440, 20)}, {0, 1}));
}

TEST_CASE("a point of a stepped controller landing on the tick of another point replaces it", "[edit][controller]")
{
    const auto pedal = ControllerKind::controlChange(64);
    const Events before{pedal.point(0, 0), pedal.point(1920, 127), pedal.point(3840, 0)};
    CHECK(ControllerPointEdits::move(before, pedal, {2}, 2, 1920, 0) ==
          result({pedal.point(0, 0), pedal.point(1920, 0)}, {1}));
    CHECK(ControllerPointEdits::move(before, pedal, {0}, 0, 1920, 0) ==
          result({pedal.point(1920, 0), pedal.point(3840, 0)}, {0}));
}

TEST_CASE("a point of a stepped controller landing on points stacked on one tick replaces them all",
          "[edit][controller]")
{
    const auto pedal = ControllerKind::controlChange(64);
    const Events before{pedal.point(0, 0), pedal.point(1920, 127), pedal.point(1920, 64), pedal.point(3840, 0)};
    CHECK(ControllerPointEdits::move(before, pedal, {3}, 3, 1920, 0) ==
          result({pedal.point(0, 0), pedal.point(1920, 0)}, {1}));
    CHECK(ControllerPointEdits::move(before, pedal, {3}, 3, 960, 0) ==
          result({pedal.point(0, 0), pedal.point(960, 0)}, {1}));
}

TEST_CASE("points of a stepped controller dragged together replace the points where they land", "[edit][controller]")
{
    const auto pedal = ControllerKind::controlChange(64);
    const Events before{pedal.point(0, 0), pedal.point(960, 127), pedal.point(1920, 0), pedal.point(2880, 127)};
    CHECK(ControllerPointEdits::move(before, pedal, {0, 1}, 0, 960, 0) ==
          result({pedal.point(960, 0), pedal.point(1920, 127), pedal.point(2880, 127)}, {0, 1}));
}

TEST_CASE("a value-only drag of a stepped controller keeps the points on its tick", "[edit][controller]")
{
    const auto pedal = ControllerKind::controlChange(64);
    const Events before{pedal.point(480, 127), pedal.point(480, 0)};
    CHECK(ControllerPointEdits::move(before, pedal, {1}, 1, 480, 10) ==
          result({pedal.point(480, 127), pedal.point(480, 10)}, {1}));
}

TEST_CASE("events of another kind are never deleted or carried", "[edit][controller]")
{
    const Events before{p(0, 40), other(480), p(960, 80)};
    CHECK(ControllerPointEdits::move(before, cc1, {0}, 0, 1200, 40) == result({other(480), p(1200, 40)}, {1}));
}

TEST_CASE("a value-only drag keeps the order", "[edit][controller]")
{
    const Events before{p(480, 127), p(480, 0)};
    CHECK(ControllerPointEdits::move(before, cc1, {1}, 1, 480, 10) == result({p(480, 127), p(480, 10)}, {1}));
}

TEST_CASE("the moved anchor lands on the target", "[edit][controller]")
{
    CHECK(ControllerPointEdits::movedAnchor(spread, cc1, {1}, 1, 600, 50) == p(600, 50));
}

TEST_CASE("the moved anchor stops with its group at the value limit", "[edit][controller]")
{
    CHECK(ControllerPointEdits::movedAnchor(spread, cc1, {1, 2}, 1, 480, 120) == p(480, 87));
}

TEST_CASE("the moved anchor stops with its group at tick 0", "[edit][controller]")
{
    CHECK(ControllerPointEdits::movedAnchor(spread, cc1, {1, 2}, 2, -1000, 80) == p(480, 80));
}

TEST_CASE("an anchor outside the moved points does not move", "[edit][controller]")
{
    CHECK_FALSE(ControllerPointEdits::movedAnchor(spread, cc1, {2}, 1, 600, 50).has_value());
}
