#include "model/ControllerKind.h"
#include "model/ControllerLineValue.h"
#include "model/ControllerRamps.h"
#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <vector>

namespace
{
using Events = std::vector<MidiEvent>;

MidiEvent cc(int tick, int number, int value, bool ramp = true)
{
    return {.type = MidiEvent::Type::ControlChange, .tick = tick, .data1 = number, .data2 = value, .ramp = ramp};
}

MidiEvent pb(int tick, int value)
{
    return {.type = MidiEvent::Type::PitchBend, .tick = tick, .data1 = value, .ramp = true};
}

int sentValueAt(const Events& events, int tick)
{
    int value = -1;
    for (const auto& event : expandControllerRamps(events))
        if (event.tick <= tick)
            value = event.data2;
    return value;
}
} // namespace

TEST_CASE("the line has no value before the first point", "[model][ramp]")
{
    CHECK(controllerLineValueAt({cc(480, 1, 64)}, ControllerKind::controlChange(1), 479) == std::nullopt);
}

TEST_CASE("the line holds the last value after the last point", "[model][ramp]")
{
    CHECK(controllerLineValueAt({cc(0, 1, 0), cc(480, 1, 64)}, ControllerKind::controlChange(1), 5000) == 64);
}

TEST_CASE("the line between two points shows the value playback sends", "[model][ramp]")
{
    const Events events{cc(0, 1, 0), cc(1000, 1, 127), cc(1300, 1, 20)};
    for (int tick = 0; tick <= 1300; ++tick)
        CHECK(controllerLineValueAt(events, ControllerKind::controlChange(1), tick) == sentValueAt(events, tick));
}

TEST_CASE("the line shows the last point of a stack on its tick", "[model][ramp]")
{
    const Events events{cc(0, 1, 127), cc(480, 1, 127), cc(480, 1, 10), cc(960, 1, 10)};
    CHECK(controllerLineValueAt(events, ControllerKind::controlChange(1), 480) == 10);
    CHECK(controllerLineValueAt(events, ControllerKind::controlChange(1), 479) == 127);
}

TEST_CASE("the line holds until a point that does not ramp", "[model][ramp]")
{
    CHECK(controllerLineValueAt({cc(0, 1, 0), cc(960, 1, 100, false)}, ControllerKind::controlChange(1), 959) == 0);
    CHECK(controllerLineValueAt({cc(0, 0, 0), cc(960, 0, 100)}, ControllerKind::controlChange(0), 959) == 0);
}

TEST_CASE("the line follows only its own kind", "[model][ramp]")
{
    const Events events{cc(0, 1, 0), cc(240, 7, 100), pb(300, 16383), cc(480, 1, 48)};
    CHECK(controllerLineValueAt(events, ControllerKind::controlChange(1), 240) == 24);
    CHECK(controllerLineValueAt(events, ControllerKind::controlChange(7), 100) == std::nullopt);
}

TEST_CASE("the pitch bend line ramps through the centre", "[model][ramp]")
{
    CHECK(controllerLineValueAt({pb(0, 0), pb(960, 16383)}, ControllerKind::pitchBend(), 480) == 8192);
}
