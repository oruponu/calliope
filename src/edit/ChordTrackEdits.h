#pragma once

#include "model/ChordChange.h"
#include "model/TimelineMap.h"
#include <utility>
#include <vector>

struct RelativeChord
{
    int tickOffset;
    int length;
    int chordRoot;
    int chordType;
    int bassRoot;
    int bassType;
};

namespace ChordTrackEdits
{
void add(std::vector<ChordChange>& changes, int tick, int chordRoot, int chordType, int bassRoot, int bassType);

std::pair<int, int> addSpanAt(const std::vector<ChordChange>& chords, int tick, const TimelineMap& timeline);

std::vector<ChordChange> afterAdd(const std::vector<ChordChange>& before, int startTick, int endTick, int chordRoot,
                                  int chordType, int bassRoot, int bassType);
std::vector<ChordChange> afterResize(const std::vector<ChordChange>& before, int chordIndex, int targetEndTick,
                                     int gridTicks);
std::vector<ChordChange> afterMove(const std::vector<ChordChange>& before, const std::vector<int>& movedIndices,
                                   int anchorIndex, int targetTick, int gridTicks);
std::vector<ChordChange> afterStartResize(const std::vector<ChordChange>& before, int chordIndex, int targetStartTick,
                                          int gridTicks);
std::vector<ChordChange> afterDelete(const std::vector<ChordChange>& before, const std::vector<int>& deletedIndices);
std::vector<ChordChange> afterPaste(const std::vector<ChordChange>& before, const std::vector<RelativeChord>& items,
                                    int anchorTick);
} // namespace ChordTrackEdits
