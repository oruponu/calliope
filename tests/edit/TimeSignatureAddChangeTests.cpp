#include "support/TimeSignatureTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace timesigtest;

TEST_CASE("changing the first time signature shifts the later ones by bars", "[timesig][add-change]")
{
    auto changes = timeSigs({{1, 4, 4}, {5, 3, 4}});
    TimeSignatureEdits::add(changes, 0, 3, 4, ppq);
    CHECK(changes == timeSigs({{1, 3, 4}, {5, 3, 4}}));
}

TEST_CASE("a time signature added mid-bar lands on its bar start", "[timesig][add-change]")
{
    auto changes = timeSigs({{1, 4, 4}});
    TimeSignatureEdits::add(changes, 2500, 3, 4, ppq);
    CHECK(changes == timeSigs({{1, 4, 4}, {2, 3, 4}}));
}

TEST_CASE("inserting a time signature keeps the later ones on their bars", "[timesig][add-change]")
{
    auto changes = timeSigs({{1, 4, 4}, {5, 3, 4}});
    TimeSignatureEdits::add(changes, 3840, 6, 8, ppq);
    CHECK(changes == timeSigs({{1, 4, 4}, {3, 6, 8}, {5, 3, 4}}));
}

TEST_CASE("a time signature after the last change is appended on its bar", "[timesig][add-change]")
{
    auto changes = timeSigs({{1, 4, 4}, {5, 3, 4}});
    TimeSignatureEdits::add(changes, 10560, 2, 4, ppq);
    CHECK(changes == timeSigs({{1, 4, 4}, {5, 3, 4}, {7, 2, 4}}));
}

TEST_CASE("adding moves a first time signature that is not at tick 0 to tick 0", "[timesig][add-change]")
{
    std::vector<TimeSignatureChange> changes{{1920, 3, 4}};
    TimeSignatureEdits::add(changes, 3840, 2, 4, ppq);
    CHECK(changes == std::vector<TimeSignatureChange>{{0, 3, 4}, {1440, 2, 4}});
}

TEST_CASE("added time signatures follow the sequence resolution", "[timesig][add-change]")
{
    std::vector<TimeSignatureChange> changes{{0, 4, 4}};
    TimeSignatureEdits::add(changes, 4000, 3, 4, 960);
    CHECK(changes == std::vector<TimeSignatureChange>{{0, 4, 4}, {3840, 3, 4}});
}
