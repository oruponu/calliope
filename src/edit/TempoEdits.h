#pragma once

#include "model/TempoChange.h"
#include <set>
#include <vector>

namespace TempoEdits
{
// returns the index of the added or overwritten change
int add(std::vector<TempoChange>& changes, int tick, double bpm);

std::vector<TempoChange> afterDelete(const std::vector<TempoChange>& before, const std::set<int>& deletedIndices);
std::vector<TempoChange> afterPaste(const std::vector<TempoChange>& before, const std::vector<TempoChange>& items,
                                    int anchorTick);
} // namespace TempoEdits
