#pragma once

#include "model/KeySignatureChange.h"
#include "model/SequenceContents.h"
#include "model/TimeSignatureChange.h"

namespace SequenceContentsRules
{
// The value ranges and orderings the model relies on. They must admit everything the app itself can produce,
// MIDI import included, or a saved project would fail to open again. Track ids are not checked.
bool accepts(const SequenceContents& contents);

bool acceptsTimeSignature(const TimeSignatureChange& change, int ppq);
bool acceptsKeySignature(const KeySignatureChange& change);
} // namespace SequenceContentsRules
