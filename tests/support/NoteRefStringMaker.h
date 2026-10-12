#pragma once
// IWYU pragma: always_keep

#include "edit/NoteRef.h"
#include <catch2/catch_tostring.hpp>
#include <format>
#include <string>

namespace Catch
{
template <> struct StringMaker<NoteRef>
{
    static std::string convert(const NoteRef& ref)
    {
        return std::format("{}{}:{}", ref.isRecorded() ? "recorded " : "", ref.trackIndex, ref.noteIndex);
    }
};
} // namespace Catch
