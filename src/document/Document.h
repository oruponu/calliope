#pragma once

#include "io/ProjectXml.h"
#include "model/MidiSequence.h"
#include "undo/UndoHistory.h"
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

    MidiSequence& getSequence() { return sequence; }
    const MidiSequence& getSequence() const { return sequence; }
    UndoHistory& getHistory() { return history; }

    juce::String getDocumentTitle() override;

    std::function<void()> onWillReplaceSequence;
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
    void noteChangeOutsideHistory();
    void updateChangedFlag();

    bool changedOutsideHistory = false;
    MidiSequence sequence;
    UndoHistory history;

    JUCE_DECLARE_NON_COPYABLE(Document)
};
