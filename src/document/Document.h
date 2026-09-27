#pragma once

#include "model/MidiSequence.h"
#include "undo/UndoHistory.h"
#include <juce_core/juce_core.h>

class Document
{
public:
    Document() = default;

    void newDocument();
    bool loadFrom(const juce::File& file);
    bool saveTo(const juce::File& file);

    MidiSequence& getSequence() { return sequence; }
    const MidiSequence& getSequence() const { return sequence; }
    UndoHistory& getHistory() { return history; }
    const juce::File& getCurrentFile() const { return currentFile; }

private:
    MidiSequence sequence;
    UndoHistory history;
    juce::File currentFile;

    JUCE_DECLARE_NON_COPYABLE(Document)
};
