#pragma once

#include "edit/time/TimeEditError.h"
#include "model/TimedContents.h"
#include <expected>

// Deletes the time [start, end): everything at or after end moves forward by end - start, and each channel is left at
// start in the state it had at end.
std::expected<TimedContents, TimeEditError> deleteTime(const TimedContents& before, int start, int end);
