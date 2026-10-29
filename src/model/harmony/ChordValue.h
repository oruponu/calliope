#pragma once

#include "model/harmony/ChordSpelling.h"

namespace ChordValue
{
// A root has a note from C to B and an accidental from bbb to ###.
bool isValidRoot(int root);

int normalizeRoot(int root);
int normalizeType(int type);
int normalizeBassRoot(int bassRoot);

int rootToSemitone(int root);
int semitoneToRoot(int semitone, ChordSpelling spelling);
ChordSpelling spellingForKeySignature(int sharpsOrFlats);
} // namespace ChordValue
