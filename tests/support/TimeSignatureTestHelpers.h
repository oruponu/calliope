#pragma once

#include "edit/TimeSignatureEdits.h"
#include "model/MidiSequence.h"
#include <catch2/catch_tostring.hpp>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

namespace timesigtest
{
constexpr int ppq = 480;

struct BarSignature
{
    int bar;
    int numerator;
    int denominator;
};

inline std::vector<TimeSignatureChange> timeSigs(std::initializer_list<BarSignature> entries)
{
    std::vector<TimeSignatureChange> result;
    int prevBar = 1;
    for (const auto& e : entries)
    {
        if (result.empty())
        {
            if (e.bar != 1)
                throw std::invalid_argument("the first time signature must be at bar 1");
            result.push_back({0, e.numerator, e.denominator});
        }
        else
        {
            if (e.bar <= prevBar)
                throw std::invalid_argument("bars must be strictly increasing");
            const auto prev = result.back();
            const int ticksPerBar = ppq * 4 / prev.denominator * prev.numerator;
            result.push_back({prev.tick + (e.bar - prevBar) * ticksPerBar, e.numerator, e.denominator});
        }
        prevBar = e.bar;
    }
    return result;
}

inline void setTimeSignatures(MidiSequence& sequence, std::initializer_list<BarSignature> entries)
{
    sequence.setTimeSignatureChanges(timeSigs(entries));
}

inline void setTimeSignatures(TimelineMap& timeline, std::initializer_list<BarSignature> entries)
{
    timeline.setTimeSignatureChanges(timeSigs(entries));
}
} // namespace timesigtest

namespace Catch
{
template <> struct StringMaker<TimeSignatureChange>
{
    static std::string convert(const TimeSignatureChange& ts)
    {
        return std::to_string(ts.tick) + ":" + std::to_string(ts.numerator) + "/" + std::to_string(ts.denominator);
    }
};
} // namespace Catch
