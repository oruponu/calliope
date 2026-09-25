#pragma once

#include "model/BarBeatTick.h"
#include "model/ChordChange.h"
#include "model/KeySignatureChange.h"
#include "model/MidiTrack.h"
#include "model/TempoChange.h"
#include "model/TimeSignatureChange.h"
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

    static constexpr int defaultTicksPerQuarterNote = 480;
    static constexpr double minBpm = 10.0;
    static constexpr double maxBpm = 400.0;

    MidiSequence();

    void clear();

    MidiTrack& addTrack();
    void insertTrack(int index, const MidiTrack& track);
    void removeTrack(int index);

    MidiTrack& getTrack(int index);
    const MidiTrack& getTrack(int index) const;
    int getNumTracks() const;
    bool isAnySolo() const;

    void setBpm(double newBpm);
    double getBpm() const;
    int getTicksPerQuarterNote() const;
    void setTicksPerQuarterNote(int ppq);

    double getTempoAt(int tick) const;
    TempoChange getTempoChangeAt(int tick) const;
    TimeSignatureChange getTimeSignatureAt(int tick) const;

    KeySignatureChange getKeySignatureAt(int tick) const;

    const std::vector<TempoChange>& getTempoChanges() const;
    const std::vector<TimeSignatureChange>& getTimeSignatureChanges() const;
    const std::vector<KeySignatureChange>& getKeySignatureChanges() const;
    const std::vector<ChordChange>& getChordChanges() const;

    int addTempoChange(int tick, double bpm);
    void addTimeSignatureChange(int tick, int num, int den);
    void addKeySignatureChange(int tick, int sharpsOrFlats, bool isMinor);
    void addChordChange(int tick, int chordRoot, int chordType, int bassRoot, int bassType);

    void setTempoChanges(std::vector<TempoChange> changes);
    void setTimeSignatureChanges(std::vector<TimeSignatureChange> changes);
    void setKeySignatureChanges(std::vector<KeySignatureChange> changes);
    void setChordChanges(std::vector<ChordChange> changes);

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

    double ticksToSeconds(int ticks) const;
    int secondsToTicks(double seconds) const;

    BarBeatTick tickToBarBeatTick(int tick) const;
    int barStartToTick(int barNumber) const;
    int barBeatTickToTick(int bar, int beat, int tickInBeat) const;

private:
    std::vector<MidiTrack> tracks;
    std::vector<TempoChange> tempoChanges;
    std::vector<TimeSignatureChange> timeSignatureChanges;
    std::vector<KeySignatureChange> keySignatureChanges;
    std::vector<ChordChange> chordChanges;
    int ticksPerQuarterNote = defaultTicksPerQuarterNote;
    std::vector<Listener*> listeners;
};
