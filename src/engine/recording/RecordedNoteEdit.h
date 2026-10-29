#pragma once

#include "edit/event/NoteFieldEdit.h"

struct RecordedNoteEdit
{
    int id = 0;
    NoteFieldEdit fields;

    bool operator==(const RecordedNoteEdit&) const = default;
};
