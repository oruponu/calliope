#include "undo/UndoHistory.h"

void UndoHistory::beginNewTransaction(const juce::String& name)
{
    undoManager.beginNewTransaction(name);
}

bool UndoHistory::perform(juce::UndoableAction* action)
{
    const juce::ScopedValueSetter<bool> performingScope(performing, true);
    const bool startsNewTransaction = undoManager.getNumActionsInCurrentTransaction() == 0;
    if (!undoManager.perform(action))
        return false;

    if (startsNewTransaction)
        savePoint.transactionPerformed();
    else
        savePoint.actionAppended();
    notifyChanged();
    return true;
}

bool UndoHistory::undo()
{
    const juce::ScopedValueSetter<bool> performingScope(performing, true);
    if (!undoManager.undo())
        return false;

    // juce::UndoManager clears the whole history when an action fails to undo.
    if (!undoManager.canUndo() && !undoManager.canRedo())
        savePoint.cleared();
    else
        savePoint.undone();
    notifyChanged();
    return true;
}

bool UndoHistory::redo()
{
    const juce::ScopedValueSetter<bool> performingScope(performing, true);
    if (!undoManager.redo())
        return false;

    // juce::UndoManager clears the whole history when an action fails to redo.
    if (!undoManager.canUndo() && !undoManager.canRedo())
        savePoint.cleared();
    else
        savePoint.redone();
    notifyChanged();
    return true;
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
    savePoint.cleared();
    notifyChanged();
}

void UndoHistory::markSaved()
{
    savePoint.markSaved();
    notifyChanged();
}

bool UndoHistory::isAtSavePoint() const
{
    return savePoint.isAtSavePoint();
}

bool UndoHistory::isPerforming() const
{
    return performing;
}

void UndoHistory::notifyChanged()
{
    if (onChanged)
        onChanged();
}
