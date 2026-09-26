#include "model/MidiSequence.h"
#include <algorithm>
#include <cassert>
#include <iterator>
#include <ranges>

void MidiSequence::clear()
{
    tracks.clear();
    timeline = TimelineMap{};
    keySignatureChanges.clear();
    chordChanges.clear();
}

MidiTrack& MidiSequence::addTrack()
{
    auto& track = tracks.emplace_back();
    track.id = TrackId{nextTrackId++};
    return track;
}

void MidiSequence::insertTrack(int index, const MidiTrack& track)
{
    assert(indexOf(track.getId()) < 0);
    tracks.insert(tracks.begin() + index, track);
}

void MidiSequence::removeTrack(int index)
{
    tracks.erase(tracks.begin() + index);
}

MidiTrack& MidiSequence::getTrack(int index)
{
    return tracks[index];
}

const MidiTrack& MidiSequence::getTrack(int index) const
{
    return tracks[index];
}

int MidiSequence::getNumTracks() const
{
    return static_cast<int>(tracks.size());
}

bool MidiSequence::isAnySolo() const
{
    return std::ranges::any_of(tracks, [](const MidiTrack& track) { return track.isSolo(); });
}

int MidiSequence::indexOf(TrackId id) const
{
    auto it = std::ranges::find(tracks, id, &MidiTrack::getId);
    return it == tracks.end() ? -1 : static_cast<int>(std::distance(tracks.begin(), it));
}

TrackId MidiSequence::resolveRouteTarget(int index) const
{
    const auto& track = tracks[index];
    if (const auto& target = track.getRouteTarget(); target && indexOf(*target) >= 0)
        return *target;
    return track.getId();
}

KeySignatureChange MidiSequence::getKeySignatureAt(int tick) const
{
    auto reversed = std::views::reverse(keySignatureChanges);
    auto it = std::ranges::find_if(reversed, [tick](const KeySignatureChange& ks) { return ks.tick <= tick; });
    return it != reversed.end() ? *it : KeySignatureChange{0, 0, false};
}

const std::vector<KeySignatureChange>& MidiSequence::getKeySignatureChanges() const
{
    return keySignatureChanges;
}

const std::vector<ChordChange>& MidiSequence::getChordChanges() const
{
    return chordChanges;
}

const TimelineMap& MidiSequence::getTimeline() const
{
    return timeline;
}

void MidiSequence::setTicksPerQuarterNote(int ppq)
{
    timeline.setTicksPerQuarterNote(ppq);
}

void MidiSequence::setTempoChanges(std::vector<TempoChange> changes)
{
    timeline.setTempoChanges(std::move(changes));
}

void MidiSequence::setTimeSignatureChanges(std::vector<TimeSignatureChange> changes)
{
    timeline.setTimeSignatureChanges(std::move(changes));
}

void MidiSequence::setKeySignatureChanges(std::vector<KeySignatureChange> changes)
{
    keySignatureChanges = std::move(changes);
}

void MidiSequence::setChordChanges(std::vector<ChordChange> changes)
{
    chordChanges = std::move(changes);
}

void MidiSequence::addListener(Listener* listener)
{
    if (listener != nullptr && std::find(listeners.begin(), listeners.end(), listener) == listeners.end())
        listeners.push_back(listener);
}

void MidiSequence::removeListener(Listener* listener)
{
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

void MidiSequence::notifyNotesChanged(int trackIndex)
{
    const auto snapshot = listeners;
    for (auto* l : snapshot)
        l->notesChanged(trackIndex);
}

void MidiSequence::notifyTracksChanged()
{
    const auto snapshot = listeners;
    for (auto* l : snapshot)
        l->tracksChanged();
}

void MidiSequence::notifyTempoChanged()
{
    const auto snapshot = listeners;
    for (auto* l : snapshot)
        l->tempoChanged();
}

void MidiSequence::notifyTimelineMetadataChanged()
{
    const auto snapshot = listeners;
    for (auto* l : snapshot)
        l->timelineMetadataChanged();
}

void MidiSequence::notifySequenceReset()
{
    const auto snapshot = listeners;
    for (auto* l : snapshot)
        l->sequenceReset();
}
