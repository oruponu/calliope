#include "Document.h"

#include "../io/MidiFileIO.h"

void Document::newDocument()
{
    sequence.clear();
    sequence.addTrack();
    currentFile = juce::File{};
    undoManager.clearUndoHistory();
}

bool Document::loadFrom(const juce::File& file)
{
    if (!MidiFileIO::load(sequence, file))
        return false;
    currentFile = file;
    undoManager.clearUndoHistory();
    return true;
}

bool Document::saveTo(const juce::File& file)
{
    if (!MidiFileIO::save(sequence, file))
        return false;
    currentFile = file;
    return true;
}
