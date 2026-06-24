#pragma once

#include "../model/MidiEvent.h"
#include "../model/MidiNote.h"
#include "PlaybackSnapshot.h"

class PlaybackListener
{
public:
    virtual ~PlaybackListener() = default;
    virtual void onNoteOn(const PlaybackTrackContext& ctx, const MidiNote& note) = 0;
    virtual void onNoteOff(const PlaybackTrackContext& ctx, const MidiNote& note) = 0;
    virtual void onMidiEvent(const PlaybackTrackContext& ctx, const MidiEvent& event) = 0;
};
