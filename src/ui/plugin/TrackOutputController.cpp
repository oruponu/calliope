#include "ui/plugin/TrackOutputController.h"
#include "document/Document.h"
#include "engine/PlaybackEngine.h"
#include "model/PluginAssignment.h"
#include "plugin/PluginAssignmentCodec.h"
#include "plugin/VstPluginHost.h"
#include "ui/plugin/PluginCatalogController.h"
#include <optional>
#include <utility>

TrackOutputController::TrackOutputController(VstPluginHost& pluginHostRef, Document& documentRef,
                                             PlaybackEngine& playbackEngineRef,
                                             PluginCatalogController& pluginCatalogRef,
                                             std::function<void()> stopPlaybackCallback)
    : pluginHost(pluginHostRef), document(documentRef), playbackEngine(playbackEngineRef),
      pluginCatalog(pluginCatalogRef), stopPlayback(std::move(stopPlaybackCallback))
{
}

void TrackOutputController::showOutputMenu(int trackIndex)
{
    auto& sequence = document.getSequence();
    if (trackIndex < 0 || trackIndex >= sequence.getNumTracks())
        return;

    const auto& track = sequence.getTrack(trackIndex);
    const TrackId trackId = track.getId();
    const auto currentDest = track.getOutputDestination();
    const TrackId currentTarget = sequence.resolveRouteTarget(trackIndex);
    auto types = pluginCatalog.getTypes();

    juce::PopupMenu menu;
    menu.addSectionHeader("Output");
    for (int i = 0; i < sequence.getNumTracks(); ++i)
    {
        const TrackId candidate = sequence.getTrack(i).getId();
        juce::String pluginName = pluginHost.getPluginName(candidate);
        if (pluginName.isEmpty())
            continue;
        const bool ticked = currentDest == MidiTrack::OutputDestination::Plugin && currentTarget == candidate;
        menu.addItem(pluginName, true, ticked, [this, trackId, candidate]() { routeToPlugin(trackId, candidate); });
    }

    menu.addItem("MIDI Device", true, currentDest == MidiTrack::OutputDestination::MidiDevice,
                 [this, trackId]() { setDestination(trackId, MidiTrack::OutputDestination::MidiDevice); });
    menu.addItem("None", true, currentDest == MidiTrack::OutputDestination::None,
                 [this, trackId]() { setDestination(trackId, MidiTrack::OutputDestination::None); });
    menu.addSeparator();

    menu.addItem("Load Plugin...", true, false, [this, trackId]()
                 { choosePluginFile([this, trackId](const juce::File& file) { attachPluginFile(trackId, file); }); });

    juce::PopupMenu chooseSubmenu;
    juce::KnownPluginList::addToMenu(chooseSubmenu, types, juce::KnownPluginList::sortByManufacturer);
    menu.addSubMenu("Choose Plugin", chooseSubmenu, !types.isEmpty());

    menu.addItem("Detach Plugin", track.getPluginAssignment() != nullptr, false,
                 [this, trackId]() { detachPlugin(trackId); });

    menu.showMenuAsync(juce::PopupMenu::Options{},
                       [this, trackId, types](int result)
                       {
                           int index = juce::KnownPluginList::getIndexChosenByMenu(types, result);
                           if (index < 0)
                               return;
                           attachPlugin(trackId, types.getReference(index));
                       });
}

void TrackOutputController::attachPluginToFirstTrack(const juce::PluginDescription& description)
{
    auto& sequence = document.getSequence();
    if (sequence.getNumTracks() == 0)
        return;
    attachPlugin(sequence.getTrack(0).getId(), description);
}

void TrackOutputController::attachPluginToFirstTrackViaFileChooser()
{
    choosePluginFile(
        [this](const juce::File& file)
        {
            auto& sequence = document.getSequence();
            if (sequence.getNumTracks() > 0)
                attachPluginFile(sequence.getTrack(0).getId(), file);
        });
}

void TrackOutputController::choosePluginFile(std::function<void(const juce::File&)> onChosen)
{
    fileChooser = std::make_unique<juce::FileChooser>("Load Plugin", juce::File{}, "*.vst3");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [onChosen = std::move(onChosen)](const juce::FileChooser& fc)
                             {
                                 auto file = fc.getResult();
                                 if (file != juce::File{})
                                     onChosen(file);
                             });
}

void TrackOutputController::attachPluginFile(TrackId trackId, const juce::File& file)
{
    stopPlaybackIfPlaying();
    if (auto description = pluginHost.describePluginFile(file))
        attachPlugin(trackId, *description);
}

void TrackOutputController::attachPlugin(TrackId trackId, const juce::PluginDescription& description)
{
    auto& sequence = document.getSequence();
    if (sequence.indexOf(trackId) < 0)
        return;

    stopPlaybackIfPlaying();
    if (!pluginHost.attachPlugin(trackId, description))
        return;

    auto& track = sequence.getTrack(sequence.indexOf(trackId));
    track.setPluginAssignment(
        std::make_shared<const PluginAssignment>(PluginAssignment{PluginAssignmentCodec::toXml(description), {}}));
    track.setRouteTarget(std::nullopt);
    track.setOutputDestination(MidiTrack::OutputDestination::Plugin);
    sequence.notifyTracksChanged();
    playbackEngine.rebuildSnapshot();
}

void TrackOutputController::detachPlugin(TrackId trackId)
{
    auto& sequence = document.getSequence();
    const int index = sequence.indexOf(trackId);
    if (index < 0)
        return;

    stopPlaybackIfPlaying();
    playbackEngine.releaseActiveNotesForTrack(trackId);
    pluginHost.detachPlugin(trackId);
    auto& track = sequence.getTrack(index);
    track.setPluginAssignment(nullptr);
    track.setOutputDestination(MidiTrack::OutputDestination::MidiDevice);
    sequence.notifyTracksChanged();
    playbackEngine.rebuildSnapshot();
}

void TrackOutputController::routeToPlugin(TrackId trackId, TrackId target)
{
    auto& sequence = document.getSequence();
    const int index = sequence.indexOf(trackId);
    if (index < 0)
        return;

    playbackEngine.releaseActiveNotesForTrack(trackId);
    auto& track = sequence.getTrack(index);
    track.setRouteTarget(target == trackId ? std::optional<TrackId>{} : target);
    track.setOutputDestination(MidiTrack::OutputDestination::Plugin);
    sequence.notifyTracksChanged();
    playbackEngine.rebuildSnapshot();
}

void TrackOutputController::setDestination(TrackId trackId, MidiTrack::OutputDestination destination)
{
    auto& sequence = document.getSequence();
    const int index = sequence.indexOf(trackId);
    if (index < 0)
        return;

    playbackEngine.releaseActiveNotesForTrack(trackId);
    sequence.getTrack(index).setOutputDestination(destination);
    sequence.notifyTracksChanged();
    playbackEngine.rebuildSnapshot();
}

void TrackOutputController::stopPlaybackIfPlaying()
{
    if (playbackEngine.isPlaying())
        stopPlayback();
}
