#include "edit/NoteEdits.h"
#include <algorithm>

MidiNote NoteEdits::afterStartResize(const MidiNote& note, int deltaTick, int minDuration)
{
    const int endTick = note.endTick();
    MidiNote resized = note;
    resized.startTick = std::clamp(note.startTick + deltaTick, 0, std::max(0, endTick - minDuration));
    resized.duration = endTick - resized.startTick;
    return resized;
}
