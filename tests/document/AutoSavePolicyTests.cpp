#include "document/AutoSavePolicy.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
AutoSaveInputs inputs(double now, std::uint64_t revision, bool unsaved, bool busy)
{
    return {now, revision, unsaved, busy};
}
} // namespace

TEST_CASE("nothing happens before activation", "[document][autosave]")
{
    AutoSavePolicy policy;

    CHECK(policy.update(inputs(1000, 5, true, false)) == AutoSaveAction::none);
    CHECK_FALSE(policy.isActive());
}

TEST_CASE("nothing is written before five minutes pass after the first change", "[document][autosave]")
{
    AutoSavePolicy policy;
    policy.activate(0, false);

    CHECK(policy.update(inputs(0, 1, true, false)) == AutoSaveAction::none);
    CHECK(policy.update(inputs(299, 1, true, false)) == AutoSaveAction::none);
}

TEST_CASE("the recovery file is written five minutes after the first change", "[document][autosave]")
{
    AutoSavePolicy policy;
    policy.activate(0, false);
    policy.update(inputs(0, 1, true, false));
    policy.update(inputs(299, 1, true, false));

    CHECK(policy.update(inputs(300, 1, true, false)) == AutoSaveAction::write);
}

TEST_CASE("the five minutes count from the first change, not the last", "[document][autosave]")
{
    AutoSavePolicy policy;
    policy.activate(0, false);

    policy.update(inputs(0, 1, true, false));
    policy.update(inputs(200, 2, true, false));

    CHECK(policy.update(inputs(300, 3, true, false)) == AutoSaveAction::write);
}

TEST_CASE("a write held back while busy happens as soon as it is no longer busy", "[document][autosave]")
{
    AutoSavePolicy policy;
    policy.activate(0, false);
    policy.update(inputs(0, 1, true, false));

    CHECK(policy.update(inputs(300, 1, true, true)) == AutoSaveAction::none);
    CHECK(policy.update(inputs(900, 1, true, true)) == AutoSaveAction::none);
    CHECK(policy.update(inputs(901, 1, true, false)) == AutoSaveAction::write);
}

TEST_CASE("nothing is written again when nothing changed since the last write", "[document][autosave]")
{
    AutoSavePolicy policy;
    policy.activate(0, false);
    policy.update(inputs(0, 1, true, false));
    REQUIRE(policy.update(inputs(300, 1, true, false)) == AutoSaveAction::write);
    policy.wrote(1);

    CHECK(policy.update(inputs(1000, 1, true, false)) == AutoSaveAction::none);
}

TEST_CASE("a written file is removed when the song returns to its saved state", "[document][autosave]")
{
    AutoSavePolicy policy;
    policy.activate(0, false);
    policy.update(inputs(0, 1, true, false));
    REQUIRE(policy.update(inputs(300, 1, true, false)) == AutoSaveAction::write);
    policy.wrote(1);

    CHECK(policy.update(inputs(310, 2, false, false)) == AutoSaveAction::remove);
    policy.removed();
    CHECK(policy.update(inputs(320, 2, false, false)) == AutoSaveAction::none);
}

TEST_CASE("no removal is asked when no recovery file exists", "[document][autosave]")
{
    AutoSavePolicy policy;
    policy.activate(0, false);

    CHECK(policy.update(inputs(10, 0, false, false)) == AutoSaveAction::none);
}

TEST_CASE("a failed write waits another five minutes before retrying", "[document][autosave]")
{
    AutoSavePolicy policy;
    policy.activate(0, false);
    policy.update(inputs(0, 1, true, false));
    REQUIRE(policy.update(inputs(300, 1, true, false)) == AutoSaveAction::write);
    policy.writeFailed(300);

    CHECK(policy.update(inputs(301, 1, true, false)) == AutoSaveAction::none);
    CHECK(policy.update(inputs(599, 1, true, false)) == AutoSaveAction::none);
    CHECK(policy.update(inputs(600, 1, true, false)) == AutoSaveAction::write);
}

TEST_CASE("a recovered song is not written or removed until it changes again", "[document][autosave]")
{
    AutoSavePolicy policy;
    policy.activate(7, true);

    CHECK(policy.update(inputs(1000, 7, true, false)) == AutoSaveAction::none);
    CHECK(policy.update(inputs(1000, 8, true, false)) == AutoSaveAction::none);
    CHECK(policy.update(inputs(1300, 8, true, false)) == AutoSaveAction::write);
}

TEST_CASE("activating again forgets the previous song's first change", "[document][autosave]")
{
    AutoSavePolicy policy;
    policy.activate(0, false);
    policy.update(inputs(0, 1, true, false));
    policy.activate(5, false);

    CHECK(policy.update(inputs(300, 6, true, false)) == AutoSaveAction::none);
    CHECK(policy.update(inputs(599, 6, true, false)) == AutoSaveAction::none);
    CHECK(policy.update(inputs(600, 6, true, false)) == AutoSaveAction::write);
}

TEST_CASE("a removal that failed is asked again", "[document][autosave]")
{
    AutoSavePolicy policy;
    policy.activate(0, false);
    policy.update(inputs(0, 1, true, false));
    REQUIRE(policy.update(inputs(300, 1, true, false)) == AutoSaveAction::write);
    policy.wrote(1);

    CHECK(policy.update(inputs(310, 2, false, false)) == AutoSaveAction::remove);
    CHECK(policy.update(inputs(320, 2, false, false)) == AutoSaveAction::remove);
}
