#pragma once

#include "model/MidiNote.h"
#include <algorithm>
#include <optional>

// The fields one edit sets on a note. Fields left unset keep the note's value when the edit is applied, so an edit
// made for a held note still keeps the length the note was released with in the meantime.
struct NoteFieldEdit
{
    std::optional<int> startTick;
    // With startTick, keep the note's end rather than its length.
    bool keepEnd = false;
    std::optional<int> endTick;
    std::optional<int> noteNumber;
    std::optional<int> velocity;
    // The length a held note ends with when its start moves to the playhead or later; without it, the length it has
    // reached. A finished note ignores it.
    std::optional<int> heldLength;

    MidiNote appliedTo(MidiNote note) const
    {
        if (startTick)
        {
            const int end = note.endTick();
            note.startTick = *startTick;
            if (keepEnd)
                note.duration = std::max(1, end - note.startTick);
        }
        if (endTick)
            note.duration = std::max(1, *endTick - note.startTick);
        if (noteNumber)
            note.noteNumber = *noteNumber;
        if (velocity)
            note.velocity = *velocity;
        return note;
    }

    bool operator==(const NoteFieldEdit&) const = default;
};
