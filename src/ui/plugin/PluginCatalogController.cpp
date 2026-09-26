#include "ui/plugin/PluginCatalogController.h"
#include "AppProperties.h"
#include <juce_audio_utils/juce_audio_utils.h>

PluginCatalogController::PluginCatalogController(juce::AudioPluginFormatManager& formatManagerRef)
    : formatManager(formatManagerRef)
{
    if (auto xml = getAppProperties().getUserSettings()->getXmlValue("knownPluginList"))
        knownPluginList.recreateFromXml(*xml);
    knownPluginList.addChangeListener(this);
}

PluginCatalogController::~PluginCatalogController()
{
    knownPluginList.removeChangeListener(this);
}

void PluginCatalogController::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &knownPluginList)
    {
        if (auto xml = knownPluginList.createXml())
            getAppProperties().getUserSettings()->setValue("knownPluginList", xml.get());
    }
}

juce::Array<juce::PluginDescription> PluginCatalogController::getTypes() const
{
    return knownPluginList.getTypes();
}

void PluginCatalogController::showManageDialog()
{
    auto* listComp = new juce::PluginListComponent(formatManager, knownPluginList, juce::File{}, nullptr);
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
