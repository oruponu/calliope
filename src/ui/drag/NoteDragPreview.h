#pragma once

#include "edit/event/NoteRef.h"
#include "model/event/MidiNote.h"
#include "ui/drag/DraftNote.h"
#include "ui/drag/DraggedEvents.h"
#include <map>
#include <optional>

// What a note or controller point drag shows until the mouse is released; the sequence does not change before then.
struct NoteDragPreview
{
    // For the whole drag, even while it shows nothing different from the sequence.
    bool dragging = false;
    std::map<NoteRef, MidiNote> changedNotes;
    std::optional<DraftNote> draftNote;
    // The note whose row is kept on screen; invalid for none.
    NoteRef focus;
    std::optional<DraggedEvents> draggedEvents;

    bool operator==(const NoteDragPreview&) const = default;
};
