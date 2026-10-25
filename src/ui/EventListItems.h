#pragma once

#include "edit/NoteRef.h"
#include "engine/RecordingLayer.h"
#include "model/ControllerKind.h"
#include "model/MidiSequence.h"
#include "ui/NoteDragPreview.h"
#include <cstdint>
#include <optional>
#include <set>
#include <vector>

struct EventListItem
{
    enum Kind
    {
        SysEx,
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
    std::vector<std::uint8_t> sysExData;
    bool isRecording = false;
    // The note being drawn, not yet in the sequence; it has no note ref.
    bool isDraft = false;
};

namespace EventListItems
{
// The shown tracks' notes and events, then the recording layer's, by tick, then track, then kind. Held notes are
// left out. A note the preview changes is listed as the preview shows it, and the preview's draft note is added when
// its track is shown. The events of the track whose points the preview drags are listed as the preview shows them.
std::vector<EventListItem> build(const MidiSequence& sequence, const std::set<int>& tracks, const RecordingLayer* layer,
                                 const NoteDragPreview& preview = {});
// The grabbed point's row while dragging points, or the first dragged row when that is not listed; the draft row while
// drawing; otherwise the focus note's row.
std::optional<int> focusRow(const std::vector<EventListItem>& items, const NoteDragPreview& preview);
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
// The rows, in order, of the non-recorded events on the track whose indices are given; while points are dragged,
// the indices are into the drag's events.
std::vector<int> eventRows(const std::vector<EventListItem>& items, int trackIndex, const std::set<int>& eventIndices);
// The rows, in order, of the points the preview drags; none when it drags no points.
std::vector<int> draggedRows(const std::vector<EventListItem>& items, const NoteDragPreview& preview);
// The controller of a control change or pitch bend row in the sequence; none for other rows and recorded ones.
std::optional<ControllerKind> controllerOf(const EventListItem& item);
// The event indices of the selected rows that hold the clicked row's controller on its track.
std::set<int> sameControllerEvents(const std::vector<EventListItem>& items, const std::vector<int>& selectedRows,
                                   int clickedRow);
} // namespace EventListItems
