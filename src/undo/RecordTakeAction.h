#pragma once

#include "edit/NoteRef.h"
#include "engine/Take.h"
#include "model/MidiSequence.h"
#include "model/controller/ControllerPointConversion.h"
#include <algorithm>
#include <juce_data_structures/juce_data_structures.h>
#include <map>
#include <ranges>
#include <utility>
#include <vector>

// The recorded notes are appended and the action is always performed on top of the history, so undo can remove
// them by the positions perform used.
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

            const auto& track = sequence->getTrack(trackIndex);
            Placement placement{id, track.getNumNotes(), static_cast<int>(recorded.notes.size()), track.getEvents()};
            for (const auto& note : recorded.notes)
                sequence->addNote(trackIndex, note);
            if (!recorded.events.empty())
            {
                // Back to steps first, so a hold point after a recorded value holds that value instead.
                auto merged = convertHoldsToSteps(placement.eventsBefore);
                merged.insert(merged.end(), recorded.events.begin(), recorded.events.end());
                std::ranges::stable_sort(merged, {}, &MidiEvent::tick);
                sequence->setEvents(trackIndex, convertStepsToPoints(merged));
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
            if (!take.at(placement.track).events.empty())
                sequence->setEvents(trackIndex, placement.eventsBefore);
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

    // After perform: where each recorded note id was added.
    std::map<int, NoteRef> getCommittedNotes() const
    {
        std::map<int, NoteRef> committed;
        for (const auto& placement : placements)
        {
            const int trackIndex = sequence->indexOf(placement.track);
            const auto& ids = take.at(placement.track).noteIds;
            for (int i = 0; i < static_cast<int>(ids.size()); ++i)
                committed[ids[static_cast<size_t>(i)]] = {trackIndex, placement.firstNoteIndex + i};
        }
        return committed;
    }

private:
    struct Placement
    {
        TrackId track{};
        int firstNoteIndex = 0;
        int numNotes = 0;
        std::vector<MidiEvent> eventsBefore;
    };

    MidiSequence* sequence;
    Take take;
    std::vector<Placement> placements;
};
