#pragma once

#include "../model/MidiEvent.h"
#include "../model/MidiNote.h"
#include "../model/MidiSequence.h"
#include "../model/MidiTrack.h"
#include <vector>

struct PlaybackTrackContext
{
    int trackIndex = 0;
    int channel = 1;
    int routeTarget = 0;
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
    int ticksPerQuarterNote = MidiSequence::defaultTicksPerQuarterNote;

    double getTempoAt(int tick) const;
    static PlaybackSnapshot build(const MidiSequence& seq);
};
