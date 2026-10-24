#include "model/ControllerRamps.h"
#include "support/MidiEventTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
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

MidiEvent pc(int tick, int program)
{
    return {.type = MidiEvent::Type::ProgramChange, .tick = tick, .data1 = program};
}
} // namespace

TEST_CASE("events without ramps come out unchanged", "[model][ramp]")
{
    const Events events{cc(0, 7, 100), pb(10, 9000), cc(10, 1, 5), pc(20, 3)};
    CHECK(expandControllerRamps(events) == events);
}

TEST_CASE("a ramp point fills in a value at each tick where the value changes", "[model][ramp]")
{
    CHECK(expandControllerRamps({cc(0, 1, 0), cc(4, 1, 4, true)}) ==
          Events{cc(0, 1, 0), cc(1, 1, 1), cc(2, 1, 2), cc(3, 1, 3), cc(4, 1, 4)});
}

TEST_CASE("values that do not change are not repeated but the point is always sent", "[model][ramp]")
{
    CHECK(expandControllerRamps({cc(0, 1, 0), cc(10, 1, 2, true)}) ==
          Events{cc(0, 1, 0), cc(3, 1, 1), cc(8, 1, 2), cc(10, 1, 2)});
}

TEST_CASE("a ramp can go down", "[model][ramp]")
{
    CHECK(expandControllerRamps({cc(0, 1, 4), cc(4, 1, 0, true)}) ==
          Events{cc(0, 1, 4), cc(1, 1, 3), cc(2, 1, 2), cc(3, 1, 1), cc(4, 1, 0)});
}

TEST_CASE("a ramp point without an earlier point of its kind is sent as it is", "[model][ramp]")
{
    CHECK(expandControllerRamps({cc(0, 7, 100), cc(4, 1, 10, true)}) == Events{cc(0, 7, 100), cc(4, 1, 10)});
}

TEST_CASE("two points on one tick switch the value at that tick", "[model][ramp]")
{
    CHECK(expandControllerRamps({cc(0, 1, 0), cc(2, 1, 2, true), cc(2, 1, 0, true)}) ==
          Events{cc(0, 1, 0), cc(1, 1, 1), cc(2, 1, 2), cc(2, 1, 0)});
}

TEST_CASE("a ramp follows the previous point of the same controller only", "[model][ramp]")
{
    CHECK(expandControllerRamps({cc(0, 1, 0), cc(0, 7, 100), cc(2, 1, 2, true)}) ==
          Events{cc(0, 1, 0), cc(0, 7, 100), cc(1, 1, 1), cc(2, 1, 2)});
}

TEST_CASE("a ramp across a very long span fills in only where the value changes", "[model][ramp]")
{
    CHECK(expandControllerRamps({cc(0, 1, 0), cc(2000000000, 1, 2, true)}) ==
          Events{cc(0, 1, 0), cc(500000000, 1, 1), cc(1500000000, 1, 2), cc(2000000000, 1, 2)});
}

TEST_CASE("a steep ramp sends the last value reached on each tick", "[model][ramp]")
{
    CHECK(expandControllerRamps({pb(0, 0), pb(4, 16383, true)}) ==
          Events{pb(0, 0), pb(1, 4096), pb(2, 8192), pb(3, 12287), pb(4, 16383)});
}

TEST_CASE("pitch bend ramps across its full range", "[model][ramp]")
{
    CHECK(expandControllerRamps({pb(0, 0), pb(2, 16383, true)}) == Events{pb(0, 0), pb(1, 8192), pb(2, 16383)});
}

TEST_CASE("controllers that must not ramp are stepped", "[model][ramp]")
{
    const int number = GENERATE(0, 6, 32, 38, 64, 69, 96, 101, 120, 127);
    CHECK(expandControllerRamps({cc(0, number, 0), cc(4, number, 4, true)}) ==
          Events{cc(0, number, 0), cc(4, number, 4)});
}

TEST_CASE("events out of tick order are expanded in tick order", "[model][ramp]")
{
    CHECK(expandControllerRamps({cc(4, 1, 4, true), cc(0, 1, 0)}) ==
          Events{cc(0, 1, 0), cc(1, 1, 1), cc(2, 1, 2), cc(3, 1, 3), cc(4, 1, 4)});
}

TEST_CASE("filled-in values sit after other events on the same tick", "[model][ramp]")
{
    CHECK(expandControllerRamps({cc(0, 1, 0), pc(2, 5), cc(4, 1, 4, true)}) ==
          Events{cc(0, 1, 0), cc(1, 1, 1), pc(2, 5), cc(2, 1, 2), cc(3, 1, 3), cc(4, 1, 4)});
}
