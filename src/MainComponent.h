#pragma once

#include "audio/MidiDeviceOutput.h"
#include "audio/VstPluginHost.h"
#include "engine/PlaybackEngine.h"
#include "document/Document.h"
#include "model/MidiSequence.h"
#include "ui/menu/MainMenuModel.h"
#include "ui/PianoRollComponent.h"
#include "ui/ControllerLaneComponent.h"
#include "ui/EventListComponent.h"
#include "ui/TrackListComponent.h"
#include "ui/PianoRollViewport.h"
#include "ui/ControllerLaneViewport.h"
#include "ui/Divider.h"
#include "ui/ZoomStrip.h"
#include "ui/FocusBorder.h"
#include "ui/TransportBarComponent.h"
#include "ui/ToolButton.h"
#include "plugin/PluginManagementController.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

class MainComponent : public juce::Component,
                      public juce::ApplicationCommandTarget,
                      public juce::FileDragAndDropTarget,
                      public juce::ChangeListener,
                      public juce::FocusChangeListener,
                      public MidiSequence::Listener
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;
    void mouseDown(const juce::MouseEvent& e) override;

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

private:
    void tracksChanged() override;

    void setActiveTool(PianoRollComponent::EditMode mode);
    void onVBlank();
    void scrollToPlayhead(int tick);
    void scrollNoteIntoView(int startTick, int noteNumber);
    void scrollViewVertically(int deltaY);
    void scrollViewHorizontally(int deltaX);
    void newFile();
    void saveFile();
    void loadFile();
    void showAudioSettings();
    void stopPlayback();
    void onSequenceLoaded();
    void updateTitleBar();
    PlaybackTrackContext makeTrackContext(int trackIndex) const;

    Document document;
    PlaybackEngine playbackEngine;
    MidiDeviceOutput midiOutput;
    juce::AudioDeviceManager audioDeviceManager;
    juce::AudioProcessorGraph audioGraph;
    juce::AudioProcessorPlayer audioPlayer;
    VstPluginHost pluginHost;
    PluginManagementController pluginController{pluginHost, document, playbackEngine, [this] { stopPlayback(); }};

    TransportBarComponent transportBar{document, playbackEngine};

    PianoRollComponent pianoRoll;
    PianoRollViewport viewport;
    TrackListComponent trackList;
    juce::Viewport trackListViewport;
    juce::Rectangle<int> trackListHeaderBounds;

    ControllerLaneComponent controllerLane;

    ControllerLaneViewport controllerLaneViewport;

    Divider controllerLaneDivider{Divider::Horizontal};
    Divider trackListDivider{Divider::Vertical};
    Divider eventListDivider{Divider::Vertical};
    ZoomStrip horizontalZoomStrip{ZoomStrip::Horizontal};
    ZoomStrip verticalZoomStrip{ZoomStrip::Vertical};
    int controllerLaneHeight = 120;
    int controllerLaneHeightOnDragStart = 120;
    int trackListWidth = 248;
    int trackListWidthOnDragStart = 248;
    int eventListWidth = 280;
    int eventListWidthOnDragStart = 280;
    bool syncingScroll = false;

    EventListComponent eventList;

    enum class FocusPanel
    {
        TrackList,
        PianoRoll,
        EventList
    };
    FocusPanel focusedPanel = FocusPanel::PianoRoll;
    FocusBorder focusBorder;
    juce::Rectangle<int> trackListPanelBounds;
    juce::Rectangle<int> pianoRollPanelBounds;
    juce::Rectangle<int> eventListPanelBounds;
    void updateFocusBorder();

    void setHorizontalZoom(int newBeatWidth, int anchorXInViewport);
    void setVerticalZoom(int newNoteHeight, int anchorYInViewport);
    void zoomHorizontal(float factor, int anchorXInViewport);
    void zoomVertical(float factor, int anchorYInViewport);

    juce::ApplicationCommandManager commandManager;

    MainMenuModel mainMenuModel{commandManager, pluginController, midiOutput, [this] { showAudioSettings(); }};

    juce::MenuBarComponent menuBar;

    ToolButton editToolButton{ToolButton::EditTool};
    ToolButton selectToolButton{ToolButton::SelectTool};

    juce::ComboBox quantizeComboBox;

    juce::Rectangle<int> toolBarBounds;
    int toolBarSeparatorX = 0;

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::VBlankAttachment> vblankAttachment;
    bool fileDragOver = false;
    bool updatingFromEventList = false;

    static constexpr int menuBarHeight = 30;
    static constexpr int transportBarHeight = 64;
    static constexpr int toolBarHeight = 32;
    static constexpr int dividerThickness = 5;
    static constexpr int zoomStripLength = 100;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
