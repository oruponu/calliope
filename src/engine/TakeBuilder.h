#pragma once

#include "engine/PlaybackClock.h"
#include "engine/Take.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// Turns one recording segment's input into notes and events. Message thread only.
class TakeBuilder
{
public:
    void begin(std::optional<TrackId> target, ClockPosition start);
    void handle(const std::string& sourceId, const juce::MidiMessage& message, ClockPosition at);
    void setTarget(std::optional<TrackId> newTarget, int tick);
    void closeHeld(int pass, int closeTick);
    void releaseSource(const std::string& sourceId, int tick);
    Take end(int tick);

    const Take& getTake() const { return take; }
    std::vector<HeldTakeNote> getHeldNotes() const;
    // Changes whenever finished notes or events change, so the caller knows playback needs the new take.
    int getContentRevision() const { return contentRevision; }
    // Changes only when finished notes change; those must reach playback before the next loop pass reaches them.
    int getNoteRevision() const { return noteRevision; }

private:
    // (source, input channel, note number)
    using NoteKey = std::tuple<std::string, int, int>;
    // (source, input channel, controller number)
    using PedalKey = std::tuple<std::string, int, int>;

    struct HeldNote
    {
        TrackId track{};
        int velocity = 0;
        int startTick = 0;
    };

    struct RecordedPedal
    {
        int value = 0;
        int tick = 0;
    };

    void finishNote(const NoteKey& key, int tick);
    void finishAllNotes(int tick);
    void releasePedal(TrackId track, int controller, int tick);
    void releaseAllPedals(int tick);
    void addEvent(TrackId track, const MidiEvent& event);

    std::optional<TrackId> target;
    int pass = 0;
    std::map<NoteKey, HeldNote> heldNotes;
    // Tracked with or without a target, since the pedal is physically down either way.
    std::set<PedalKey> pedalsDown;
    // The last hold pedal value, and its tick, recorded on each (track, controller) in this segment.
    std::map<std::pair<TrackId, int>, RecordedPedal> recordedPedals;
    Take take;
    int contentRevision = 0;
    int noteRevision = 0;
};
