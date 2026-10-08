#include "audio/MetronomeVolume.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("gain follows the square of the volume", "[audio][metronome]")
{
    CHECK_THAT(MetronomeVolume::gainFor(0), WithinAbs(0.0, 1e-6));
    CHECK_THAT(MetronomeVolume::gainFor(50), WithinAbs(0.25, 1e-6));
    CHECK_THAT(MetronomeVolume::gainFor(70), WithinAbs(0.49, 1e-6));
    CHECK_THAT(MetronomeVolume::gainFor(100), WithinAbs(1.0, 1e-6));
}

TEST_CASE("nudging moves by five and stops at the ends", "[audio][metronome]")
{
    CHECK(MetronomeVolume::nudged(70, 1) == 75);
    CHECK(MetronomeVolume::nudged(70, -1) == 65);
    CHECK(MetronomeVolume::nudged(100, 1) == 100);
    CHECK(MetronomeVolume::nudged(0, -1) == 0);
    CHECK(MetronomeVolume::nudged(98, 1) == 100);
}

TEST_CASE("stored volume is clamped into range", "[audio][metronome]")
{
    CHECK(MetronomeVolume::clamped(250) == 100);
    CHECK(MetronomeVolume::clamped(-3) == 0);
    CHECK(MetronomeVolume::clamped(42) == 42);
}
