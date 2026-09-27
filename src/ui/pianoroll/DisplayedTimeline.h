#pragma once

#include "model/MidiSequence.h"
#include <optional>
#include <vector>

class DisplayedTimeline
{
public:
    void setSequence(const MidiSequence* seq)
    {
        sequence = seq;
        preview.reset();
    }

    void setTimeSignaturePreview(const std::vector<TimeSignatureChange>* changes)
    {
        if (changes == nullptr || sequence == nullptr)
        {
            preview.reset();
            return;
        }
        preview = sequence->getTimeline();
        preview->setTimeSignatureChanges(*changes);
    }

    const TimelineMap& get() const { return preview ? *preview : sequence->getTimeline(); }

private:
    const MidiSequence* sequence = nullptr;
    std::optional<TimelineMap> preview;
};
