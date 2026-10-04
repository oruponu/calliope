#include "undo/UndoHistory.h"
#include <catch2/catch_test_macros.hpp>
#include <juce_data_structures/juce_data_structures.h>
#include <utility>
#include <vector>

namespace
{
class IncrementAction : public juce::UndoableAction
{
public:
    explicit IncrementAction(int& valueRef) : value(valueRef) {}

    bool perform() override
    {
        ++value;
        return true;
    }

    bool undo() override
    {
        --value;
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    int& value;
};

class RejectedAction : public juce::UndoableAction
{
public:
    bool perform() override { return false; }
    bool undo() override { return true; }
    int getSizeInUnits() override { return 1; }
};

class UndoFailingAction : public juce::UndoableAction
{
public:
    bool perform() override { return true; }
    bool undo() override { return false; }
    int getSizeInUnits() override { return 1; }
};

class RedoFailingAction : public juce::UndoableAction
{
public:
    bool perform() override { return !std::exchange(performed, true); }
    bool undo() override { return true; }
    int getSizeInUnits() override { return 1; }

private:
    bool performed = false;
};

class PerformingProbeAction : public juce::UndoableAction
{
public:
    PerformingProbeAction(const UndoHistory& historyRef, std::vector<bool>& seenRef)
        : history(historyRef), seen(seenRef)
    {
    }

    bool perform() override
    {
        seen.push_back(history.isPerforming());
        return true;
    }

    bool undo() override
    {
        seen.push_back(history.isPerforming());
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    const UndoHistory& history;
    std::vector<bool>& seen;
};
} // namespace

TEST_CASE("a new history is at the save point with nothing to undo or redo", "[undo][history]")
{
    UndoHistory history;
    CHECK(history.isAtSavePoint());
    CHECK_FALSE(history.canUndo());
    CHECK_FALSE(history.canRedo());
}

TEST_CASE("performing leaves the save point and undoing returns to it", "[undo][history]")
{
    UndoHistory history;
    int value = 0;
    history.beginNewTransaction();
    CHECK(history.perform(new IncrementAction(value)));
    CHECK(value == 1);
    CHECK_FALSE(history.isAtSavePoint());

    CHECK(history.undo());
    CHECK(value == 0);
    CHECK(history.isAtSavePoint());
}

TEST_CASE("redoing back to the saved state returns to the save point", "[undo][history]")
{
    UndoHistory history;
    int value = 0;
    history.beginNewTransaction();
    history.perform(new IncrementAction(value));
    history.markSaved();
    history.undo();
    CHECK_FALSE(history.isAtSavePoint());

    CHECK(history.redo());
    CHECK(value == 1);
    CHECK(history.isAtSavePoint());
}

TEST_CASE("appending to the saved transaction loses the save point", "[undo][history]")
{
    UndoHistory history;
    int value = 0;
    history.beginNewTransaction();
    history.perform(new IncrementAction(value));
    history.markSaved();
    history.perform(new IncrementAction(value));
    CHECK_FALSE(history.isAtSavePoint());

    history.undo();
    CHECK(value == 0);
    CHECK_FALSE(history.isAtSavePoint());
}

TEST_CASE("an action performed right after an undo starts a new transaction", "[undo][history]")
{
    UndoHistory history;
    int value = 0;
    history.beginNewTransaction();
    history.perform(new IncrementAction(value));
    history.markSaved();
    history.beginNewTransaction();
    history.perform(new IncrementAction(value));
    history.undo();
    history.perform(new IncrementAction(value));
    CHECK_FALSE(history.isAtSavePoint());

    history.undo();
    CHECK(value == 1);
    CHECK(history.isAtSavePoint());
}

TEST_CASE("a rejected action leaves the history unchanged", "[undo][history]")
{
    UndoHistory history;
    int notifications = 0;
    history.onChanged = [&] { ++notifications; };
    history.beginNewTransaction();
    CHECK_FALSE(history.perform(new RejectedAction));
    CHECK(history.isAtSavePoint());
    CHECK_FALSE(history.canUndo());
    CHECK(notifications == 0);
}

TEST_CASE("undoing or redoing with nothing to undo or redo changes nothing", "[undo][history]")
{
    UndoHistory history;
    int notifications = 0;
    history.onChanged = [&] { ++notifications; };
    CHECK_FALSE(history.undo());
    CHECK_FALSE(history.redo());
    CHECK(history.isAtSavePoint());
    CHECK(notifications == 0);
}

TEST_CASE("a failed undo clears the history and leaves the save point", "[undo][history]")
{
    UndoHistory history;
    history.beginNewTransaction();
    history.perform(new UndoFailingAction);
    history.undo();
    CHECK_FALSE(history.canUndo());
    CHECK_FALSE(history.canRedo());
    CHECK_FALSE(history.isAtSavePoint());
}

TEST_CASE("a failed redo clears the history and leaves the save point", "[undo][history]")
{
    UndoHistory history;
    history.beginNewTransaction();
    history.perform(new RedoFailingAction);
    history.markSaved();
    history.undo();
    history.redo();
    CHECK_FALSE(history.canUndo());
    CHECK_FALSE(history.canRedo());
    CHECK_FALSE(history.isAtSavePoint());
}

TEST_CASE("clearing leaves the save point until saved again", "[undo][history]")
{
    UndoHistory history;
    int value = 0;
    history.beginNewTransaction();
    history.perform(new IncrementAction(value));
    history.markSaved();
    history.clear();
    CHECK_FALSE(history.canUndo());
    CHECK_FALSE(history.isAtSavePoint());

    history.markSaved();
    CHECK(history.isAtSavePoint());
}

TEST_CASE("every change to the history notifies the listener", "[undo][history]")
{
    UndoHistory history;
    int value = 0;
    int notifications = 0;
    history.onChanged = [&] { ++notifications; };
    history.beginNewTransaction();
    history.perform(new IncrementAction(value));
    CHECK(notifications == 1);
    history.undo();
    CHECK(notifications == 2);
    history.redo();
    CHECK(notifications == 3);
    history.markSaved();
    CHECK(notifications == 4);
    history.clear();
    CHECK(notifications == 5);
}

TEST_CASE("the history reports performing only while an action runs", "[undo][history]")
{
    UndoHistory history;
    std::vector<bool> seen;

    history.perform(new PerformingProbeAction(history, seen));
    const bool afterPerform = history.isPerforming();
    history.undo();
    const bool afterUndo = history.isPerforming();
    history.redo();

    CHECK(seen == std::vector<bool>{true, true, true});
    CHECK_FALSE(afterPerform);
    CHECK_FALSE(afterUndo);
    CHECK_FALSE(history.isPerforming());
}

TEST_CASE("the history is not performing after an action is rejected", "[undo][history]")
{
    UndoHistory history;

    CHECK_FALSE(history.perform(new RejectedAction));
    CHECK_FALSE(history.isPerforming());
}
