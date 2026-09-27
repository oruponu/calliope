#pragma once

#include "model/MidiSequence.h"
#include "undo/UndoHistory.h"
#include <functional>
#include <juce_gui_extra/juce_gui_extra.h>

class Document : public juce::FileBasedDocument
{
public:
    Document();

    void newDocument();

    MidiSequence& getSequence() { return sequence; }
    const MidiSequence& getSequence() const { return sequence; }
    UndoHistory& getHistory() { return history; }

    juce::String getDocumentTitle() override;

    std::function<void()> onWillReplaceSequence;

protected:
    juce::Result loadDocument(const juce::File& file) override;
    juce::Result saveDocument(const juce::File& file) override;
    juce::File getLastDocumentOpened() override;
    void setLastDocumentOpened(const juce::File& file) override;

private:
    void notifyWillReplaceSequence();

    MidiSequence sequence;
    UndoHistory history;

    JUCE_DECLARE_NON_COPYABLE(Document)
};
