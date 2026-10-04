#pragma once

#include "model/SequenceContents.h"

namespace SequenceContentsRules
{
// The value ranges and orderings the model relies on. They must admit everything the app itself can produce,
// MIDI import included, or a saved project would fail to open again. Track ids are not checked.
bool accepts(const SequenceContents& contents);
} // namespace SequenceContentsRules
