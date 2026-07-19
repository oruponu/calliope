#include "plugin/PluginManagementController.h"
#include "AppProperties.h"
#include "document/Document.h"
#include "engine/PlaybackEngine.h"
#include "model/MidiTrack.h"
#include "plugin/VstPluginHost.h"
#include <juce_audio_utils/juce_audio_utils.h>
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
    if (index < 0)
        return;

    stopPlaybackIfPlaying();
    if (pluginHost.loadPlugin(pluginMenuSnapshot.getReference(index)))
        applyPluginRoutingToTrack(0);
}

void PluginManagementController::loadPluginViaFileChooser()
{
    fileChooser = std::make_unique<juce::FileChooser>("Load Plugin", juce::File{}, "*.vst3");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [this](const juce::FileChooser& fc)
                             {
                                 auto file = fc.getResult();
                                 if (file == juce::File{})
                                     return;
                                 stopPlaybackIfPlaying();
                                 if (pluginHost.loadPlugin(file))
                                     applyPluginRoutingToTrack(0);
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

void PluginManagementController::attachPluginToTrackViaFileChooser(int trackIndex)
{
    fileChooser = std::make_unique<juce::FileChooser>("Load Plugin", juce::File{}, "*.vst3");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [this, trackIndex](const juce::FileChooser& fc)
                             {
                                 auto file = fc.getResult();
                                 if (file == juce::File{})
                                     return;
                                 stopPlaybackIfPlaying();
                                 if (pluginHost.attachPlugin(trackIndex, file))
                                     applyPluginRoutingToTrack(trackIndex);
                             });
}

void PluginManagementController::attachPluginToTrack(int trackIndex, const juce::PluginDescription& description)
{
    stopPlaybackIfPlaying();
    if (pluginHost.attachPlugin(trackIndex, description))
        applyPluginRoutingToTrack(trackIndex);
}

juce::Array<juce::PluginDescription> PluginManagementController::getPluginTypes() const
{
    return knownPluginList.getTypes();
}

void PluginManagementController::applyPluginRoutingToTrack(int trackIndex)
{
    auto& track = document.getSequence().getTrack(trackIndex);
    track.setRouteTargetTrackIndex(-1);
    track.setOutputDestination(MidiTrack::OutputDestination::Plugin);
    document.getSequence().notifyTracksChanged();
    playbackEngine.rebuildSnapshot();
}

void PluginManagementController::stopPlaybackIfPlaying()
{
    if (playbackEngine.isPlaying())
        stopPlayback();
}
