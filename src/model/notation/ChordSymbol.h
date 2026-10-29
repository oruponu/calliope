#pragma once

#include "model/harmony/ChordChange.h"
#include "model/notation/ChordSymbolParts.h"
#include <string>

namespace ChordSymbol
{
std::string toString(const ChordChange& chord);
ChordSymbolParts toParts(const ChordChange& chord);

std::string rootToString(int root);
bool rootFromString(const std::string& text, int& root);
std::string typeToString(int type);
bool typeFromString(const std::string& text, int& type);
} // namespace ChordSymbol
