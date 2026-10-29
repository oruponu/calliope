#pragma once

#include "engine/PassBoundary.h"
#include "engine/PlaybackClock.h"
#include "engine/PlaybackProgress.h"
#include "engine/RecordedNoteEdit.h"
#include "engine/Take.h"
#include "model/event/MidiNote.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

class TimelineGrid;

// Turns one recording segment's input into notes and events. Message thread only.
class TakeBuilder
{
public:
    void begin(std::optional<TrackId> target, ClockPosition start);
    // With a grid, a note starts where quantizeInputStart puts it and keeps the length it is played with. It stays
    // pending, neither shown, edited nor recorded, until reach() learns that playback has sent its start.
    void handle(const std::string& sourceId, const juce::MidiMessage& message, ClockPosition at,
                const TimelineGrid* grid = nullptr, LoopRange loop = {});
    // Count-in input waits without a tick until endCountIn. A key released meanwhile is dropped, and of everything
    // else only the last value of each controller, program, pitch bend and pressure (per key for polyphonic pressure)
    // is kept, per source and input channel.
    void collectCountIn(const std::string& sourceId, const juce::MidiMessage& message);
    // Handles the kept values, in the order they arrived, then the keys still held, at startTick.
    void endCountIn(int startTick, const TimelineGrid* grid = nullptr, LoopRange loop = {});
    // Playback has sent every event before `sent` (PlaybackProgress::sent). Pending notes that start before it stop
    // pending; a released one joins the take, to sound from the next loop pass. Returns whether any note did.
    bool reach(ClockPosition sent);
    // Playback ended these passes at these ticks (PlaybackProgress::passEnds). Pending notes left from a wrap announced
    // before playback took it join the take if they start before the end of their pass, and are dropped otherwise.
    // Returns whether any note joined.
    bool reachPassEnds(const std::vector<ClockPosition>& passEnds);
    void setTarget(std::optional<TrackId> newTarget, int tick);
    // `progress` is the one handed over with the boundary. Pending notes playback has not sent are dropped, except a
    // note carried over the loop end when this is a loop wrap: it starts at the boundary's startTick. A wrap that input
    // announced before playback took it keeps this pass's pending notes, held ones ended at closeTick, until
    // reachPassEnds learns how far playback sent the pass.
    void closeHeld(const PassBoundary& boundary, const PlaybackProgress& progress);
    void releaseSource(const std::string& sourceId, int tick);
    Take end(int tick);
    // Edits recorded notes by id and ignores ids no longer recorded or still pending. A held note stays held unless the
    // edit sets its end or moves its start to playheadTick or later; it then ends, and releasing its key does nothing.
    void editNotes(const std::vector<RecordedNoteEdit>& edits, int playheadTick);
    void removeNotes(const std::vector<int>& ids);

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
    // (source, input channel, MidiEvent::Type, controller number or note number, or 0)
    using CountInValueKey = std::tuple<std::string, int, int, int>;

    struct CountInValue
    {
        CountInValueKey key;
        juce::MidiMessage message;
    };

    struct HeldNote
    {
        TrackId track{};
        // Follows pitch edits; the NoteKey keeps the key that was pressed.
        int noteNumber = 0;
        int velocity = 0;
        // Meaningless for a note carried over the loop end until the loop wraps.
        int startTick = 0;
        int id = 0;
        // One past the builder's pass for a note carried over the loop end.
        int pass = 0;
        int playedTick = 0;
        // Added to the release tick to keep the length the key was held. An edit of the start drops it.
        int endOffset = 0;
        bool pending = false;
    };
    using HeldNotes = std::map<NoteKey, HeldNote>;

    // A released note still pending.
    struct PendingNote
    {
        std::string sourceId;
        TrackId track{};
        // Its start is meaningless for a note carried over the loop end until the loop wraps.
        MidiNote note;
        int id = 0;
        // Behind the builder's pass while waiting for reachPassEnds.
        int pass = 0;
    };

    struct RecordedPedal
    {
        int value = 0;
        int tick = 0;
    };

    void finishNote(const NoteKey& key, int tick);
    // Ends a note for a reason other than its key: at `tick`, without keeping the played length. Drops a pending note.
    void closeNote(HeldNotes::iterator it, int tick);
    void closeAllNotes(int tick);
    void endHeldNote(HeldNotes::iterator it, int startTick, int duration);
    void addNote(TrackId track, const MidiNote& note, int id);
    bool hasReached(int notePass, int startTick, ClockPosition sent) const;
    void editHeldNote(HeldNotes::iterator it, const NoteFieldEdit& edit, int playheadTick);
    HeldNotes::iterator findHeld(int id);
    MidiNote* findFinished(int id);
    void releasePedal(TrackId track, int controller, int tick);
    void releaseAllPedals(int tick);
    void addEvent(TrackId track, const MidiEvent& event);

    std::optional<TrackId> target;
    int pass = 0;
    HeldNotes heldNotes;
    std::vector<PendingNote> releasedPending;
    // Tracked with or without a target, since the pedal is physically down either way.
    std::set<PedalKey> pedalsDown;
    // The last hold pedal value, and its tick, recorded on each (track, controller) in this segment.
    std::map<std::pair<TrackId, int>, RecordedPedal> recordedPedals;
    // The note-on of each key held during the count-in.
    std::map<NoteKey, juce::MidiMessage> countInNotes;
    // In the order their latest values arrived.
    std::vector<CountInValue> countInValues;
    Take take;
    int contentRevision = 0;
    int noteRevision = 0;
    // Never reset, so a reference left from an earlier segment cannot reach a note of a later one.
    int nextNoteId = 1;
};
