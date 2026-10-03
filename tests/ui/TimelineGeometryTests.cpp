#include "ui/pianoroll/TimelineGeometry.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
TimelineGeometry makeGeometry(int quantizeDenominator = 4)
{
    TimelineGeometry geometry(100);
    geometry.setTicksPerQuarterNote(480);
    geometry.setBeatWidth(80);
    geometry.setQuantizeDenominator(quantizeDenominator);
    return geometry;
}
} // namespace

TEST_CASE("ticks map to x from the timeline start", "[ui][geometry]")
{
    const auto geometry = makeGeometry();
    CHECK(geometry.tickToX(0) == 100);
    CHECK(geometry.tickToX(240) == 140);
    CHECK(geometry.tickToX(1920) == 420);
}

TEST_CASE("x maps back to ticks", "[ui][geometry]")
{
    const auto geometry = makeGeometry();
    CHECK(geometry.xToTick(100) == 0);
    CHECK(geometry.xToTick(140) == 240);
    CHECK(geometry.xToTick(420) == 1920);
}

TEST_CASE("durations map to widths", "[ui][geometry]")
{
    const auto geometry = makeGeometry();
    CHECK(geometry.tickToWidth(480) == 80);
    CHECK(geometry.tickToWidth(120) == 20);
}

TEST_CASE("the grid follows the resolution and the quantize value", "[ui][geometry]")
{
    CHECK(makeGeometry(1).gridTicks() == 1920);
    CHECK(makeGeometry(4).gridTicks() == 480);
    CHECK(makeGeometry(16).gridTicks() == 120);

    auto fine = makeGeometry(8);
    fine.setTicksPerQuarterNote(960);
    CHECK(fine.gridTicks() == 480);
}

TEST_CASE("ticks round to the nearest grid line and halfway rounds up", "[ui][geometry]")
{
    const auto geometry = makeGeometry();
    CHECK(geometry.roundTickToGrid(0) == 0);
    CHECK(geometry.roundTickToGrid(239) == 0);
    CHECK(geometry.roundTickToGrid(240) == 480);
    CHECK(geometry.roundTickToGrid(719) == 480);
    CHECK(geometry.roundTickToGrid(720) == 960);
}

TEST_CASE("ticks floor to the grid line at or before them", "[ui][geometry]")
{
    const auto geometry = makeGeometry();
    CHECK(geometry.floorTickToGrid(0) == 0);
    CHECK(geometry.floorTickToGrid(479) == 0);
    CHECK(geometry.floorTickToGrid(480) == 480);
    CHECK(geometry.floorTickToGrid(959) == 480);
}

TEST_CASE("without a resolution positions stay at the timeline start and the grid is 480 ticks", "[ui][geometry]")
{
    TimelineGeometry geometry(100);
    CHECK(geometry.tickToX(960) == 100);
    CHECK(geometry.tickToWidth(960) == 0);
    CHECK(geometry.xToTick(500) == 0);
    CHECK(geometry.gridTicks() == 480);
}
