#include "edit/ControllerPointEdits.h"
#include "support/ControllerPointTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace ControllerPointTest;

TEST_CASE("removed points are taken out", "[edit][controller]")
{
    CHECK(ControllerPointEdits::remove({p(0, 0), p(480, 40), p(960, 80)}, {1}) == result({p(0, 0), p(960, 80)}, {}));
}
