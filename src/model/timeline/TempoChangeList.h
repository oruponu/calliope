#pragma once

#include "model/timeline/TempoChange.h"
#include <vector>

namespace TempoChangeList
{
// returns the index of the added or overwritten change
int put(std::vector<TempoChange>& changes, int tick, double bpm);
} // namespace TempoChangeList
