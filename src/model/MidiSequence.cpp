#include "model/MidiSequence.h"
#include <algorithm>
#include <cassert>
#include <iterator>
#include <map>
#include <ranges>
#include <utility>

MidiSequence::ChangeBatch::ChangeBatch(MidiSequence& sequenceRef) : sequence(sequenceRef)
{
    ++sequence.batchDepth;
}

MidiSequence::ChangeBatch::~ChangeBatch()
{
    if (--sequence.batchDepth == 0)
        sequence.flushPendingNotifications();
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

void MidiSequence::replaceContents(SequenceContents contents)
{
    // Issue ids from this sequence's counter so that ids are never reused across documents.
    std::map<TrackId, TrackId> renumbered;
    for (auto& track : contents.tracks)
    {
        const TrackId fresh{nextTrackId++};
        if (track.id != TrackId{})
        {
            assert(!renumbered.contains(track.id));
            renumbered[track.id] = fresh;
        }
        track.id = fresh;
    }
    for (auto& track : contents.tracks)
    {
        if (!track.routeTarget)
            continue;
        auto it = renumbered.find(*track.routeTarget);
        track.routeTarget = it != renumbered.end() ? std::optional<TrackId>{it->second} : std::nullopt;
    }

    tracks = std::move(contents.tracks);
    timeline = std::move(contents.timeline);
    keySignatureChanges = std::move(contents.keySignatureChanges);
    chordChanges = std::move(contents.chordChanges);
    notifySequenceReset();
}

const MidiTrack& MidiSequence::addTrack(MidiTrack track)
{
    track.id = TrackId{nextTrackId++};
    tracks.push_back(std::move(track));
    notifyTrackStructureChanged();
    return tracks.back();
}

void MidiSequence::insertTrack(int index, const MidiTrack& track)
{
    assert(indexOf(track.getId()) < 0);
    tracks.insert(tracks.begin() + index, track);
    notifyTrackStructureChanged();
}

void MidiSequence::removeTrack(int index)
{
    tracks.erase(tracks.begin() + index);
    notifyTrackStructureChanged();
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

void MidiSequence::setTrackMuted(int index, bool muted)
{
    tracks[index].setMuted(muted);
    notifyTracksChanged();
}

void MidiSequence::setTrackSolo(int index, bool solo)
{
    tracks[index].setSolo(solo);
    notifyTracksChanged();
}

void MidiSequence::setTrackName(int index, const std::string& name)
{
    tracks[index].setName(name);
    notifyTracksChanged();
}

void MidiSequence::setTrackChannel(int index, int channel)
{
    tracks[index].setChannel(channel);
    notifyTracksChanged();
}

void MidiSequence::setTrackOutputDestination(int index, MidiTrack::OutputDestination destination)
{
    tracks[index].setOutputDestination(destination);
    notifyTracksChanged();
}

void MidiSequence::setTrackRouteTarget(int index, std::optional<TrackId> target)
{
    tracks[index].setRouteTarget(target);
    notifyTracksChanged();
}

void MidiSequence::setTrackPluginAssignment(int index, std::shared_ptr<const PluginAssignment> assignment)
{
    tracks[index].setPluginAssignment(std::move(assignment));
    notifyTracksChanged();
}

void MidiSequence::addNote(int trackIndex, const MidiNote& note)
{
    tracks[trackIndex].addNote(note);
    notifyNotesChanged(trackIndex);
}

void MidiSequence::insertNote(int trackIndex, int noteIndex, const MidiNote& note)
{
    tracks[trackIndex].insertNote(noteIndex, note);
    notifyNotesChanged(trackIndex);
}

void MidiSequence::removeNote(int trackIndex, int noteIndex)
{
    tracks[trackIndex].removeNote(noteIndex);
    notifyNotesChanged(trackIndex);
}

void MidiSequence::setNote(int trackIndex, int noteIndex, const MidiNote& note)
{
    tracks[trackIndex].getNote(noteIndex) = note;
    notifyNotesChanged(trackIndex);
}

const TimelineMap& MidiSequence::getTimeline() const
{
    return timeline;
}

void MidiSequence::setTempoChanges(std::vector<TempoChange> changes)
{
    timeline.setTempoChanges(std::move(changes));
    notifyTempoChanged();
}

void MidiSequence::setTimeSignatureChanges(std::vector<TimeSignatureChange> changes)
{
    timeline.setTimeSignatureChanges(std::move(changes));
    notifyTimelineMetadataChanged();
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

void MidiSequence::setKeySignatureChanges(std::vector<KeySignatureChange> changes)
{
    keySignatureChanges = std::move(changes);
    notifyTimelineMetadataChanged();
}

const std::vector<ChordChange>& MidiSequence::getChordChanges() const
{
    return chordChanges;
}

void MidiSequence::setChordChanges(std::vector<ChordChange> changes)
{
    chordChanges = std::move(changes);
    notifyTimelineMetadataChanged();
}

template <typename Callback> void MidiSequence::forEachListener(Callback callback)
{
    const auto snapshot = listeners;
    for (auto* l : snapshot)
        callback(*l);
}

void MidiSequence::notifyNotesChanged(int trackIndex)
{
    if (batchDepth > 0)
    {
        pending.noteTracks.insert(trackIndex);
        return;
    }
    forEachListener([trackIndex](Listener& l) { l.notesChanged(trackIndex); });
}

void MidiSequence::notifyTracksChanged()
{
    if (batchDepth > 0)
    {
        pending.tracks = true;
        return;
    }
    forEachListener([](Listener& l) { l.tracksChanged(); });
}

void MidiSequence::notifyTempoChanged()
{
    if (batchDepth > 0)
    {
        pending.tempo = true;
        return;
    }
    forEachListener([](Listener& l) { l.tempoChanged(); });
}

void MidiSequence::notifyTimelineMetadataChanged()
{
    if (batchDepth > 0)
    {
        pending.timelineMetadata = true;
        return;
    }
    forEachListener([](Listener& l) { l.timelineMetadataChanged(); });
}

void MidiSequence::notifySequenceReset()
{
    if (batchDepth > 0)
    {
        pending.reset = true;
        return;
    }
    forEachListener([](Listener& l) { l.sequenceReset(); });
}

void MidiSequence::notifyTrackStructureChanged()
{
    if (batchDepth > 0)
        pending.trackStructure = true;
    notifyTracksChanged();
}

void MidiSequence::flushPendingNotifications()
{
    const auto flushed = std::exchange(pending, {});
    if (flushed.reset)
    {
        notifySequenceReset();
        return;
    }

    if (flushed.tracks)
        notifyTracksChanged();
    if (!flushed.noteTracks.empty())
    {
        // A structural change inside the batch may have shifted the recorded track indices.
        const bool single = flushed.noteTracks.size() == 1 && !flushed.trackStructure;
        notifyNotesChanged(single ? *flushed.noteTracks.begin() : -1);
    }
    if (flushed.tempo)
        notifyTempoChanged();
    if (flushed.timelineMetadata)
        notifyTimelineMetadataChanged();
}
