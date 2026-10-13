#pragma once

#include "edit/NoteRef.h"
#include "engine/NoteFieldEdit.h"

struct NoteEdit
{
    NoteRef ref;
    NoteFieldEdit fields;

    bool operator==(const NoteEdit&) const = default;
};
