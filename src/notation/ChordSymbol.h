#pragma once

#include "model/ChordChange.h"
#include <string>

enum class ChordSpelling
{
    Sharp,
    Flat,
    Mixed
};

namespace ChordSymbol
{
std::string toString(const ChordChange& chord);

std::string rootToString(int root);
bool rootFromString(const std::string& text, int& root);
std::string typeToString(int type);
bool typeFromString(const std::string& text, int& type);

int rootToSemitone(int root);
int semitoneToRoot(int semitone, ChordSpelling spelling);
ChordSpelling spellingForKeySignature(int sharpsOrFlats);

int normalizeRoot(int root);
int normalizeType(int type);
int normalizeBassRoot(int bassRoot);
} // namespace ChordSymbol
