#pragma once

#include "engine/recording/HeldTakeNote.h"
#include "engine/recording/Take.h"
#include <vector>

// The recording segment in progress, read on the message thread by playback and the editors.
class RecordingLayer
{
public:
    virtual ~RecordingLayer() = default;
    virtual const Take& getTake() const = 0;
    virtual std::vector<HeldTakeNote> getHeldNotes() const = 0;
};
