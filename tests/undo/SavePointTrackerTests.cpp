#include "undo/SavePointTracker.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("a new tracker is at the save point", "[undo][savepoint]")
{
    SavePointTracker tracker;
    CHECK(tracker.isAtSavePoint());
}

TEST_CASE("performing a transaction leaves the save point", "[undo][savepoint]")
{
    SavePointTracker tracker;
    tracker.transactionPerformed();
    CHECK_FALSE(tracker.isAtSavePoint());
}

TEST_CASE("undoing back to the save point returns to it and redoing leaves it", "[undo][savepoint]")
{
    SavePointTracker tracker;
    tracker.transactionPerformed();
    tracker.undone();
    CHECK(tracker.isAtSavePoint());
    tracker.redone();
    CHECK_FALSE(tracker.isAtSavePoint());
}

TEST_CASE("redoing back to the save point returns to it", "[undo][savepoint]")
{
    SavePointTracker tracker;
    tracker.transactionPerformed();
    tracker.markSaved();
    tracker.undone();
    CHECK_FALSE(tracker.isAtSavePoint());
    tracker.redone();
    CHECK(tracker.isAtSavePoint());
}

TEST_CASE("a new transaction after undoing past the save point loses it", "[undo][savepoint]")
{
    SavePointTracker tracker;
    tracker.transactionPerformed();
    tracker.markSaved();
    tracker.undone();
    tracker.transactionPerformed();
    CHECK_FALSE(tracker.isAtSavePoint());
    tracker.undone();
    CHECK_FALSE(tracker.isAtSavePoint());
    tracker.transactionPerformed();
    CHECK_FALSE(tracker.isAtSavePoint());
}

TEST_CASE("appending to the saved transaction loses the save point", "[undo][savepoint]")
{
    SavePointTracker tracker;
    tracker.transactionPerformed();
    tracker.markSaved();
    tracker.actionAppended();
    CHECK_FALSE(tracker.isAtSavePoint());
    tracker.undone();
    CHECK_FALSE(tracker.isAtSavePoint());
}

TEST_CASE("appending to a later transaction keeps the save point reachable", "[undo][savepoint]")
{
    SavePointTracker tracker;
    tracker.transactionPerformed();
    tracker.markSaved();
    tracker.transactionPerformed();
    tracker.actionAppended();
    CHECK_FALSE(tracker.isAtSavePoint());
    tracker.undone();
    CHECK(tracker.isAtSavePoint());
}

TEST_CASE("clearing loses the save point until saved again", "[undo][savepoint]")
{
    SavePointTracker tracker;
    tracker.transactionPerformed();
    tracker.markSaved();
    tracker.cleared();
    CHECK_FALSE(tracker.isAtSavePoint());
    tracker.markSaved();
    CHECK(tracker.isAtSavePoint());
}

TEST_CASE("saving again moves the save point", "[undo][savepoint]")
{
    SavePointTracker tracker;
    tracker.transactionPerformed();
    tracker.markSaved();
    tracker.transactionPerformed();
    tracker.markSaved();
    CHECK(tracker.isAtSavePoint());
    tracker.undone();
    CHECK_FALSE(tracker.isAtSavePoint());
    tracker.redone();
    CHECK(tracker.isAtSavePoint());
}
