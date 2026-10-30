#pragma once

#include "undo/SavePointTracker.h"
#include <functional>
#include <juce_data_structures/juce_data_structures.h>

class UndoHistory
{
public:
    UndoHistory() = default;

    void beginNewTransaction(const juce::String& name = {});
    bool perform(juce::UndoableAction* action);
    bool undo();
    bool redo();
    // Rolls back the transaction still being added to and drops it, so it leaves no redo step and the redo steps from
    // before it come back. A save point the transaction discarded from the redo side stays lost.
    bool cancelCurrentTransaction();
    bool canUndo() const;
    bool canRedo() const;
    juce::String getUndoDescription() const;
    juce::String getRedoDescription() const;
    void clear();

    void markSaved();
    bool isAtSavePoint() const;
    bool isPerforming() const;

    std::function<void()> onChanged;

private:
    void notifyChanged();

    juce::UndoManager undoManager{10000, 100};
    SavePointTracker savePoint;
    bool performing = false;

    JUCE_DECLARE_NON_COPYABLE(UndoHistory)
};
