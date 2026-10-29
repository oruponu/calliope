#pragma once

#include <string>

namespace KeySignatureName
{
std::string toString(int sharpsOrFlats, bool isMinor);
bool fromString(const std::string& text, int& sharpsOrFlats, bool& isMinor);
} // namespace KeySignatureName
