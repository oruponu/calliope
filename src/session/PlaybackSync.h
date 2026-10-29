#pragma once

#include "engine/PlaybackEngine.h"
#include "model/MidiSequence.h"
#include <juce_events/juce_events.h>

class PlaybackSync : private MidiSequence::Listener, private juce::AsyncUpdater
{
public:
    PlaybackSync(PlaybackEngine& engineRef, MidiSequence& sequenceRef);
    ~PlaybackSync() override;

    PlaybackSync(const PlaybackSync&) = delete;
    PlaybackSync& operator=(const PlaybackSync&) = delete;

private:
    void notesChanged(int trackIndex) override;
    void tracksChanged() override;
    void tempoChanged() override;
    void timelineMetadataChanged() override;
    void sequenceReset() override;
    void handleAsyncUpdate() override;

    PlaybackEngine& engine;
    MidiSequence& sequence;
};
