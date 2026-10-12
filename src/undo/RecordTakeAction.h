#pragma once

#include "engine/Take.h"
#include "model/MidiSequence.h"
#include <algorithm>
#include <juce_data_structures/juce_data_structures.h>
#include <ranges>
#include <utility>
#include <vector>

// Appends the recorded notes and inserts the recorded events in tick order. It is always performed on top of the
// history, so undo can remove everything by the positions perform used.
class RecordTakeAction : public juce::UndoableAction
{
public:
    RecordTakeAction(MidiSequence* seq, Take recorded) : sequence(seq), take(std::move(recorded))
    {
        for (auto& [id, track] : take)
            std::ranges::stable_sort(track.events, {}, &MidiEvent::tick);
    }

    bool perform() override
    {
        MidiSequence::ChangeBatch batch(*sequence);
        placements.clear();
        for (const auto& [id, recorded] : take)
        {
            const int trackIndex = sequence->indexOf(id);
            if (trackIndex < 0)
                continue;

            Placement placement{
                id, sequence->getTrack(trackIndex).getNumNotes(), static_cast<int>(recorded.notes.size()), {}};
            for (const auto& note : recorded.notes)
                sequence->addNote(trackIndex, note);
            for (const auto& event : recorded.events)
            {
                const auto& events = sequence->getTrack(trackIndex).getEvents();
                const auto at = std::ranges::upper_bound(events, event.tick, {}, &MidiEvent::tick);
                const int index = static_cast<int>(at - events.begin());
                sequence->insertEvent(trackIndex, index, event);
                placement.eventIndices.push_back(index);
            }
            placements.push_back(std::move(placement));
        }
        return true;
    }

    bool undo() override
    {
        MidiSequence::ChangeBatch batch(*sequence);
        for (const auto& placement : placements | std::views::reverse)
        {
            const int trackIndex = sequence->indexOf(placement.track);
            if (trackIndex < 0)
                continue;
            for (const int index : placement.eventIndices | std::views::reverse)
                sequence->removeEvent(trackIndex, index);
            for (int i = placement.numNotes - 1; i >= 0; --i)
                sequence->removeNote(trackIndex, placement.firstNoteIndex + i);
        }
        return true;
    }

    int getSizeInUnits() override
    {
        int units = 0;
        for (const auto& [id, recorded] : take)
            units += static_cast<int>(recorded.notes.size() + recorded.events.size());
        return std::max(1, units);
    }

private:
    struct Placement
    {
        TrackId track{};
        int firstNoteIndex = 0;
        int numNotes = 0;
        std::vector<int> eventIndices;
    };

    MidiSequence* sequence;
    Take take;
    std::vector<Placement> placements;
};
