#pragma once

#include "model/TrackId.h"
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

class Document;
class PlaybackEngine;
class VstPluginHost;

class PluginManagementController : public juce::ChangeListener
{
public:
    PluginManagementController(VstPluginHost& pluginHost, Document& document, PlaybackEngine& playbackEngine,
                               std::function<void()> stopPlaybackCallback);
    ~PluginManagementController() override;

    juce::PopupMenu buildPluginMenu();
    void handleMenuSelection(int menuItemID);
    void loadPluginViaFileChooser();
    void managePlugins();
    void attachPluginToTrackViaFileChooser(TrackId trackId);
    void attachPluginToTrack(TrackId trackId, const juce::PluginDescription& description);
    juce::Array<juce::PluginDescription> getPluginTypes() const;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    enum MenuItemID
    {
        loadPluginItem = 1,
        managePluginsItem
    };

    void attachPluginFileToTrack(TrackId trackId, const juce::File& file);
    void applyPluginRoutingToTrack(TrackId trackId);
    void stopPlaybackIfPlaying();

    VstPluginHost& pluginHost;
    Document& document;
    PlaybackEngine& playbackEngine;
    std::function<void()> stopPlayback;

    juce::KnownPluginList knownPluginList;
    juce::Array<juce::PluginDescription> pluginMenuSnapshot;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE(PluginManagementController)
};
