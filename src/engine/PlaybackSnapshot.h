#pragma once

#include "model/MidiEvent.h"
#include "model/MidiNote.h"
#include "model/MidiSequence.h"
#include "model/MidiTrack.h"
#include <vector>

struct PlaybackTrackContext
{
    TrackId trackId{};
    int channel = 1;
    TrackId routeTarget{};
    MidiTrack::OutputDestination destination = MidiTrack::OutputDestination::MidiDevice;
};

struct ScheduledNote
{
    PlaybackTrackContext ctx;
    MidiNote note;
};

struct ScheduledEvent
{
    PlaybackTrackContext ctx;
    MidiEvent event;
};

struct PlaybackSnapshot
{
    std::vector<ScheduledNote> notes;
    std::vector<ScheduledEvent> events;
    std::vector<TempoChange> tempoChanges;
    int ticksPerQuarterNote = TimelineMap::defaultTicksPerQuarterNote;

    double getTempoAt(int tick) const;
    static PlaybackSnapshot build(const MidiSequence& seq);
};

PlaybackTrackContext makePlaybackTrackContext(const MidiSequence& seq, int trackIndex);
