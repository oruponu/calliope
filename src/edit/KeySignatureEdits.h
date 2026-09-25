#pragma once

#include "model/KeySignatureChange.h"
#include "model/TimelineMap.h"
#include <vector>

struct RelativeKeySignature
{
    int barOffset;
    int sharpsOrFlats;
    bool isMinor;
};

namespace KeySignatureEdits
{
void add(std::vector<KeySignatureChange>& changes, int tick, int sharpsOrFlats, bool isMinor);

std::vector<KeySignatureChange> afterMove(const std::vector<KeySignatureChange>& before,
                                          const std::vector<int>& movedIndices, int anchorIndex, int targetTick,
                                          const TimelineMap& timeline);
} // namespace KeySignatureEdits
