#include "document/AutoSaver.h"
#include <utility>

namespace
{
constexpr int checkIntervalMs = 2000;
}

AutoSaver::AutoSaver(Document& documentRef, juce::File file, std::function<bool()> isBusySource)
    : document(documentRef), recoveryFile(std::move(file)), isBusy(std::move(isBusySource))
{
    document.addChangeListener(this);
}

AutoSaver::~AutoSaver()
{
    document.removeChangeListener(this);
}

void AutoSaver::start()
{
    policy.activate(document.getRevision(), recoveryFile.existsAsFile());
    startTimer(checkIntervalMs);
}

void AutoSaver::songReplaced()
{
    if (!policy.isActive())
        return;
    recoveryFile.deleteFile();
    policy.activate(document.getRevision(), recoveryFile.existsAsFile());
}

void AutoSaver::stopAndDiscard()
{
    if (!policy.isActive())
        return;
    stopTimer();
    recoveryFile.deleteFile();
}

void AutoSaver::timerCallback()
{
    check();
}

void AutoSaver::changeListenerCallback(juce::ChangeBroadcaster*)
{
    check();
}

void AutoSaver::check()
{
    const double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    switch (policy.update({now, document.getRevision(), document.hasChangedSinceSaved(), isBusy()}))
    {
    case AutoSaveAction::write:
        // Read the revision after writing, as flushing pending plugin changes while writing can raise it.
        if (document.writeRecoveryFile(recoveryFile))
            policy.wrote(document.getRevision());
        else
            policy.writeFailed(now);
        break;
    case AutoSaveAction::remove:
        recoveryFile.deleteFile();
        if (!recoveryFile.existsAsFile())
            policy.removed();
        break;
    case AutoSaveAction::none:
        break;
    }
}
