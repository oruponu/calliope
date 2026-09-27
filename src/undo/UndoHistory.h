#pragma once

#include <juce_data_structures/juce_data_structures.h>

class UndoHistory
{
public:
    UndoHistory() = default;

    void beginNewTransaction(const juce::String& name = {});
    bool perform(juce::UndoableAction* action);
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;
    juce::String getUndoDescription() const;
    juce::String getRedoDescription() const;
    void clear();

private:
    juce::UndoManager undoManager{10000, 100};

    JUCE_DECLARE_NON_COPYABLE(UndoHistory)
};
