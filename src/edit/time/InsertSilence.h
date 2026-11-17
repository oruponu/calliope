#pragma once

#include "edit/time/TimeEditError.h"
#include "model/TimedContents.h"
#include <expected>

// Inserts the silence [start, end) at start: everything at or after start moves back by end - start.
std::expected<TimedContents, TimeEditError> insertSilence(const TimedContents& before, int start, int end);
