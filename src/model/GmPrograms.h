#pragma once

namespace GmPrograms
{
constexpr int programCount = 128;
constexpr int familyCount = 16;
constexpr int programsPerFamily = 8;

// program 0-127; nullptr outside the range.
const char* programName(int program);

// family 0-15; nullptr outside the range.
const char* familyName(int family);
} // namespace GmPrograms
