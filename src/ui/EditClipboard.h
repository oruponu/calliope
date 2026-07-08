#pragma once

#include "../model/MidiNote.h"
#include "../model/MidiSequence.h"
#include <utility>
#include <variant>
#include <vector>

class EditClipboard
{
public:
    bool hasNotes() const { return std::holds_alternative<std::vector<MidiNote>>(content); }
    bool hasTempoPoints() const { return std::holds_alternative<std::vector<TempoChange>>(content); }

    void setNotes(std::vector<MidiNote> notes)
    {
        if (notes.empty())
            content = std::monostate{};
        else
            content = std::move(notes);
    }

    void setTempoPoints(std::vector<TempoChange> points)
    {
        if (points.empty())
            content = std::monostate{};
        else
            content = std::move(points);
    }

    const std::vector<MidiNote>& getNotes() const
    {
        if (const auto* notes = std::get_if<std::vector<MidiNote>>(&content))
            return *notes;
        static const std::vector<MidiNote> empty;
        return empty;
    }

    const std::vector<TempoChange>& getTempoPoints() const
    {
        if (const auto* points = std::get_if<std::vector<TempoChange>>(&content))
            return *points;
        static const std::vector<TempoChange> empty;
        return empty;
    }

private:
    std::variant<std::monostate, std::vector<MidiNote>, std::vector<TempoChange>> content;
};
