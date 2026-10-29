#pragma once

#include "model/MidiSequence.h"
#include <functional>
#include <juce_events/juce_events.h>

class MidiThru;

// Keeps the thru target on the active track. Recomputes on the next message loop pass: track edits notify
// inside the undoable action, before the track list has re-pointed its active index.
class MidiThruSync : private MidiSequence::Listener, private juce::AsyncUpdater
{
public:
    MidiThruSync(MidiSequence& sequenceRef, MidiThru& thruRef, std::function<int()> activeTrackIndexSource);
    ~MidiThruSync() override;

    MidiThruSync(const MidiThruSync&) = delete;
    MidiThruSync& operator=(const MidiThruSync&) = delete;

    void activeTrackChanged();

private:
    void tracksChanged() override;
    void sequenceReset() override;
    void handleAsyncUpdate() override;

    MidiSequence& sequence;
    MidiThru& thru;
    std::function<int()> activeTrackIndex;
};
