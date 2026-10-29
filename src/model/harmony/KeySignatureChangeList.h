#pragma once

#include "model/harmony/KeySignatureChange.h"
#include <vector>

namespace KeySignatureChangeList
{
void put(std::vector<KeySignatureChange>& changes, int tick, int sharpsOrFlats, bool isMinor);
} // namespace KeySignatureChangeList
