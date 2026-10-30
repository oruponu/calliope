#pragma once

#include "engine/playback/PlaybackTrackContext.h"
#include "model/MidiSequence.h"
#include <optional>

std::optional<PlaybackTrackContext> resolveThruTarget(const MidiSequence& seq, int activeTrackIndex);
