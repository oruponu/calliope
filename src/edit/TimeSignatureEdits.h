#pragma once

#include "model/timeline/TimeSignatureChange.h"
#include <set>
#include <vector>

struct RelativeTimeSignature
{
    int barOffset;
    int numerator;
    int denominator;
};

namespace TimeSignatureEdits
{
std::vector<TimeSignatureChange> afterMove(const std::vector<TimeSignatureChange>& before,
                                           const std::vector<int>& movedIndices, int anchorIndex, int targetTick,
                                           int ppq);
std::vector<TimeSignatureChange> afterDelete(const std::vector<TimeSignatureChange>& before,
                                             const std::set<int>& deletedIndices, int ppq);
std::vector<TimeSignatureChange> afterPaste(const std::vector<TimeSignatureChange>& before,
                                            const std::vector<RelativeTimeSignature>& items, int anchorBar, int ppq);
} // namespace TimeSignatureEdits
