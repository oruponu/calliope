#include "document/Document.h"
#include "io/MidiFileIO.h"

void Document::newDocument()
{
    sequence.clear();
    sequence.addTrack();
    currentFile = juce::File{};
    history.clear();
}

bool Document::loadFrom(const juce::File& file)
{
    if (!MidiFileIO::load(sequence, file))
        return false;
    currentFile = file;
    history.clear();
    return true;
}

bool Document::saveTo(const juce::File& file)
{
    if (!MidiFileIO::save(sequence, file))
        return false;
    currentFile = file;
    return true;
}
