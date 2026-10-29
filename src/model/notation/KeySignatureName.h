#pragma once

#include <string>

namespace KeySignatureName
{
std::string toString(int sharpsOrFlats, bool isMinor);
bool fromString(const std::string& text, int& sharpsOrFlats, bool& isMinor);
int normalizeSharpsOrFlats(int sharpsOrFlats);
} // namespace KeySignatureName
