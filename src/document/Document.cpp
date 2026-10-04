#include "document/Document.h"
#include "AppProperties.h"
#include "io/MidiFileIO.h"
#include "io/ProjectFileIO.h"
#include <utility>

namespace
{
constexpr const char* kLastDocumentKey = "lastDocumentFile";
}

Document::Document() : juce::FileBasedDocument(".calliope", "*.calliope", "Open Project", "Save Project")
{
    history.onChanged = [this] { updateChangedFlag(); };
    sequence.addListener(this);
}

Document::~Document()
{
    sequence.removeListener(this);
}

void Document::newDocument()
{
    notifyWillReplaceSequence();
    SequenceContents contents;
    contents.tracks.emplace_back();
    sequence.replaceContents(std::move(contents));
    setFile({});
    history.clear();
    changedOutsideHistory = false;
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
    changedOutsideHistory = false;
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
    auto contents = ProjectFileIO::load(file);
    if (!contents)
        return juce::Result::fail("The file could not be read as a Calliope project.");
    sequence.replaceContents(std::move(*contents));
    history.clear();
    changedOutsideHistory = false;
    history.markSaved();
    return juce::Result::ok();
}

juce::Result Document::saveDocument(const juce::File& file)
{
    if (onWillSave)
        onWillSave();
    if (!ProjectFileIO::save(sequence, pluginStateSource, file))
        return juce::Result::fail("The project could not be written.");
    changedOutsideHistory = false;
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

void Document::markChanged()
{
    changedOutsideHistory = true;
    updateChangedFlag();
}

void Document::notesChanged(int)
{
    noteChangeOutsideHistory();
}

void Document::tracksChanged()
{
    noteChangeOutsideHistory();
}

void Document::tempoChanged()
{
    noteChangeOutsideHistory();
}

void Document::timelineMetadataChanged()
{
    noteChangeOutsideHistory();
}

void Document::noteChangeOutsideHistory()
{
    // Changes made through the undo history are tracked by its save point instead. A ChangeBatch must not be
    // opened around an UndoHistory call, or its deferred notification would arrive after performing ends.
    if (!history.isPerforming())
        markChanged();
}

void Document::updateChangedFlag()
{
    setChangedFlag(changedOutsideHistory || !history.isAtSavePoint());
}
