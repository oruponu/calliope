#pragma once
// IWYU pragma: always_keep

#include "edit/event/NoteEdit.h"
#include <catch2/catch_tostring.hpp>
#include <format>
#include <optional>
#include <string>

namespace Catch
{
template <> struct StringMaker<NoteEdit>
{
    static std::string convert(const NoteEdit& edit)
    {
        const auto field = [](const char* name, const std::optional<int>& value)
        { return value ? std::format(" {}={}", name, *value) : std::string{}; };
        const auto& f = edit.fields;
        return std::format("{}{}:{}{}{}{}{}{}{}", edit.ref.isRecorded() ? "recorded " : "", edit.ref.trackIndex,
                           edit.ref.noteIndex, field("start", f.startTick), f.keepEnd ? " keepEnd" : "",
                           field("end", f.endTick), field("note", f.noteNumber), field("vel", f.velocity),
                           field("held", f.heldLength));
    }
};
} // namespace Catch
