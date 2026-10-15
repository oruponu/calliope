#pragma once

#include "edit/NoteRef.h"
#include "engine/RecordingLayer.h"
#include "model/MidiSequence.h"
#include <set>
#include <vector>

struct EventListItem
{
    enum Kind
    {
        Note,
        ControlChange,
        ProgramChange,
        PitchBend,
        ChannelPressure,
        KeyPressure
    };

    Kind kind = Note;
    int tick = 0;
    int trackIndex = 0;
    // The note or event index on the track, or the recorded note id for a recorded note.
    int sourceIndex = 0;
    int noteNumber = 0;
    int duration = 0;
    int velocity = 0;
    int data1 = 0;
    int data2 = 0;
    bool isRecording = false;
};

namespace EventListItems
{
// The shown tracks' notes and events, then the recording layer's, by tick, then track, then kind. Held notes are
// left out.
std::vector<EventListItem> build(const MidiSequence& sequence, const std::set<int>& tracks,
                                 const RecordingLayer* layer);
NoteRef noteRefOf(const EventListItem& item);
// The first row at the tick when one is there, otherwise the last row before it, or the first row; -1 for no rows.
int playheadRow(const std::vector<EventListItem>& items, double tick);
// Whether b is the event row a was before a rebuild. Event indices shift when events are added or removed, so the
// values decide, not the index.
bool isSameEvent(const EventListItem& a, const EventListItem& b);
// The rows of after, in order, that hold what the selected rows of before held: a note by its ref, an event by its
// values and its place among the equal events.
std::vector<int> reselectedRows(const std::vector<EventListItem>& before, const std::vector<int>& selected,
                                const std::vector<EventListItem>& after);
} // namespace EventListItems
