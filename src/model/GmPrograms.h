#pragma once

namespace GmPrograms
{
constexpr int programCount = 128;

// program 0-127; nullptr outside the range.
const char* programName(int program);
} // namespace GmPrograms
