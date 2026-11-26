#pragma once

#include "app/AudioMidiHost.h"
#include "app/PlayheadLinks.h"
#include "app/SelectionLinks.h"
#include "document/Document.h"
#include "engine/PlaybackEngine.h"
#include "session/MidiThruSync.h"
#include "session/PlaybackSync.h"
#include "session/Recorder.h"
#include "ui/eventlist/EventListComponent.h"
#include "ui/file/FileOperations.h"
#include "ui/file/RecoveryController.h"
#include "ui/menu/MainMenuModel.h"
#include "ui/pianoroll/PianoRollPanel.h"
#include "ui/plugin/PluginCatalogController.h"
#include "ui/plugin/PluginEditorController.h"
#include "ui/plugin/TrackOutputController.h"
#include "ui/time/TimeEditLauncher.h"
#include "ui/tracklist/TrackListActions.h"
#include "ui/tracklist/TrackListPanel.h"
#include "ui/transport/MetronomeSettingsController.h"
#include "ui/transport/TransportBarComponent.h"
#include "ui/widgets/Divider.h"
#include "ui/widgets/PanelFrame.h"
#include "ui/window/AudioSettingsDialog.h"
#include <functional>
#include <juce_gui_extra/juce_gui_extra.h>
#include <optional>

class MainComponent : public juce::Component,
                      public juce::ApplicationCommandTarget,
                      public juce::FileDragAndDropTarget,
                      public juce::ChangeListener,
                      public juce::FocusChangeListener
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;
    std::unique_ptr<juce::ComponentTraverser> createKeyboardFocusTraverser() override;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void globalFocusChanged(juce::Component* focusedComponent) override;

    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    // next runs with the file operation still in progress and is meant to quit the app.
    void saveIfNeededThen(std::function<void()> next);
    void stopAutoSaveAndDiscardRecovery();

private:
    void toggleInputQuantize();
    void updateInputQuantize();
    void updateFocusBorder();
    void stopPlayback();
    void toggleRecording();
    void onSequenceLoaded();
    void updateTitleBar();
    PlaybackTrackContext makeTrackContext(int trackIndex) const;

    Document document;
    AudioMidiHost audio{[this](const juce::String& id) { recorder.releaseSource(id); }};
    PlaybackSync playbackSync{audio.getPlaybackEngine(), document.getSequence()};
    PluginCatalogController pluginCatalog{audio.getPluginHost().getFormatManager()};
    TrackOutputController trackOutput{
        audio.getPluginHost(),      document,
        audio.getPlaybackEngine(),  pluginCatalog,
        [this] { stopPlayback(); }, [this](TrackId trackId) { editorController.showEditor(trackId); }};
    PluginEditorController editorController{audio.getPluginHost(), *this, commandManager, *this};

    TransportBarComponent transportBar{document, audio.getPlaybackEngine()};

    PianoRollPanel pianoRollPanel{document.getHistory()};
    TrackListPanel trackListPanel;
    TrackListActions trackListActions{trackListPanel.getTrackList(), document, audio.getPlaybackEngine()};
    MidiThruSync midiThruSync{document.getSequence(), audio.getMidiThru(),
                              [this] { return trackListPanel.getTrackList().getActiveTrackIndex(); }};
    Recorder recorder{document.getSequence(), document.getHistory(), audio.getPlaybackEngine(),
                      audio.getRecordingInbox(),
                      [this] { return trackListPanel.getTrackList().getActiveTrackIndex(); }};

    TimeEditLauncher timeEdits{document, audio.getPlaybackEngine(), recorder, *this,
                               [this] { pianoRollPanel.clearSelections(); }};

    Divider trackListDivider{Divider::Vertical, Divider::Gutter};
    Divider eventListDivider{Divider::Vertical, Divider::Gutter};
    int trackListWidth = 248;
    int trackListWidthOnDragStart = 248;
    int eventListWidth = 280;
    int eventListWidthOnDragStart = 280;

    EventListComponent eventList{document.getHistory()};
    PlayheadLinks playheadLinks{pianoRollPanel, eventList, transportBar, audio.getPlaybackEngine(), *this};
    SelectionLinks selectionLinks{pianoRollPanel, eventList, trackListPanel.getTrackList(), [this]
                                  {
                                      midiThruSync.activeTrackChanged();
                                      recorder.activeTrackChanged();
                                  }};

    enum class FocusPanel
    {
        TrackList,
        PianoRoll,
        EventList
    };
    FocusPanel focusedPanel = FocusPanel::PianoRoll;
    PanelFrame trackListFrame;
    PanelFrame pianoRollFrame;
    PanelFrame eventListFrame;

    juce::ApplicationCommandManager commandManager;

    MainMenuModel mainMenuModel{commandManager,
                                pluginCatalog,
                                trackOutput,
                                audio.getMidiOutput(),
                                audio.getAudioDeviceManager(),
                                audio.getUserDisabledMidiInputs(),
                                [this]
                                {
                                    showAudioSettingsDialog(
                                        audio.getAudioDeviceManager(),
                                        getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
                                }};

    juce::MenuBarComponent menuBar;

    bool inputQuantizeEnabled = false;
    MetronomeSettingsController metronomeSettings{
        transportBar, audio.getPlaybackEngine(), audio.getMetronome(), commandManager, *this, *this};

    bool fileDragOver = false;
    FileOperations fileOperations{document, recorder, audio.getPluginHost(), [this]
                                  {
                                      onSequenceLoaded();
                                      updateTitleBar();
                                  }};
    RecoveryController recovery{document, audio.getPlaybackEngine(), recorder, fileOperations};

    static constexpr int menuBarHeight = 30;
    static constexpr int transportBarHeight = 64;
    static constexpr int panelGap = 6;
    static constexpr int panelPadding = 4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
