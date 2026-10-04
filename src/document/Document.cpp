#include "document/Document.h"
#include "AppProperties.h"
#include "io/MidiFileIO.h"
#include <utility>

namespace
{
constexpr const char* kLastDocumentKey = "lastDocumentFile";
}

Document::Document() : juce::FileBasedDocument(".mid", "*.mid;*.midi", "Open MIDI File", "Save MIDI File")
{
    history.onChanged = [this] { setChangedFlag(!history.isAtSavePoint()); };
}

void Document::newDocument()
{
    notifyWillReplaceSequence();
    SequenceContents contents;
    contents.tracks.emplace_back();
    sequence.replaceContents(std::move(contents));
    setFile({});
    history.clear();
    history.markSaved();
}

bool Document::importMidi(const juce::File& file)
{
    notifyWillReplaceSequence();
    auto contents = MidiFileIO::load(file);
    if (!contents)
        return false;
    sequence.replaceContents(std::move(*contents));
    setFile({});
    history.clear();
    history.markSaved();
    return true;
}

bool Document::exportMidi(const juce::File& file) const
{
    return MidiFileIO::save(sequence, file);
}

juce::String Document::getDocumentTitle()
{
    return getFile() == juce::File{} ? juce::String("Untitled") : getFile().getFileName();
}

juce::Result Document::loadDocument(const juce::File& file)
{
    notifyWillReplaceSequence();
    auto contents = MidiFileIO::load(file);
    if (!contents)
        return juce::Result::fail("The file could not be read as a MIDI file.");
    sequence.replaceContents(std::move(*contents));
    history.clear();
    history.markSaved();
    return juce::Result::ok();
}

juce::Result Document::saveDocument(const juce::File& file)
{
    if (!MidiFileIO::save(sequence, file))
        return juce::Result::fail("The MIDI file could not be written.");
    history.markSaved();
    return juce::Result::ok();
}

juce::File Document::getLastDocumentOpened()
{
    const auto path = getAppProperties().getUserSettings()->getValue(kLastDocumentKey);
    return path.isEmpty() ? juce::File{} : juce::File(path);
}

void Document::setLastDocumentOpened(const juce::File& file)
{
    getAppProperties().getUserSettings()->setValue(kLastDocumentKey, file.getFullPathName());
}

void Document::notifyWillReplaceSequence()
{
    if (onWillReplaceSequence)
        onWillReplaceSequence();
}
