#pragma once

#include "model/MidiSequence.h"
#include <set>

namespace ExclusiveSolo
{
// Solos only the given tracks, or unsolos them when they are already the only soloed tracks.
void toggle(MidiSequence& sequence, const std::set<int>& indices);
} // namespace ExclusiveSolo
