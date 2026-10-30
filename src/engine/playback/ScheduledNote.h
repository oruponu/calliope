#pragma once

#include "engine/playback/PlaybackTrackContext.h"
#include "model/event/MidiNote.h"

struct ScheduledNote
{
    PlaybackTrackContext ctx;
    MidiNote note;
};
