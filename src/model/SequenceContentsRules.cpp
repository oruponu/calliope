#include "model/SequenceContentsRules.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <vector>

namespace
{
constexpr int intMax = std::numeric_limits<int>::max();

bool inRange(int value, int min, int max)
{
    return value >= min && value <= max;
}

template <typename T> bool ticksStrictlyIncrease(const std::vector<T>& changes)
{
    return std::ranges::adjacent_find(changes, [](const T& a, const T& b) { return a.tick >= b.tick; }) ==
           changes.end();
}

// MIDI import snaps a mid-bar time signature to its bar start, so two changes can share a tick.
template <typename T> bool ticksNeverDecrease(const std::vector<T>& changes)
{
    return std::ranges::adjacent_find(changes, [](const T& a, const T& b) { return a.tick > b.tick; }) == changes.end();
}

template <typename T> bool startsAtZero(const std::vector<T>& changes)
{
    return !changes.empty() && changes.front().tick == 0;
}

template <typename T> bool startsAtOrAfterZero(const std::vector<T>& changes)
{
    return changes.empty() || changes.front().tick >= 0;
}

bool acceptsTempo(const TempoChange& change)
{
    return std::isfinite(change.bpm) && change.bpm > 0.0;
}

bool acceptsTimeSignature(const TimeSignatureChange& change, int ppq)
{
    if (change.numerator < 1 || change.denominator < 1 ||
        !std::has_single_bit(static_cast<unsigned>(change.denominator)))
        return false;

    // TimelineMap measures a beat as ppq * 4 / denominator ticks and a bar as that times the numerator, in int.
    const long long ticksPerBeat = 4LL * ppq / change.denominator;
    return ticksPerBeat >= 1 && ticksPerBeat <= intMax / change.numerator;
}

bool acceptsTimeline(const TimelineMap& timeline)
{
    const int ppq = timeline.getTicksPerQuarterNote();
    const auto& tempos = timeline.getTempoChanges();
    const auto& signatures = timeline.getTimeSignatureChanges();
    return ppq > 0 && ppq <= intMax / 4 && startsAtZero(tempos) && ticksStrictlyIncrease(tempos) &&
           std::ranges::all_of(tempos, acceptsTempo) && startsAtZero(signatures) && ticksNeverDecrease(signatures) &&
           std::ranges::all_of(signatures,
                               [ppq](const TimeSignatureChange& c) { return acceptsTimeSignature(c, ppq); });
}

bool acceptsKeySignatures(const std::vector<KeySignatureChange>& changes)
{
    return startsAtOrAfterZero(changes) && ticksStrictlyIncrease(changes) &&
           std::ranges::all_of(changes, [](const KeySignatureChange& c) { return inRange(c.sharpsOrFlats, -7, 7); });
}

bool acceptsChord(const ChordChange& change)
{
    return inRange(change.chordRoot, 0, 255) && inRange(change.chordType, 0, 255) && inRange(change.bassRoot, 0, 255) &&
           inRange(change.bassType, 0, 255);
}

bool acceptsChords(const std::vector<ChordChange>& changes)
{
    return startsAtOrAfterZero(changes) && ticksStrictlyIncrease(changes) && std::ranges::all_of(changes, acceptsChord);
}

bool acceptsNote(const MidiNote& note)
{
    return inRange(note.noteNumber, 0, 127) && inRange(note.velocity, 0, 127) && note.startTick >= 0 &&
           note.duration >= 0 && note.duration <= intMax - note.startTick;
}

bool acceptsEvent(const MidiEvent& event)
{
    const int data1Max = event.type == MidiEvent::Type::PitchBend ? 16383 : 127;
    return event.tick >= 0 && inRange(event.data1, 0, data1Max) && inRange(event.data2, 0, 127);
}

bool acceptsTrack(const MidiTrack& track)
{
    return inRange(track.getChannel(), 1, 16) && std::ranges::all_of(track.getNotes(), acceptsNote) &&
           std::ranges::all_of(track.getEvents(), acceptsEvent);
}
} // namespace

namespace SequenceContentsRules
{
bool accepts(const SequenceContents& contents)
{
    return acceptsTimeline(contents.timeline) && acceptsKeySignatures(contents.keySignatureChanges) &&
           acceptsChords(contents.chordChanges) && std::ranges::all_of(contents.tracks, acceptsTrack);
}
} // namespace SequenceContentsRules
