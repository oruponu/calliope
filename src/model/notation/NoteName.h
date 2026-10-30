#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace NoteName
{
// C4 is note 60.
std::string format(int noteNumber);
// A note number, or a note name as format writes it, in either case and with a sharp or a flat. Kept inside 0 to
// 127; nullopt for anything else.
std::optional<int> parse(std::string_view text);
} // namespace NoteName
