#pragma once

#include "model/MidiTrack.h"
#include <set>
#include <string>
#include <utility>
#include <vector>

struct TempoChange
{
    int tick;
    double bpm;
    bool operator==(const TempoChange&) const = default;
};

struct TimeSignatureChange
{
    int tick;
    int numerator;
    int denominator;
    bool operator==(const TimeSignatureChange&) const = default;
};

struct RelativeTimeSignature
{
    int barOffset;
    int numerator;
    int denominator;
};

struct KeySignatureChange
{
    int tick;
    int sharpsOrFlats; // -7..+7 (negative=flats, positive=sharps)
    bool isMinor;
    bool operator==(const KeySignatureChange&) const = default;
};

struct RelativeKeySignature
{
    int barOffset;
    int sharpsOrFlats;
    bool isMinor;
};

struct ChordChange
{
    int tick;
    int chordRoot; // XF format: upper nibble=accidental(0-6), lower nibble=note(0-7)
    int chordType; // XF format: 0-34
    int bassRoot;  // same as chordRoot, 0x7F=none
    int bassType;  // same as chordType, 0x7F=none
    bool operator==(const ChordChange&) const = default;
};

enum class ChordSpelling
{
    Sharp,
    Flat,
    Mixed
};

struct BarBeatTick
{
    int bar;  // 1-based
    int beat; // 1-based
    int tick; // tick within beat
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

    static int normalizeSharpsOrFlats(int sharpsOrFlats);
    static std::string keySignatureToString(int sharpsOrFlats, bool isMinor);
    static bool keySignatureFromString(const std::string& text, int& sharpsOrFlats, bool& isMinor);
    static std::string chordToString(const ChordChange& chord);

    static constexpr int chordNone = 0x7F;
    static constexpr int chordTypeCount = 34;

    static std::string chordRootToString(int root);
    static bool chordRootFromString(const std::string& text, int& root);
    static std::string chordTypeToString(int type);
    static bool chordTypeFromString(const std::string& text, int& type);
    static int chordRootToSemitone(int root);
    static int semitoneToChordRoot(int semitone, ChordSpelling spelling);
    static ChordSpelling chordSpellingForKeySignature(int sharpsOrFlats);
    static int normalizeChordRoot(int root);
    static int normalizeChordType(int type);
    static int normalizeChordBassRoot(int bassRoot);

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
