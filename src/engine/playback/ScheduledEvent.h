#pragma once

#include "engine/playback/PlaybackTrackContext.h"
#include "model/event/MidiEvent.h"

struct ScheduledEvent
{
    PlaybackTrackContext ctx;
    MidiEvent event;
};
