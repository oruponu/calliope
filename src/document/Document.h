#pragma once

#include "../model/MidiSequence.h"
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

class Document
{
public:
    Document() = default;

    void newDocument();
    bool loadFrom(const juce::File& file);
    bool saveTo(const juce::File& file);

    MidiSequence& getSequence() { return sequence; }
    const MidiSequence& getSequence() const { return sequence; }
    juce::UndoManager& getUndoManager() { return undoManager; }
    const juce::File& getCurrentFile() const { return currentFile; }

private:
    MidiSequence sequence;
    juce::UndoManager undoManager{10000, 100};
    juce::File currentFile;

    JUCE_DECLARE_NON_COPYABLE(Document)
};
