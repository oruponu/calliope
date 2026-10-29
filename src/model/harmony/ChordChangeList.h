#pragma once

#include "model/harmony/ChordChange.h"
#include <vector>

namespace ChordChangeList
{
void put(std::vector<ChordChange>& changes, int tick, int chordRoot, int chordType, int bassRoot, int bassType);
} // namespace ChordChangeList
