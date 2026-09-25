#pragma once

#include "model/TempoChange.h"
#include <vector>

namespace TempoEdits
{
// returns the index of the added or overwritten change
int add(std::vector<TempoChange>& changes, int tick, double bpm);
} // namespace TempoEdits
