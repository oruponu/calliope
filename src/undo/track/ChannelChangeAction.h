#pragma once

#include "model/MidiSequence.h"
#include <functional>
#include <juce_data_structures/juce_data_structures.h>

class ChannelChangeAction : public juce::UndoableAction
{
public:
    ChannelChangeAction(MidiSequence* seq, int trackIndex, int oldChannel, int newChannel,
                        std::function<void(int)> beforeChange)
        : sequence(seq), trackIdx(trackIndex), oldCh(oldChannel), newCh(newChannel),
          beforeChange(std::move(beforeChange))
    {
    }

    bool perform() override
    {
        if (beforeChange)
            beforeChange(trackIdx);
        sequence->setTrackChannel(trackIdx, newCh);
        return true;
    }

    bool undo() override
    {
        if (beforeChange)
            beforeChange(trackIdx);
        sequence->setTrackChannel(trackIdx, oldCh);
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    int trackIdx;
    int oldCh;
    int newCh;
    std::function<void(int)> beforeChange;
};
