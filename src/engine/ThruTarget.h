#pragma once

#include "engine/PlaybackSnapshot.h"
#include "model/MidiSequence.h"
#include <optional>

std::optional<PlaybackTrackContext> resolveThruTarget(const MidiSequence& seq, int activeTrackIndex);
