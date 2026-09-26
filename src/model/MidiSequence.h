#pragma once

#include "model/ChordChange.h"
#include "model/KeySignatureChange.h"
#include "model/MidiTrack.h"
#include "model/TimelineMap.h"
#include "model/TrackId.h"
#include <cstdint>
#include <vector>

class MidiSequence
{
public:
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void notesChanged([[maybe_unused]] int trackIndex) {}
        virtual void tracksChanged() {}
        virtual void tempoChanged() {}
        virtual void timelineMetadataChanged() {}
        virtual void sequenceReset() {}
    };

    MidiSequence() = default;
    MidiSequence(const MidiSequence&) = delete;
    MidiSequence& operator=(const MidiSequence&) = delete;
    MidiSequence(MidiSequence&&) = delete;
    MidiSequence& operator=(MidiSequence&&) = delete;

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    void notifyNotesChanged(int trackIndex);
    void notifyTracksChanged();
    void notifyTempoChanged();
    void notifyTimelineMetadataChanged();
    void notifySequenceReset();

    void clear();

    MidiTrack& addTrack();
    void insertTrack(int index, const MidiTrack& track);
    void removeTrack(int index);

    MidiTrack& getTrack(int index);
    const MidiTrack& getTrack(int index) const;
    int getNumTracks() const;
    bool isAnySolo() const;
    int indexOf(TrackId id) const;
    TrackId resolveRouteTarget(int index) const;

    const TimelineMap& getTimeline() const;
    void setTicksPerQuarterNote(int ppq);
    void setTempoChanges(std::vector<TempoChange> changes);
    void setTimeSignatureChanges(std::vector<TimeSignatureChange> changes);

    KeySignatureChange getKeySignatureAt(int tick) const;
    const std::vector<KeySignatureChange>& getKeySignatureChanges() const;
    void setKeySignatureChanges(std::vector<KeySignatureChange> changes);
    const std::vector<ChordChange>& getChordChanges() const;
    void setChordChanges(std::vector<ChordChange> changes);

private:
    std::vector<MidiTrack> tracks;
    TimelineMap timeline;
    std::vector<KeySignatureChange> keySignatureChanges;
    std::vector<ChordChange> chordChanges;
    std::vector<Listener*> listeners;
    std::uint32_t nextTrackId = 1;
};
