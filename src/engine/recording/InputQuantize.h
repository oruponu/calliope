#pragma once

#include "engine/transport/PlaybackClock.h"
#include "model/grid/TimelineGrid.h"
#include <optional>

// Where a note played at playedTick starts when input is quantized: the nearest grid line, the later one on a tie. A
// note played inside an enabled loop stays inside it: rounded before the loop start it starts there, and rounded to the
// loop end or later it starts the next loop pass instead (nullopt), wherever that pass starts once the loop wraps.
std::optional<int> quantizeInputStart(int playedTick, const TimelineGrid& grid, LoopRange loop);
