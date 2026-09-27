#pragma once

#include <cstdint>
#include <optional>

// Positions count transactions from the last clear. They are absolute, so transactions that the undo manager
// drops from the bottom of its history do not shift them.
class SavePointTracker
{
public:
    void transactionPerformed()
    {
        // The undo manager discards the redo side, which holds the saved state.
        if (saved && *saved > position)
            saved.reset();
        ++position;
    }

    void actionAppended()
    {
        // The saved state's transaction now contains more, so undoing no longer lands on the saved state.
        if (saved && *saved >= position)
            saved.reset();
    }

    void undone() { --position; }
    void redone() { ++position; }

    void cleared()
    {
        position = 0;
        saved.reset();
    }

    void markSaved() { saved = position; }

    bool isAtSavePoint() const { return saved == position; }

private:
    std::int64_t position = 0;
    std::optional<std::int64_t> saved = 0;
};
