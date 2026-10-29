#include "ui/pianoroll/VelocityBarGrab.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("a velocity bar is grabbed near its top within its width", "[ui][velocity]")
{
    const std::vector<VelocityBar> bars{{100, 50}, {200, 80}};
    CHECK(VelocityBarGrab::hit(bars, 103, 52, 7, 0, 4) == 0);
    CHECK(VelocityBarGrab::hit(bars, 206, 76, 7, 0, 4) == 1);
}

TEST_CASE("a velocity bar is not grabbed below or above its top or beside it", "[ui][velocity]")
{
    const std::vector<VelocityBar> bars{{100, 50}};
    CHECK(VelocityBarGrab::hit(bars, 103, 60, 7, 0, 4) == -1);
    CHECK(VelocityBarGrab::hit(bars, 103, 40, 7, 0, 4) == -1);
    CHECK(VelocityBarGrab::hit(bars, 107, 50, 7, 0, 4) == -1);
    CHECK(VelocityBarGrab::hit(bars, 99, 50, 7, 0, 4) == -1);
}

TEST_CASE("of bars on one tick the top nearest the mouse is grabbed", "[ui][velocity]")
{
    const std::vector<VelocityBar> bars{{100, 50}, {100, 54}};
    CHECK(VelocityBarGrab::hit(bars, 103, 51, 7, 0, 4) == 0);
    CHECK(VelocityBarGrab::hit(bars, 103, 53, 7, 0, 4) == 1);
}

TEST_CASE("of bars with tops equally near the later one is grabbed", "[ui][velocity]")
{
    CHECK(VelocityBarGrab::hit({{100, 50}, {100, 50}}, 103, 50, 7, 0, 4) == 1);
}

TEST_CASE("a velocity bar is grabbed within the margin on either side", "[ui][velocity]")
{
    const std::vector<VelocityBar> bars{{100, 50}};
    CHECK(VelocityBarGrab::hit(bars, 98, 50, 4, 2, 4) == 0);
    CHECK(VelocityBarGrab::hit(bars, 105, 50, 4, 2, 4) == 0);
    CHECK(VelocityBarGrab::hit(bars, 97, 50, 4, 2, 4) == -1);
    CHECK(VelocityBarGrab::hit(bars, 106, 50, 4, 2, 4) == -1);
}

TEST_CASE("a grabbed velocity keeps its value until the mouse moves", "[ui][velocity]")
{
    CHECK(VelocityBarGrab::draggedValue({.pressY = 52, .velocity = 90}, 52, 127) == 90);
}

TEST_CASE("a grabbed velocity follows the mouse by the distance moved", "[ui][velocity]")
{
    CHECK(VelocityBarGrab::draggedValue({.pressY = 52, .velocity = 90}, 42, 127) == 100);
    CHECK(VelocityBarGrab::draggedValue({.pressY = 52, .velocity = 90}, 72, 254) == 80);
}

TEST_CASE("a grabbed velocity is kept in range", "[ui][velocity]")
{
    CHECK(VelocityBarGrab::draggedValue({.pressY = 52, .velocity = 120}, 0, 127) == 127);
    CHECK(VelocityBarGrab::draggedValue({.pressY = 52, .velocity = 10}, 200, 127) == 0);
}
