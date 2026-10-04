#pragma once

#include "model/KeySignatureChange.h"
#include "model/TimelineMap.h"
#include <set>
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
std::vector<KeySignatureChange> afterDelete(const std::vector<KeySignatureChange>& before,
                                            const std::set<int>& deletedIndices);
std::vector<KeySignatureChange> afterPaste(const std::vector<KeySignatureChange>& before,
                                           const std::vector<RelativeKeySignature>& items, int anchorBar,
                                           const TimelineMap& timeline);
} // namespace KeySignatureEdits
