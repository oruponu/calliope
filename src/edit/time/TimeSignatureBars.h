#pragma once

#include "model/timeline/TimeSignatureChange.h"
#include <vector>

// Every change after the first lies a whole number of bars, counted in the change before it, after that change.
bool timeSignaturesStartBars(const std::vector<TimeSignatureChange>& changes, int ppq);
