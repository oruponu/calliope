#pragma once

#include "model/ChordChange.h"
#include "model/KeySignatureChange.h"
#include "model/MidiTrack.h"
#include "model/SequenceContents.h"
#include "model/TimelineMap.h"
#include "model/TrackId.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <string>
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

    // Holds notifications until the outermost batch ends, then sends each kind once.
    class ChangeBatch
    {
    public:
        explicit ChangeBatch(MidiSequence& sequenceRef);
        ~ChangeBatch();

        ChangeBatch(const ChangeBatch&) = delete;
        ChangeBatch& operator=(const ChangeBatch&) = delete;

    private:
        MidiSequence& sequence;
    };

    MidiSequence() = default;
    MidiSequence(const MidiSequence&) = delete;
    MidiSequence& operator=(const MidiSequence&) = delete;
    MidiSequence(MidiSequence&&) = delete;
    MidiSequence& operator=(MidiSequence&&) = delete;

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    void replaceContents(SequenceContents contents);

    const MidiTrack& addTrack(MidiTrack track = {});
    void insertTrack(int index, const MidiTrack& track);
    void removeTrack(int index);

    const MidiTrack& getTrack(int index) const;
    int getNumTracks() const;
    bool isAnySolo() const;
    int indexOf(TrackId id) const;
    TrackId resolveRouteTarget(int index) const;

    void setTrackMuted(int index, bool muted);
    void setTrackSolo(int index, bool solo);
    void setTrackName(int index, const std::string& name);
    void setTrackChannel(int index, int channel);
    void setTrackOutputDestination(int index, MidiTrack::OutputDestination destination);
    void setTrackRouteTarget(int index, std::optional<TrackId> target);
    void setTrackPluginAssignment(int index, std::shared_ptr<const PluginAssignment> assignment);

    void addNote(int trackIndex, const MidiNote& note);
    void insertNote(int trackIndex, int noteIndex, const MidiNote& note);
    void removeNote(int trackIndex, int noteIndex);
    void setNote(int trackIndex, int noteIndex, const MidiNote& note);

    const TimelineMap& getTimeline() const;
    void setTempoChanges(std::vector<TempoChange> changes);
    void setTimeSignatureChanges(std::vector<TimeSignatureChange> changes);

    KeySignatureChange getKeySignatureAt(int tick) const;
    const std::vector<KeySignatureChange>& getKeySignatureChanges() const;
    void setKeySignatureChanges(std::vector<KeySignatureChange> changes);
    const std::vector<ChordChange>& getChordChanges() const;
    void setChordChanges(std::vector<ChordChange> changes);

private:
    struct PendingNotifications
    {
        std::set<int> noteTracks;
        bool tracks = false;
        bool trackStructure = false;
        bool tempo = false;
        bool timelineMetadata = false;
        bool reset = false;
    };

    void notifyNotesChanged(int trackIndex);
    void notifyTracksChanged();
    void notifyTempoChanged();
    void notifyTimelineMetadataChanged();
    void notifySequenceReset();

    void notifyTrackStructureChanged();
    void flushPendingNotifications();
    template <typename Callback> void forEachListener(Callback callback);

    std::vector<MidiTrack> tracks;
    TimelineMap timeline;
    std::vector<KeySignatureChange> keySignatureChanges;
    std::vector<ChordChange> chordChanges;
    std::vector<Listener*> listeners;
    std::uint32_t nextTrackId = 1;
    int batchDepth = 0;
    PendingNotifications pending;
};
