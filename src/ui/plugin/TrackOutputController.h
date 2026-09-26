#pragma once

#include "model/MidiTrack.h"
#include "model/TrackId.h"
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

class Document;
class PlaybackEngine;
class PluginCatalogController;
class VstPluginHost;

class TrackOutputController
{
public:
    TrackOutputController(VstPluginHost& pluginHost, Document& document, PlaybackEngine& playbackEngine,
                          PluginCatalogController& pluginCatalog, std::function<void()> stopPlaybackCallback);

    void showOutputMenu(int trackIndex);
    void attachPluginToFirstTrack(const juce::PluginDescription& description);
    void attachPluginToFirstTrackViaFileChooser();

private:
    void choosePluginFile(std::function<void(const juce::File&)> onChosen);
    void attachPluginFile(TrackId trackId, const juce::File& file);
    void attachPlugin(TrackId trackId, const juce::PluginDescription& description);
    void detachPlugin(TrackId trackId);
    void routeToPlugin(TrackId trackId, TrackId target);
    void setDestination(TrackId trackId, MidiTrack::OutputDestination destination);
    void stopPlaybackIfPlaying();

    VstPluginHost& pluginHost;
    Document& document;
    PlaybackEngine& playbackEngine;
    PluginCatalogController& pluginCatalog;
    std::function<void()> stopPlayback;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE(TrackOutputController)
};
