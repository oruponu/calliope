#include "edit/time/LineCrossings.h"
#include "support/MidiEventTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
MidiEvent cc(int tick, int number, int value, bool ramp = true)
{
    return {MidiEvent::Type::ControlChange, tick, number, value, ramp};
}

MidiEvent pb(int tick, int value)
{
    return {MidiEvent::Type::PitchBend, tick, value, 0, true};
}

LineCrossing crossing(const MidiEvent& point, bool nextAtTick)
{
    return {point, nextAtTick};
}
} // namespace

TEST_CASE("a line through the tick gives its value there", "[time][line]")
{
    const auto result = lineCrossings({cc(0, 7, 0), cc(3840, 7, 100)}, 1920, false);

    CHECK(result == std::vector<LineCrossing>{crossing(cc(1920, 7, 50), false)});
}

TEST_CASE("a line ending at the tick is marked", "[time][line]")
{
    const auto result = lineCrossings({cc(0, 7, 0), cc(1920, 7, 100)}, 1920, false);

    CHECK(result == std::vector<LineCrossing>{crossing(cc(1920, 7, 100), true)});
}

TEST_CASE("from the tick, a point at the tick starts the line", "[time][line]")
{
    const auto result = lineCrossings({cc(0, 7, 0), cc(1920, 7, 100)}, 0, true);

    CHECK(result == std::vector<LineCrossing>{crossing(cc(0, 7, 0), false)});
}

TEST_CASE("flat lines and steps are not crossed", "[time][line]")
{
    const auto result =
        lineCrossings({cc(0, 7, 40), cc(3840, 7, 40), cc(0, 10, 0), cc(3840, 10, 100, false)}, 1920, false);

    CHECK(result.empty());
}

TEST_CASE("pitch bend lines are crossed", "[time][line]")
{
    const auto result = lineCrossings({pb(0, 0), pb(3840, 16000)}, 1920, false);

    CHECK(result == std::vector<LineCrossing>{crossing(pb(1920, 8000), false)});
}

TEST_CASE("each kind is crossed in kind order", "[time][line]")
{
    const auto result = lineCrossings({cc(0, 10, 0), cc(3840, 10, 100), cc(0, 7, 0), cc(3840, 7, 100)}, 1920, false);

    CHECK(result == std::vector<LineCrossing>{crossing(cc(1920, 7, 50), false), crossing(cc(1920, 10, 50), false)});
}
