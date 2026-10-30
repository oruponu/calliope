#pragma once

#include "engine/playback/PlaybackTrackContext.h"
#include "model/event/SysExEvent.h"

struct ScheduledSysEx
{
    PlaybackTrackContext ctx;
    SysExEvent sysEx;
};
