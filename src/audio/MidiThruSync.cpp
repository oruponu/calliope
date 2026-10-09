#include "audio/MidiThruSync.h"
#include "audio/MidiThru.h"
#include "engine/ThruTarget.h"
#include <utility>

MidiThruSync::MidiThruSync(MidiSequence& sequenceRef, MidiThru& thruRef, std::function<int()> activeTrackIndexSource)
    : sequence(sequenceRef), thru(thruRef), activeTrackIndex(std::move(activeTrackIndexSource))
{
    sequence.addListener(this);
    triggerAsyncUpdate();
}

MidiThruSync::~MidiThruSync()
{
    sequence.removeListener(this);
    cancelPendingUpdate();
}

void MidiThruSync::activeTrackChanged()
{
    triggerAsyncUpdate();
}

void MidiThruSync::tracksChanged()
{
    triggerAsyncUpdate();
}

void MidiThruSync::sequenceReset()
{
    triggerAsyncUpdate();
}

void MidiThruSync::handleAsyncUpdate()
{
    thru.setTarget(resolveThruTarget(sequence, activeTrackIndex()));
}
