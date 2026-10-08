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
