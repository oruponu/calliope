#include "ui/pianoroll/strips/RangeSelectGesture.h"
#include <catch2/catch_test_macros.hpp>
#include <set>
#include <vector>

namespace
{
TimelineGeometry unitGeometry()
{
    TimelineGeometry geometry{100};
    geometry.setTicksPerQuarterNote(64);
    geometry.setBeatWidth(64);
    return geometry;
}

std::set<int> selectPoints(const RangeSelectGesture& gesture, const std::vector<int>& ticks)
{
    return gesture.selectionFor(static_cast<int>(ticks.size()), unitGeometry(),
                                [&ticks](int i, int tickLo, int tickHi)
                                {
                                    const int tick = ticks[static_cast<size_t>(i)];
                                    return tick >= tickLo && tick <= tickHi;
                                });
}
} // namespace

TEST_CASE("the range bounds do not depend on the drag direction", "[ui][range-select]")
{
    const RangeSelectGesture gesture{300, 150, {}};
    CHECK(gesture.lo() == 150);
    CHECK(gesture.hi() == 300);
}

TEST_CASE("points inside the dragged range are selected", "[ui][range-select]")
{
    CHECK(selectPoints({200, 400, {}}, {0, 100, 250, 400}) == std::set<int>{1, 2});
}

TEST_CASE("dragging leftward selects the same points as dragging rightward", "[ui][range-select]")
{
    CHECK(selectPoints({400, 200, {}}, {0, 100, 250, 400}) == std::set<int>{1, 2});
}

TEST_CASE("the base selection is kept alongside the points in range", "[ui][range-select]")
{
    CHECK(selectPoints({200, 400, {0, 7}}, {0, 100, 250, 400}) == std::set<int>{0, 1, 2, 7});
}

TEST_CASE("the tick range passed to the predicate is converted from the x range", "[ui][range-select]")
{
    const RangeSelectGesture gesture{150, 130, {}};
    int seenLo = -1;
    int seenHi = -1;
    gesture.selectionFor(1, unitGeometry(),
                         [&](int, int tickLo, int tickHi)
                         {
                             seenLo = tickLo;
                             seenHi = tickHi;
                             return false;
                         });
    CHECK(seenLo == 30);
    CHECK(seenHi == 50);
}

TEST_CASE("only indices below the count are tested", "[ui][range-select]")
{
    const RangeSelectGesture gesture{0, 1000, {}};
    std::vector<int> tested;
    const auto selected = gesture.selectionFor(3, unitGeometry(),
                                               [&tested](int i, int, int)
                                               {
                                                   tested.push_back(i);
                                                   return true;
                                               });
    CHECK(tested == std::vector<int>{0, 1, 2});
    CHECK(selected == std::set<int>{0, 1, 2});
}
