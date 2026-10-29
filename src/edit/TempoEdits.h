#pragma once

#include "model/timeline/TempoChange.h"
#include <set>
#include <vector>

namespace TempoEdits
{
std::vector<TempoChange> afterMove(const std::vector<TempoChange>& before, const std::vector<int>& movedIndices,
                                   int anchorIndex, int targetTick, double targetBpm, int gridTicks);
std::vector<TempoChange> afterDelete(const std::vector<TempoChange>& before, const std::set<int>& deletedIndices);
std::vector<TempoChange> afterPaste(const std::vector<TempoChange>& before, const std::vector<TempoChange>& items,
                                    int anchorTick);
} // namespace TempoEdits
