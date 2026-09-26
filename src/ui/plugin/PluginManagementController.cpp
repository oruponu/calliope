#include "ui/plugin/PluginManagementController.h"
#include "AppProperties.h"
#include "document/Document.h"
#include "engine/PlaybackEngine.h"
#include "model/MidiTrack.h"
#include "model/PluginAssignment.h"
#include "plugin/PluginAssignmentCodec.h"
#include "plugin/VstPluginHost.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>
#include <utility>

PluginManagementController::PluginManagementController(VstPluginHost& pluginHostRef, Document& documentRef,
                                                       PlaybackEngine& playbackEngineRef,
                                                       std::function<void()> stopPlaybackCallback)
    : pluginHost(pluginHostRef), document(documentRef), playbackEngine(playbackEngineRef),
      stopPlayback(std::move(stopPlaybackCallback))
{
    if (auto xml = getAppProperties().getUserSettings()->getXmlValue("knownPluginList"))
        knownPluginList.recreateFromXml(*xml);
    knownPluginList.addChangeListener(this);
}

PluginManagementController::~PluginManagementController()
{
    knownPluginList.removeChangeListener(this);
}

void PluginManagementController::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &knownPluginList)
    {
        if (auto xml = knownPluginList.createXml())
            getAppProperties().getUserSettings()->setValue("knownPluginList", xml.get());
    }
}

juce::PopupMenu PluginManagementController::buildPluginMenu()
{
    juce::PopupMenu menu;

    juce::PopupMenu::Item loadPluginItemEntry;
    loadPluginItemEntry.itemID = MenuItemID::loadPluginItem;
    loadPluginItemEntry.text = "Load Plugin...";
    loadPluginItemEntry.action = [this]() { loadPluginViaFileChooser(); };
    menu.addItem(loadPluginItemEntry);

    pluginMenuSnapshot = knownPluginList.getTypes();
    juce::PopupMenu scannedSubmenu;
    juce::KnownPluginList::addToMenu(scannedSubmenu, pluginMenuSnapshot, juce::KnownPluginList::sortByManufacturer);
    menu.addSubMenu("Load Scanned Plugin", scannedSubmenu, !pluginMenuSnapshot.isEmpty());

    menu.addSeparator();

    juce::PopupMenu::Item manageItem;
    manageItem.itemID = MenuItemID::managePluginsItem;
    manageItem.text = "Manage Plugins...";
    manageItem.action = [this]() { managePlugins(); };
    menu.addItem(manageItem);

    return menu;
}

void PluginManagementController::handleMenuSelection(int menuItemID)
{
    int index = juce::KnownPluginList::getIndexChosenByMenu(pluginMenuSnapshot, menuItemID);
    if (index < 0 || document.getSequence().getNumTracks() == 0)
        return;

    attachPluginToTrack(document.getSequence().getTrack(0).getId(), pluginMenuSnapshot.getReference(index));
}

void PluginManagementController::loadPluginViaFileChooser()
{
    fileChooser = std::make_unique<juce::FileChooser>("Load Plugin", juce::File{}, "*.vst3");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [this](const juce::FileChooser& fc)
                             {
                                 auto file = fc.getResult();
                                 if (file == juce::File{} || document.getSequence().getNumTracks() == 0)
                                     return;
                                 attachPluginFileToTrack(document.getSequence().getTrack(0).getId(), file);
                             });
}

void PluginManagementController::managePlugins()
{
    auto* listComp =
        new juce::PluginListComponent(pluginHost.getFormatManager(), knownPluginList, juce::File{}, nullptr);
    listComp->setSize(800, 600);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(listComp);
    options.dialogTitle = "Manage Plugins";
    options.dialogBackgroundColour =
        juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.launchAsync();
}

void PluginManagementController::attachPluginToTrackViaFileChooser(TrackId trackId)
{
    fileChooser = std::make_unique<juce::FileChooser>("Load Plugin", juce::File{}, "*.vst3");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [this, trackId](const juce::FileChooser& fc)
                             {
                                 auto file = fc.getResult();
                                 if (file == juce::File{})
                                     return;
                                 attachPluginFileToTrack(trackId, file);
                             });
}

void PluginManagementController::attachPluginFileToTrack(TrackId trackId, const juce::File& file)
{
    stopPlaybackIfPlaying();
    if (auto description = pluginHost.describePluginFile(file))
        attachPluginToTrack(trackId, *description);
}

void PluginManagementController::attachPluginToTrack(TrackId trackId, const juce::PluginDescription& description)
{
    if (document.getSequence().indexOf(trackId) < 0)
        return;

    stopPlaybackIfPlaying();
    if (pluginHost.attachPlugin(trackId, description))
        assignPluginToTrack(trackId, description);
}

juce::Array<juce::PluginDescription> PluginManagementController::getPluginTypes() const
{
    return knownPluginList.getTypes();
}

void PluginManagementController::assignPluginToTrack(TrackId trackId, const juce::PluginDescription& description)
{
    auto& sequence = document.getSequence();
    const int index = sequence.indexOf(trackId);
    if (index < 0)
        return;

    auto& track = sequence.getTrack(index);
    track.setPluginAssignment(
        std::make_shared<const PluginAssignment>(PluginAssignment{PluginAssignmentCodec::toXml(description), {}}));
    track.setRouteTarget(std::nullopt);
    track.setOutputDestination(MidiTrack::OutputDestination::Plugin);
    sequence.notifyTracksChanged();
    playbackEngine.rebuildSnapshot();
}

void PluginManagementController::stopPlaybackIfPlaying()
{
    if (playbackEngine.isPlaying())
        stopPlayback();
}
