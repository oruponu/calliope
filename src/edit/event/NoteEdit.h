#pragma once

#include "edit/event/NoteFieldEdit.h"
#include "edit/event/NoteRef.h"

struct NoteEdit
{
    NoteRef ref;
    NoteFieldEdit fields;

    bool operator==(const NoteEdit&) const = default;
};
