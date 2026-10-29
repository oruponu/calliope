#pragma once

#include "edit/event/NoteRef.h"
#include "engine/recording/RecordingLayer.h"
#include "model/MidiSequence.h"
#include <optional>
#include <utility>
#include <vector>

// Reads the notes the editors work on: a track's notes in the sequence and, while recording, the recording layer's
// notes for that track. A held note ends at the playhead.
class EditableNotes
{
public:
    EditableNotes(const MidiSequence& sequence, const RecordingLayer* layer, int playheadTick);

    std::optional<MidiNote> find(const NoteRef& ref) const;
    // A recorded note whose key is still down.
    bool isHeld(const NoteRef& ref) const;
    // The sequence's notes in index order, then the recorded notes in take order, then the held notes.
    std::vector<std::pair<NoteRef, MidiNote>> onTrack(int trackIndex) const;
    // By start tick, then note number, then ref.
    std::vector<NoteRef> inTimeOrder(int trackIndex) const;

private:
    const MidiSequence& sequence;
    const RecordingLayer* layer;
    int playheadTick;
};
