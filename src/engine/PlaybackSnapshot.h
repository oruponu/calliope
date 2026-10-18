#pragma once

#include "engine/Take.h"
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
    bool operator==(const PlaybackTrackContext&) const = default;
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
    std::vector<TimeSignatureChange> timeSignatureChanges;
    int ticksPerQuarterNote = TimelineMap::defaultTicksPerQuarterNote;

    double getTempoAt(int tick) const;
    // The tick reached by playing for elapsedMs from tick, switching tempo at each change on the way.
    double tickAfter(double tick, double elapsedMs) const;
    // toTick must be at or after fromTick.
    double msBetween(double fromTick, double toTick) const;
    static PlaybackSnapshot build(const MidiSequence& seq, const Take* take = nullptr);
};

bool isTrackAudible(const MidiSequence& seq, int trackIndex);

PlaybackTrackContext makePlaybackTrackContext(const MidiSequence& seq, int trackIndex);
