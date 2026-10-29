#include "session/EditableNotes.h"
#include <algorithm>
#include <ranges>

namespace
{
MidiNote heldAsNote(const HeldTakeNote& held, int playheadTick)
{
    return {held.noteNumber, held.velocity, held.startTick, std::max(1, playheadTick - held.startTick)};
}
} // namespace

EditableNotes::EditableNotes(const MidiSequence& sequenceRef, const RecordingLayer* layerRef, int playhead)
    : sequence(sequenceRef), layer(layerRef), playheadTick(playhead)
{
}

std::optional<MidiNote> EditableNotes::find(const NoteRef& ref) const
{
    if (ref.trackIndex < 0 || ref.trackIndex >= sequence.getNumTracks())
        return std::nullopt;
    const auto& track = sequence.getTrack(ref.trackIndex);
    if (!ref.isRecorded())
    {
        if (ref.noteIndex < 0 || ref.noteIndex >= track.getNumNotes())
            return std::nullopt;
        return track.getNote(ref.noteIndex);
    }
    if (layer == nullptr)
        return std::nullopt;

    const auto& take = layer->getTake();
    if (const auto recorded = take.find(track.getId()); recorded != take.end())
    {
        const auto& ids = recorded->second.noteIds;
        if (const auto at = std::ranges::find(ids, ref.noteIndex); at != ids.end())
            return recorded->second.notes[static_cast<size_t>(at - ids.begin())];
    }
    for (const auto& held : layer->getHeldNotes())
        if (held.id == ref.noteIndex && held.track == track.getId())
            return heldAsNote(held, playheadTick);
    return std::nullopt;
}

bool EditableNotes::isHeld(const NoteRef& ref) const
{
    if (!ref.isRecorded() || layer == nullptr || ref.trackIndex < 0 || ref.trackIndex >= sequence.getNumTracks())
        return false;
    const TrackId track = sequence.getTrack(ref.trackIndex).getId();
    return std::ranges::any_of(layer->getHeldNotes(), [&ref, track](const HeldTakeNote& held)
                               { return held.id == ref.noteIndex && held.track == track; });
}

std::vector<std::pair<NoteRef, MidiNote>> EditableNotes::onTrack(int trackIndex) const
{
    std::vector<std::pair<NoteRef, MidiNote>> notes;
    if (trackIndex < 0 || trackIndex >= sequence.getNumTracks())
        return notes;
    const auto& track = sequence.getTrack(trackIndex);
    for (int i = 0; i < track.getNumNotes(); ++i)
        notes.push_back({{trackIndex, i}, track.getNote(i)});
    if (layer == nullptr)
        return notes;

    const auto& take = layer->getTake();
    if (const auto recorded = take.find(track.getId()); recorded != take.end())
    {
        const auto& recordedNotes = recorded->second.notes;
        const auto& ids = recorded->second.noteIds;
        for (size_t i = 0; i < recordedNotes.size() && i < ids.size(); ++i)
            notes.push_back({{trackIndex, ids[i], NoteLayer::Recording}, recordedNotes[i]});
    }
    for (const auto& held : layer->getHeldNotes())
        if (held.track == track.getId())
            notes.push_back({{trackIndex, held.id, NoteLayer::Recording}, heldAsNote(held, playheadTick)});
    return notes;
}

std::vector<NoteRef> EditableNotes::inTimeOrder(int trackIndex) const
{
    auto notes = onTrack(trackIndex);
    std::ranges::sort(notes,
                      [](const auto& a, const auto& b)
                      {
                          if (a.second.startTick != b.second.startTick)
                              return a.second.startTick < b.second.startTick;
                          if (a.second.noteNumber != b.second.noteNumber)
                              return a.second.noteNumber < b.second.noteNumber;
                          return a.first < b.first;
                      });
    return notes | std::views::keys | std::ranges::to<std::vector>();
}
