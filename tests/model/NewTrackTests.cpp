#include "model/NewTrack.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("a new track is named after the row it is added at", "[model][track]")
{
    CHECK(NewTrack::make(0).getName() == "Track 1");
    CHECK(NewTrack::make(2).getName() == "Track 3");
}
