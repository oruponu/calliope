#include "model/ControllerPointConversion.h"
#include "model/ControllerRamps.h"
#include "support/MidiEventTestHelpers.h"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <vector>

namespace
{
using Events = std::vector<MidiEvent>;

MidiEvent cc(int tick, int number, int value, bool ramp = false)
{
    return {.type = MidiEvent::Type::ControlChange, .tick = tick, .data1 = number, .data2 = value, .ramp = ramp};
}

MidiEvent pb(int tick, int value, bool ramp = false)
{
    return {.type = MidiEvent::Type::PitchBend, .tick = tick, .data1 = value, .ramp = ramp};
}
} // namespace

TEST_CASE("a step becomes a hold point and a ramp point", "[model][ramp]")
{
    CHECK(convertStepsToPoints({cc(0, 11, 50), cc(1920, 11, 100)}) ==
          Events{cc(0, 11, 50, true), cc(1920, 11, 50, true), cc(1920, 11, 100, true)});
}

TEST_CASE("a step one tick after the previous event needs no hold point", "[model][ramp]")
{
    CHECK(convertStepsToPoints({cc(0, 11, 50), cc(1, 11, 100)}) == Events{cc(0, 11, 50, true), cc(1, 11, 100, true)});
}

TEST_CASE("a step across the whole int range still gets a hold point", "[model][ramp]")
{
    constexpr int first = std::numeric_limits<int>::min();
    constexpr int last = std::numeric_limits<int>::max();
    CHECK(convertStepsToPoints({cc(first, 11, 50), cc(last, 11, 100)}) ==
          Events{cc(first, 11, 50, true), cc(last, 11, 50, true), cc(last, 11, 100, true)});
}

TEST_CASE("an unchanged value needs no hold point", "[model][ramp]")
{
    CHECK(convertStepsToPoints({cc(0, 11, 50), cc(960, 11, 50)}) == Events{cc(0, 11, 50, true), cc(960, 11, 50, true)});
}

TEST_CASE("controllers that must not ramp stay steps", "[model][ramp]")
{
    const Events events{cc(0, 0, 1), cc(960, 0, 2), cc(0, 101, 0), cc(960, 101, 127)};
    CHECK(convertStepsToPoints(events) == Events{cc(0, 0, 1), cc(0, 101, 0), cc(960, 0, 2), cc(960, 101, 127)});
}

TEST_CASE("converting twice changes nothing", "[model][ramp]")
{
    const auto once = convertStepsToPoints({cc(0, 11, 50), cc(1920, 11, 100), pb(480, 0), pb(960, 8192)});
    CHECK(convertStepsToPoints(once) == once);
}

TEST_CASE("a step holds the previous value of the same controller only", "[model][ramp]")
{
    CHECK(convertStepsToPoints({cc(0, 11, 50), cc(480, 7, 100), cc(960, 11, 100)}) ==
          Events{cc(0, 11, 50, true), cc(480, 7, 100, true), cc(960, 11, 50, true), cc(960, 11, 100, true)});
}

TEST_CASE("a pitch bend step becomes a hold point and a ramp point", "[model][ramp]")
{
    CHECK(convertStepsToPoints({pb(0, 8192), pb(480, 0)}) ==
          Events{pb(0, 8192, true), pb(480, 8192, true), pb(480, 0, true)});
}

TEST_CASE("a pedal pressed again on one tick keeps its order", "[model][ramp]")
{
    CHECK(convertStepsToPoints({cc(0, 64, 127), cc(960, 64, 0), cc(960, 64, 127)}) ==
          Events{cc(0, 64, 127, true), cc(960, 64, 127, true), cc(960, 64, 0, true), cc(960, 64, 127, true)});
}

TEST_CASE("converted steps play and export as the original events", "[model][ramp]")
{
    const Events steps{cc(0, 11, 50),  cc(480, 7, 100), cc(1920, 11, 100), cc(1921, 11, 90),
                       cc(0, 64, 127), cc(960, 64, 0),  cc(960, 64, 127),  pb(0, 8192),
                       pb(240, 0),     pb(240, 16383),  cc(0, 0, 1),       cc(960, 0, 2)};
    auto sorted = steps;
    std::ranges::stable_sort(sorted, {}, &MidiEvent::tick);
    CHECK(expandControllerRamps(convertStepsToPoints(steps)) == sorted);
}

TEST_CASE("a point overwritten on its own tick without a change is not sent", "[model][ramp]")
{
    CHECK(unsentControllerPoints({cc(960, 11, 50, true), cc(0, 11, 50, true), cc(960, 11, 100, true)}) ==
          std::vector<bool>{true, false, false});
}

TEST_CASE("a point that changes the value is sent even when overwritten on its tick", "[model][ramp]")
{
    CHECK(unsentControllerPoints({cc(0, 64, 127, true), cc(960, 64, 0, true), cc(960, 64, 127, true)}) ==
          std::vector<bool>{false, false, false});
}

TEST_CASE("a hold point and the switch after it go back to a step", "[model][ramp]")
{
    CHECK(convertHoldsToSteps(
              {cc(0, 11, 50, true), cc(960, 11, 50, true), cc(960, 11, 100, true), cc(1920, 11, 0, true)}) ==
          Events{cc(0, 11, 50, true), cc(960, 11, 100), cc(1920, 11, 0, true)});
}

TEST_CASE("the top of a drawn drop is not taken for a hold point", "[model][ramp]")
{
    const Events drop{cc(0, 11, 0, true), cc(960, 11, 127, true), cc(960, 11, 0, true)};
    CHECK(convertHoldsToSteps(drop) == drop);
}
