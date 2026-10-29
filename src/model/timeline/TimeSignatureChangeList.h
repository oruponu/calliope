#pragma once

#include "model/timeline/TimeSignatureChange.h"
#include <cstddef>
#include <vector>

namespace TimeSignatureChangeList
{
void put(std::vector<TimeSignatureChange>& changes, int tick, int numerator, int denominator, int ppq);

// The bar number of each change, counting the first change as bar 1.
std::vector<int> barNumbers(const std::vector<TimeSignatureChange>& changes, int ppq);

// Moves the changes from first on so that each starts the bar given in bars, counting from the change before it.
void retick(std::vector<TimeSignatureChange>& changes, const std::vector<int>& bars, std::size_t first, int ppq);
} // namespace TimeSignatureChangeList
