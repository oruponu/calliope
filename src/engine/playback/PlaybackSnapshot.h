#pragma once

#include "engine/playback/PlaybackTrackContext.h"
#include "engine/playback/ScheduledEvent.h"
#include "engine/playback/ScheduledNote.h"
#include "engine/playback/ScheduledSysEx.h"
#include "engine/recording/Take.h"
#include "model/MidiSequence.h"
#include "model/event/MidiEvent.h"
#include "model/event/MidiNote.h"
#include "model/event/SysExEvent.h"
#include "model/track/MidiTrack.h"
#include <vector>

struct PlaybackSnapshot
{
    std::vector<ScheduledNote> notes;
    std::vector<ScheduledEvent> events;
    std::vector<ScheduledSysEx> sysExEvents;
    // The audible tracks, in track order, whether or not they have notes or events.
    std::vector<PlaybackTrackContext> contexts;
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
