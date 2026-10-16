#include "engine/InputQuantize.h"
#include "support/GridTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
constexpr LoopRange noLoop{};
constexpr LoopRange oneBarLoop{true, 0, 1920};
} // namespace

TEST_CASE("input is quantized to the nearest line", "[engine][input-quantize]")
{
    CHECK(quantizeInputStart(130, GridTest::fourFourSixteenths, noLoop) == 120);
    CHECK(quantizeInputStart(230, GridTest::fourFourSixteenths, noLoop) == 240);
}

TEST_CASE("input halfway between two lines is quantized to the later one", "[engine][input-quantize]")
{
    CHECK(quantizeInputStart(180, GridTest::fourFourSixteenths, noLoop) == 240);
}

TEST_CASE("input is quantized to triplet lines counted from each bar start", "[engine][input-quantize]")
{
    CHECK(quantizeInputStart(1300, GridTest::threeFourQuarterTriplets, noLoop) == 1280);
    CHECK(quantizeInputStart(1400, GridTest::threeFourQuarterTriplets, noLoop) == 1440);
}

TEST_CASE("input inside the loop quantized to the loop end or later starts the next pass", "[engine][input-quantize]")
{
    CHECK(quantizeInputStart(1900, GridTest::fourFourSixteenths, oneBarLoop) == std::nullopt);
    CHECK(quantizeInputStart(1919, GridTest::fourFourSixteenths, oneBarLoop) == std::nullopt);
}

TEST_CASE("input inside the loop quantized before the loop end stays in the pass", "[engine][input-quantize]")
{
    CHECK(quantizeInputStart(1850, GridTest::fourFourSixteenths, oneBarLoop) == 1800);
}

TEST_CASE("input inside the loop quantized before the loop start starts at the loop start", "[engine][input-quantize]")
{
    CHECK(quantizeInputStart(55, GridTest::fourFourSixteenths, {true, 50, 1920}) == 50);
}

TEST_CASE("input is quantized as usual when the loop is off or empty", "[engine][input-quantize]")
{
    CHECK(quantizeInputStart(1900, GridTest::fourFourSixteenths, {false, 0, 1920}) == 1920);
    CHECK(quantizeInputStart(1900, GridTest::fourFourSixteenths, {true, 1920, 1920}) == 1920);
}

TEST_CASE("input outside the loop is quantized without the loop rules", "[engine][input-quantize]")
{
    CHECK(quantizeInputStart(1930, GridTest::fourFourSixteenths, oneBarLoop) == 1920);
    CHECK(quantizeInputStart(40, GridTest::fourFourSixteenths, {true, 50, 1920}) == 0);
}
