#pragma once

#include "io/project/ProjectXml.h"
#include "io/project/RecoveryFileIO.h"
#include "model/MidiSequence.h"
#include "undo/UndoHistory.h"
#include <cstdint>
#include <functional>
#include <juce_gui_extra/juce_gui_extra.h>

class Document : public juce::FileBasedDocument, private MidiSequence::Listener
{
public:
    Document();
    ~Document() override;

    void newDocument();
    bool importMidi(const juce::File& file);
    bool exportMidi(const juce::File& file) const;
    void markChanged();
    // Writes the song without marking it saved.
    bool writeRecoveryFile(const juce::File& file);
    // Opens the song unsaved under its original file name, with an empty undo history.
    void restoreRecovered(RecoveredProject recovered);

    // Increases whenever the song may have changed, including undo and redo.
    std::uint64_t getRevision() const { return revision; }
    MidiSequence& getSequence() { return sequence; }
    const MidiSequence& getSequence() const { return sequence; }
    UndoHistory& getHistory() { return history; }

    juce::String getDocumentTitle() override;

    std::function<void()> onWillReplaceSequence;
    // Called after a new, imported or opened song has replaced the previous one, but not after restoreRecovered().
    std::function<void()> onDidReplaceSequence;
    ProjectXml::PluginStateSource pluginStateSource;
    std::function<void()> onWillSave;

protected:
    juce::Result loadDocument(const juce::File& file) override;
    juce::Result saveDocument(const juce::File& file) override;
    juce::File getLastDocumentOpened() override;
    void setLastDocumentOpened(const juce::File& file) override;

private:
    void notesChanged(int trackIndex) override;
    void tracksChanged() override;
    void tempoChanged() override;
    void timelineMetadataChanged() override;
    void notifyWillReplaceSequence();
    void notifyDidReplaceSequence();
    void noteChangeOutsideHistory();
    void updateChangedFlag();

    bool changedOutsideHistory = false;
    std::uint64_t revision = 0;
    MidiSequence sequence;
    UndoHistory history;

    JUCE_DECLARE_NON_COPYABLE(Document)
};
