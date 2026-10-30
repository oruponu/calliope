#include "edit/controller/ChannelEventEdits.h"
#include "support/ControllerPointTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace ControllerPointTest;

namespace
{
MidiEvent pressure(int tick, int value)
{
    return {.type = MidiEvent::Type::ChannelPressure, .tick = tick, .data1 = value};
}

MidiEvent program(int tick, int number)
{
    return {.type = MidiEvent::Type::ProgramChange, .tick = tick, .data1 = number};
}

MidiEvent sustain(int tick, int value)
{
    return {.type = MidiEvent::Type::ControlChange, .tick = tick, .data1 = 64, .data2 = value};
}

MidiEvent keyPressure(int tick, int note, int value)
{
    return {.type = MidiEvent::Type::KeyPressure, .tick = tick, .data1 = note, .data2 = value};
}

MidiEvent bend(int tick, int value)
{
    return {.type = MidiEvent::Type::PitchBend, .tick = tick, .data1 = value, .ramp = true};
}
} // namespace

TEST_CASE("a moved event goes to its tick in order", "[edit][controller]")
{
    CHECK(ChannelEventEdits::moveTo({pressure(0, 10), pressure(480, 20), pressure(960, 30)}, 0, 720) ==
          result({pressure(480, 20), pressure(720, 10), pressure(960, 30)}, {1}));
    CHECK(ChannelEventEdits::moveTo({pressure(0, 10), pressure(480, 20), pressure(960, 30)}, 2, 240) ==
          result({pressure(0, 10), pressure(240, 30), pressure(480, 20)}, {1}));
}

TEST_CASE("a moved event goes after the events already on its tick", "[edit][controller]")
{
    CHECK(ChannelEventEdits::moveTo({pressure(0, 10), program(480, 5), pressure(480, 20)}, 0, 480) ==
          result({program(480, 5), pressure(480, 20), pressure(480, 10)}, {2}));
}

TEST_CASE("a moved event stays at or after tick 0", "[edit][controller]")
{
    CHECK(ChannelEventEdits::moveTo({program(480, 5)}, 0, -100) == result({program(0, 5)}, {0}));
}

TEST_CASE("an event moved past others leaves them in place", "[edit][controller]")
{
    CHECK(ChannelEventEdits::moveTo({sustain(0, 10), sustain(480, 20), sustain(960, 30)}, 0, 1440) ==
          result({sustain(480, 20), sustain(960, 30), sustain(1440, 10)}, {2}));
    CHECK(ChannelEventEdits::moveTo({p(0, 10), p(480, 20), p(960, 30)}, 0, 1440) ==
          result({p(480, 20), p(960, 30), p(1440, 10)}, {2}));
}

TEST_CASE("a moved step takes a hold point at the value before its new tick", "[edit][controller]")
{
    CHECK(ChannelEventEdits::moveTo({p(0, 10), p(480, 10), p(480, 50), p(960, 30)}, 2, 1440) ==
          result({p(0, 10), p(960, 30), p(1440, 30), p(1440, 50)}, {3}));
}

TEST_CASE("the step after a moved event holds the value now before it", "[edit][controller]")
{
    CHECK(ChannelEventEdits::moveTo({p(0, 10), p(480, 40), p(960, 40), p(960, 70)}, 1, 120) ==
          result({p(0, 10), p(120, 40), p(960, 40), p(960, 70)}, {1}));
    CHECK(ChannelEventEdits::moveTo({p(0, 10), p(480, 10), p(480, 40), p(960, 40), p(960, 70)}, 2, 1440) ==
          result({p(0, 10), p(960, 10), p(960, 70), p(1440, 70), p(1440, 40)}, {4}));
}

TEST_CASE("a hold point or an index out of range is not moved", "[edit][controller]")
{
    const Events events{p(0, 10), p(480, 10), p(480, 50)};
    CHECK(ChannelEventEdits::moveTo(events, 1, 960) == result(events, {}));
    CHECK(ChannelEventEdits::moveTo(events, 3, 960) == result(events, {}));
    CHECK(ChannelEventEdits::moveTo(events, -1, 960) == result(events, {}));
}

TEST_CASE("a set value goes into the data byte that holds the event's value", "[edit][controller]")
{
    CHECK(ChannelEventEdits::setValue({pressure(0, 10), pressure(480, 20)}, 1, 30) ==
          result({pressure(0, 10), pressure(480, 30)}, {1}));
    CHECK(ChannelEventEdits::setValue({program(0, 5)}, 0, 7) == result({program(0, 7)}, {0}));
    CHECK(ChannelEventEdits::setValue({sustain(0, 0)}, 0, 127) == result({sustain(0, 127)}, {0}));
    CHECK(ChannelEventEdits::setValue({keyPressure(0, 60, 10)}, 0, 90) == result({keyPressure(0, 60, 90)}, {0}));
    CHECK(ChannelEventEdits::setValue({bend(0, 8192)}, 0, 16383) == result({bend(0, 16383)}, {0}));
}

TEST_CASE("the hold point before the next step holds the set value", "[edit][controller]")
{
    CHECK(ChannelEventEdits::setValue({p(0, 10), p(480, 10), p(480, 50)}, 0, 20) ==
          result({p(0, 20), p(480, 20), p(480, 50)}, {0}));
}

TEST_CASE("a step set to the value before it needs no hold point", "[edit][controller]")
{
    CHECK(ChannelEventEdits::setValue({p(0, 10), p(480, 10), p(480, 50)}, 2, 10) ==
          result({p(0, 10), p(480, 10)}, {1}));
}

TEST_CASE("a hold point or an index out of range takes no value", "[edit][controller]")
{
    const Events events{p(0, 10), p(480, 10), p(480, 50)};
    CHECK(ChannelEventEdits::setValue(events, 1, 30) == result(events, {}));
    CHECK(ChannelEventEdits::setValue(events, 3, 30) == result(events, {}));
}
