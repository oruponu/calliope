#pragma once

#include "model/ChordChange.h"
#include "model/KeySignatureChange.h"
#include "model/MidiTrack.h"
#include "model/TimelineMap.h"
#include <set>
#include <utility>
#include <vector>

struct RelativeTimeSignature
{
    int barOffset;
    int numerator;
    int denominator;
};

struct RelativeKeySignature
{
    int barOffset;
    int sharpsOrFlats;
    bool isMinor;
};

struct RelativeChord
{
    int tickOffset;
    int length;
    int chordRoot;
    int chordType;
    int bassRoot;
    int bassType;
};

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

    const TimelineMap& getTimeline() const;
    void setTicksPerQuarterNote(int ppq);
    void setTempoChanges(std::vector<TempoChange> changes);
    void setTimeSignatureChanges(std::vector<TimeSignatureChange> changes);

    KeySignatureChange getKeySignatureAt(int tick) const;
    const std::vector<KeySignatureChange>& getKeySignatureChanges() const;
    void setKeySignatureChanges(std::vector<KeySignatureChange> changes);
    const std::vector<ChordChange>& getChordChanges() const;
    void setChordChanges(std::vector<ChordChange> changes);

    int addTempoChange(int tick, double bpm);
    void addTimeSignatureChange(int tick, int num, int den);
    void addKeySignatureChange(int tick, int sharpsOrFlats, bool isMinor);
    void addChordChange(int tick, int chordRoot, int chordType, int bassRoot, int bassType);

    static std::vector<TimeSignatureChange>
    buildTimeSignatureChangesAfterMove(const std::vector<TimeSignatureChange>& before,
                                       const std::vector<int>& movedIndices, int anchorIndex, int targetTick, int ppq);

    static std::vector<TimeSignatureChange>
    buildTimeSignatureChangesAfterDelete(const std::vector<TimeSignatureChange>& before,
                                         const std::set<int>& deletedIndices, int ppq);

    static std::vector<TimeSignatureChange>
    buildTimeSignatureChangesAfterPaste(const std::vector<TimeSignatureChange>& before,
                                        const std::vector<RelativeTimeSignature>& items, int anchorBar, int ppq);

    std::vector<KeySignatureChange> buildKeySignatureChangesAfterMove(const std::vector<KeySignatureChange>& before,
                                                                      const std::vector<int>& movedIndices,
                                                                      int anchorIndex, int targetTick) const;

    std::pair<int, int> chordAddSpanAt(int tick) const;

    static std::vector<ChordChange> buildChordChangesAfterAdd(const std::vector<ChordChange>& before, int startTick,
                                                              int endTick, int chordRoot, int chordType, int bassRoot,
                                                              int bassType);
    static std::vector<ChordChange> buildChordChangesAfterResize(const std::vector<ChordChange>& before, int chordIndex,
                                                                 int targetEndTick, int gridTicks);
    static std::vector<ChordChange> buildChordChangesAfterMove(const std::vector<ChordChange>& before,
                                                               const std::vector<int>& movedIndices, int anchorIndex,
                                                               int targetTick, int gridTicks);
    static std::vector<ChordChange> buildChordChangesAfterStartResize(const std::vector<ChordChange>& before,
                                                                      int chordIndex, int targetStartTick,
                                                                      int gridTicks);
    static std::vector<ChordChange> buildChordChangesAfterDelete(const std::vector<ChordChange>& before,
                                                                 const std::vector<int>& deletedIndices);
    static std::vector<ChordChange> buildChordChangesAfterPaste(const std::vector<ChordChange>& before,
                                                                const std::vector<RelativeChord>& items,
                                                                int anchorTick);

private:
    std::vector<MidiTrack> tracks;
    TimelineMap timeline;
    std::vector<KeySignatureChange> keySignatureChanges;
    std::vector<ChordChange> chordChanges;
    std::vector<Listener*> listeners;
};
