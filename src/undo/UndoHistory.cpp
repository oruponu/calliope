#include "undo/UndoHistory.h"

void UndoHistory::beginNewTransaction(const juce::String& name)
{
    undoManager.beginNewTransaction(name);
}

bool UndoHistory::perform(juce::UndoableAction* action)
{
    return undoManager.perform(action);
}

bool UndoHistory::undo()
{
    return undoManager.undo();
}

bool UndoHistory::redo()
{
    return undoManager.redo();
}

bool UndoHistory::canUndo() const
{
    return undoManager.canUndo();
}

bool UndoHistory::canRedo() const
{
    return undoManager.canRedo();
}

juce::String UndoHistory::getUndoDescription() const
{
    return undoManager.getUndoDescription();
}

juce::String UndoHistory::getRedoDescription() const
{
    return undoManager.getRedoDescription();
}

void UndoHistory::clear()
{
    undoManager.clearUndoHistory();
}
