#pragma once

#include <compare>

enum class NoteLayer
{
    Sequence,
    Recording
};

struct NoteRef
{
    int trackIndex = -1;
    // The note's index on the track, or its recorded note id when layer is Recording.
    int noteIndex = -1;
    NoteLayer layer = NoteLayer::Sequence;

    bool isValid() const { return trackIndex >= 0 && noteIndex >= 0; }
    bool isRecorded() const { return layer == NoteLayer::Recording; }
    auto operator<=>(const NoteRef&) const = default;
};
