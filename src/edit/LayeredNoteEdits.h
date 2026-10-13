#pragma once

#include "edit/NoteEdit.h"
#include "edit/NoteRef.h"
#include "engine/RecordingLayerEditor.h"
#include "model/MidiSequence.h"
#include "undo/UndoHistory.h"
#include <set>
#include <vector>

// Applies edits across the sequence and the recording layer. Only the sequence side becomes an undoable step; the
// recording layer side reaches the history inside the "Record" step at commit.
class LayeredNoteEdits
{
public:
    LayeredNoteEdits(MidiSequence& sequence, UndoHistory& history, RecordingLayerEditor* editor);

    void apply(const std::vector<NoteEdit>& edits, const juce::String& transactionName);
    void remove(const std::set<NoteRef>& refs, const juce::String& transactionName);

private:
    MidiSequence& sequence;
    UndoHistory& history;
    RecordingLayerEditor* editor;
};
