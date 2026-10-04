#pragma once

#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include <mutex>
#include <unordered_set>

// Reports edits made inside plugins on the message thread. juce::AudioProcessor does not hold its listener lock
// while calling listeners, so a plugin thread may still be inside a callback after removeListener() returns. The
// watched set closes that gap: once unwatch() returns, a callback from that processor can no longer queue a report.
// A single watcher serves every plugin, and it must outlive every plugin it has watched: the graph's render
// sequence keeps removed plugins alive until the graph itself is destroyed.
class PluginStateChangeWatcher : private juce::AudioProcessorListener, private juce::AsyncUpdater
{
public:
    void watch(juce::AudioProcessor& processor);
    void unwatch(juce::AudioProcessor& processor);

    // Delivers a pending report now, so that it lands before a save or a save prompt reads the changed flag.
    void flush();
    // Drops a pending report. Call after unwatching every plugin of the old document.
    void discardPending();

    std::function<void()> onChanged;

private:
    void audioProcessorParameterChanged(juce::AudioProcessor*, int, float) override {}
    void audioProcessorChanged(juce::AudioProcessor* processor, const ChangeDetails& details) override;
    void audioProcessorParameterChangeGestureEnd(juce::AudioProcessor* processor, int) override;
    void handleAsyncUpdate() override;
    void reportFrom(const juce::AudioProcessor* processor);

    std::mutex mutex;
    // Used only as keys; never dereferenced, so entries may outlive their processors.
    std::unordered_set<const juce::AudioProcessor*> watched;
};
