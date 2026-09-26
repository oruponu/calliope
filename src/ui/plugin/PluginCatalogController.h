#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class PluginCatalogController : public juce::ChangeListener
{
public:
    explicit PluginCatalogController(juce::AudioPluginFormatManager& formatManager);
    ~PluginCatalogController() override;

    juce::Array<juce::PluginDescription> getTypes() const;
    void showManageDialog();

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    juce::AudioPluginFormatManager& formatManager;
    juce::KnownPluginList knownPluginList;

    JUCE_DECLARE_NON_COPYABLE(PluginCatalogController)
};
