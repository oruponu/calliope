#pragma once

#include "PlaybackListener.h"
#include "PlaybackSnapshot.h"
#include <cstddef>
#include <mutex>
#include <vector>

class PlaybackProcessor
{
public:
    void resetCursors(const PlaybackSnapshot& snap, int tick);
    void process(const PlaybackSnapshot& snap, int fromTick, int toTick, PlaybackListener& sink);
    void sendAllNoteOffs(PlaybackListener& sink);
    void releaseActiveNotesForTrack(int trackIndex, PlaybackListener& sink);

private:
    void offExpired(int toTick, PlaybackListener& sink);

    std::size_t noteCursor = 0;
    std::size_t eventCursor = 0;
    std::vector<ScheduledNote> activeNotes;
    std::mutex activeNotesMutex;
};
