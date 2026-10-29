#include "edit/controller/ControllerPointEdits.h"
#include "support/ControllerPointTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace ControllerPointTest;

TEST_CASE("all points of the kind are selected and other events are not", "[edit][controller][select]")
{
    CHECK(ControllerPointEdits::allPoints({p(0, 10), other(240), p(480, 20)}, cc1) == std::set<int>{0, 2});
}

TEST_CASE("no points are selected when the track has none of the kind", "[edit][controller][select]")
{
    CHECK(ControllerPointEdits::allPoints({other(0)}, cc1).empty());
    CHECK(ControllerPointEdits::allPoints({}, cc1).empty());
}
