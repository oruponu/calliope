#include "model/timeline/TimelineMap.h"
#include "ui/time/TimeRangeInput.h"
#include <catch2/catch_test_macros.hpp>
#include <string_view>

TEST_CASE("bar, beat and tick are read as whole numbers", "[time][input]")
{
    const auto position = TimeRangeInput::parse("3", "2", "120");
    REQUIRE(position.has_value());
    CHECK(position->bar == 3);
    CHECK(position->beat == 2);
    CHECK(position->tick == 120);

    const auto padded = TimeRangeInput::parse(" 4 ", "1", "0");
    REQUIRE(padded.has_value());
    CHECK(padded->bar == 4);
    CHECK(padded->beat == 1);
    CHECK(padded->tick == 0);
}

TEST_CASE("zero, empty and non-numeric fields are refused", "[time][input]")
{
    for (const std::string_view text : {"0", "", "a", "-1", "1.5"})
    {
        CAPTURE(text);
        CHECK_FALSE(TimeRangeInput::parse(text, "1", "0").has_value());
        CHECK_FALSE(TimeRangeInput::parse("1", text, "0").has_value());
    }
    for (const std::string_view text : {"", "a", "-1"})
    {
        CAPTURE(text);
        CHECK_FALSE(TimeRangeInput::parse("1", "1", text).has_value());
    }
    CHECK(TimeRangeInput::parse("1", "1", "0").has_value());
}

TEST_CASE("the bar stops at the largest bar and beat and tick stop at int", "[time][input]")
{
    CHECK(TimeRangeInput::parse("9999", "1", "0").has_value());
    CHECK_FALSE(TimeRangeInput::parse("10000", "1", "0").has_value());
    CHECK(TimeRangeInput::parse("1", "2147483647", "0").has_value());
    CHECK_FALSE(TimeRangeInput::parse("1", "2147483648", "0").has_value());
    CHECK(TimeRangeInput::parse("1", "1", "2147483647").has_value());
    CHECK_FALSE(TimeRangeInput::parse("1", "1", "2147483648").has_value());
}

TEST_CASE("the initial end is kept inside the next bar", "[time][input]")
{
    TimelineMap timeline;
    timeline.setTimeSignatureChanges({{0, 4, 4}, {1920, 3, 4}});

    const auto clipped = TimeRangeInput::initialEnd({1, 4, 100}, timeline);
    CHECK(clipped.bar == 2);
    CHECK(clipped.beat == 3);
    CHECK(clipped.tick == 100);

    const auto kept = TimeRangeInput::initialEnd({1, 2, 30}, timeline);
    CHECK(kept.bar == 2);
    CHECK(kept.beat == 2);
    CHECK(kept.tick == 30);

    TimelineMap plain;
    plain.setTimeSignatureChanges({{0, 4, 4}});
    const auto later = TimeRangeInput::initialEnd({5, 1, 0}, plain);
    CHECK(later.bar == 6);
    CHECK(later.beat == 1);
    CHECK(later.tick == 0);
}

TEST_CASE("the initial start is kept inside the largest bar", "[time][input]")
{
    TimelineMap timeline;
    timeline.setTimeSignatureChanges({{0, 4, 4}});

    const auto inside = TimeRangeInput::initialStart(4 * 1920 + 480 + 7, timeline);
    CHECK(inside.bar == 5);
    CHECK(inside.beat == 2);
    CHECK(inside.tick == 7);

    const auto beyond = TimeRangeInput::initialStart(10000 * 1920 + 480 + 7, timeline);
    CHECK(beyond.bar == 9999);
    CHECK(beyond.beat == 2);
    CHECK(beyond.tick == 7);
}

TEST_CASE("a typed position is kept inside its bar", "[time][input]")
{
    TimelineMap timeline;
    timeline.setTimeSignatureChanges({{0, 4, 4}});

    const auto beat = TimeRangeInput::keptInside({1, 7, 0}, timeline);
    CHECK(beat.bar == 1);
    CHECK(beat.beat == 4);
    CHECK(beat.tick == 0);

    const auto tick = TimeRangeInput::keptInside({2, 1, 480}, timeline);
    CHECK(tick.bar == 2);
    CHECK(tick.beat == 1);
    CHECK(tick.tick == 479);

    const auto inside = TimeRangeInput::keptInside({3, 2, 10}, timeline);
    CHECK(inside.bar == 3);
    CHECK(inside.beat == 2);
    CHECK(inside.tick == 10);
}
