#include "plugin/PluginStateChangeWatcher.h"

void PluginStateChangeWatcher::watch(juce::AudioProcessor& processor)
{
    {
        const std::scoped_lock lock(mutex);
        watched.insert(&processor);
    }
    processor.addListener(this);
}

void PluginStateChangeWatcher::unwatch(juce::AudioProcessor& processor)
{
    processor.removeListener(this);
    const std::scoped_lock lock(mutex);
    watched.erase(&processor);
}

void PluginStateChangeWatcher::flush()
{
    handleUpdateNowIfNeeded();
}

void PluginStateChangeWatcher::discardPending()
{
    cancelPendingUpdate();
}

void PluginStateChangeWatcher::audioProcessorChanged(juce::AudioProcessor* processor, const ChangeDetails& details)
{
    if (details.programChanged || details.nonParameterStateChanged)
        reportFrom(processor);
}

void PluginStateChangeWatcher::audioProcessorParameterChangeGestureEnd(juce::AudioProcessor* processor, int)
{
    reportFrom(processor);
}

void PluginStateChangeWatcher::handleAsyncUpdate()
{
    if (onChanged)
        onChanged();
}

void PluginStateChangeWatcher::reportFrom(const juce::AudioProcessor* processor)
{
    // Trigger while holding the lock, so that unwatch() cannot slip between the check and the trigger.
    const std::scoped_lock lock(mutex);
    if (watched.contains(processor))
        triggerAsyncUpdate();
}
