#pragma once

#include "engine/playback/PlaybackTrackContext.h"
#include "model/event/MidiEvent.h"
#include "model/event/MidiNote.h"

class PlaybackListener
{
public:
    virtual ~PlaybackListener() = default;
    virtual void onNoteOn(const PlaybackTrackContext& ctx, const MidiNote& note) = 0;
    virtual void onNoteOff(const PlaybackTrackContext& ctx, const MidiNote& note) = 0;
    virtual void onSysEx(const PlaybackTrackContext& ctx, const SysExEvent& sysEx) = 0;
    virtual void onMidiEvent(const PlaybackTrackContext& ctx, const MidiEvent& event) = 0;
};
