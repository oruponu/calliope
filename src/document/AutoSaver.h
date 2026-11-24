#pragma once

#include "document/AutoSavePolicy.h"
#include "document/Document.h"
#include <functional>
#include <juce_events/juce_events.h>

class AutoSaver : private juce::Timer, private juce::ChangeListener
{
public:
    AutoSaver(Document& document, juce::File recoveryFile, std::function<bool()> isBusy);
    ~AutoSaver() override;

    void start();
    // Discards the previous song's recovery file. Does nothing before start(), so the file stays until the user
    // answers whether to recover it.
    void songReplaced();
    // For a normal quit. Does nothing before start().
    void stopAndDiscard();

private:
    void timerCallback() override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void check();

    Document& document;
    const juce::File recoveryFile;
    std::function<bool()> isBusy;
    AutoSavePolicy policy;

    JUCE_DECLARE_NON_COPYABLE(AutoSaver)
};
