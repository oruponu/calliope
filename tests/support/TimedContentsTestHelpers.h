#pragma once

#include "model/SequenceContents.h"
#include "model/TimedContents.h"
#include <utility>

namespace TimedContentsTest
{
inline SequenceContents toSequenceContents(const TimedContents& timed)
{
    SequenceContents contents;
    contents.timeline.setTicksPerQuarterNote(timed.ticksPerQuarterNote);
    contents.timeline.setTempoChanges(timed.tempoChanges);
    contents.timeline.setTimeSignatureChanges(timed.timeSignatureChanges);
    contents.keySignatureChanges = timed.keySignatureChanges;
    contents.chordChanges = timed.chordChanges;
    contents.metaEvents = timed.metaEvents;
    for (const auto& source : timed.tracks)
    {
        MidiTrack track;
        for (const auto& note : source.notes)
            track.addNote(note);
        for (const auto& event : source.events)
            track.addEvent(event);
        for (const auto& sysEx : source.sysExEvents)
            track.addSysEx(sysEx);
        for (const auto& meta : source.metaEvents)
            track.addMetaEvent(meta);
        contents.tracks.push_back(std::move(track));
    }
    return contents;
}
} // namespace TimedContentsTest
