#pragma once
// IWYU pragma: always_keep

#include "model/MidiSequence.h"
#include <catch2/catch_tostring.hpp>
#include <string>

namespace Catch
{
template <> struct StringMaker<KeySignatureChange>
{
    static std::string convert(const KeySignatureChange& ks)
    {
        const auto name = MidiSequence::keySignatureToString(ks.sharpsOrFlats, ks.isMinor);
        if (name == "--")
            return std::to_string(ks.tick) + ":--(sf=" + std::to_string(ks.sharpsOrFlats) + ")";
        return std::to_string(ks.tick) + ":" + name;
    }
};
} // namespace Catch
