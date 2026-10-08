#include "engine/PlaybackSync.h"

PlaybackSync::PlaybackSync(PlaybackEngine& engineRef, MidiSequence& sequenceRef)
    : engine(engineRef), sequence(sequenceRef)
{
    sequence.addListener(this);
}

PlaybackSync::~PlaybackSync()
{
    sequence.removeListener(this);
    cancelPendingUpdate();
}

void PlaybackSync::notesChanged(int)
{
    triggerAsyncUpdate();
}

void PlaybackSync::tracksChanged()
{
    triggerAsyncUpdate();
}

void PlaybackSync::tempoChanged()
{
    triggerAsyncUpdate();
}

void PlaybackSync::timelineMetadataChanged()
{
    triggerAsyncUpdate();
}

void PlaybackSync::sequenceReset()
{
    triggerAsyncUpdate();
}

void PlaybackSync::handleAsyncUpdate()
{
    engine.rebuildSnapshot();
}
