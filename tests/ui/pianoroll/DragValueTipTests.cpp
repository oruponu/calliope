#include "ui/pianoroll/DragValueTip.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("a drag tip counts bars and beats from 1 without padding", "[ui][drag-tip]")
{
    CHECK(DragValueTip::formatPosition({1, 1, 0}) == "1.1.0");
    CHECK(DragValueTip::formatPosition({12, 3, 240}) == "12.3.240");
}

TEST_CASE("a drag tip sits past the mouse when it fits", "[ui][drag-tip]")
{
    CHECK(DragValueTip::placeAfter(100, 50, 8, 0, 400) == 108);
    CHECK(DragValueTip::placeBefore(60, 30, 8, 0, 100) == 22);
}

TEST_CASE("a drag tip flips to the other side of the mouse at the edge of the area", "[ui][drag-tip]")
{
    CHECK(DragValueTip::placeAfter(380, 50, 8, 0, 400) == 322);
    CHECK(DragValueTip::placeBefore(10, 30, 8, 0, 100) == 18);
}

TEST_CASE("a drag tip that fits on neither side of the mouse stays inside the area", "[ui][drag-tip]")
{
    CHECK(DragValueTip::placeBefore(20, 30, 8, 0, 45) == 15);
    CHECK(DragValueTip::placeAfter(20, 50, 8, 0, 40) == 0);
}
