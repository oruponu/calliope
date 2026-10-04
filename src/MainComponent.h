#pragma once

#include "audio/MidiDeviceOutput.h"
#include "document/Document.h"
#include "engine/PlaybackEngine.h"
#include "engine/PlaybackSync.h"
#include "model/MidiSequence.h"
#include "plugin/PluginStateChangeWatcher.h"
#include "plugin/VstPluginHost.h"
#include "ui/EventListComponent.h"
#include "ui/TrackListComponent.h"
#include "ui/menu/MainMenuModel.h"
#include "ui/pianoroll/ControllerLaneComponent.h"
#include "ui/pianoroll/ControllerLaneViewport.h"
#include "ui/pianoroll/PianoRollComponent.h"
#include "ui/pianoroll/PianoRollViewport.h"
#include "ui/plugin/PluginCatalogController.h"
#include "ui/plugin/PluginEditorController.h"
#include "ui/plugin/TrackOutputController.h"
#include "ui/transport/TransportBarComponent.h"
#include "ui/widgets/Divider.h"
#include "ui/widgets/PanelFrame.h"
#include "ui/widgets/ToolButton.h"
#include "ui/widgets/ZoomStrip.h"
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
    void paintOverChildren(juce::Graphics&) override;
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

    // `next` runs with the file operation still in progress and must call finishFileOperation() when it is done,
    // unless it quits the app.
    void saveIfNeededThen(std::function<void()> next);

private:
    void tracksChanged() override;

    void setActiveTool(PianoRollComponent::EditMode mode);
    void updateFocusBorder();
    void onVBlank();
    void scrollToPlayhead(int tick);
    void scrollNoteIntoView(int startTick, int noteNumber);
    void scrollViewVertically(int deltaY);
    void scrollViewHorizontally(int deltaX);
    void setHorizontalZoom(int newBeatWidth, int anchorXInViewport);
    void setVerticalZoom(int newNoteHeight, int anchorYInViewport);
    void zoomHorizontal(float factor, int anchorXInViewport);
    void zoomVertical(float factor, int anchorYInViewport);
    void newFile();
    void saveFile();
    void saveFileAs();
    void loadFile();
    void importMidi();
    void importMidiFile(const juce::File& file);
    void exportMidi();
    void onProjectOpened();
    void showPluginLoadFailures();
    void showAudioSettings();
    void stopPlayback();
    void onSequenceLoaded();
    void updateTitleBar();
    void finishFileOperation();
    PlaybackTrackContext makeTrackContext(int trackIndex) const;

    Document document;
    PlaybackEngine playbackEngine;
    PlaybackSync playbackSync{playbackEngine, document.getSequence()};
    MidiDeviceOutput midiOutput;
    juce::AudioDeviceManager audioDeviceManager;
    // Declared before the graph so that it is destroyed after every plugin the graph owns.
    PluginStateChangeWatcher pluginStateWatcher;
    juce::AudioProcessorGraph audioGraph;
    juce::AudioProcessorPlayer audioPlayer;
    VstPluginHost pluginHost{pluginStateWatcher};
    PluginCatalogController pluginCatalog{pluginHost.getFormatManager()};
    TrackOutputController trackOutput{pluginHost, document, playbackEngine, pluginCatalog, [this] { stopPlayback(); }};
    PluginEditorController editorController{pluginHost};

    TransportBarComponent transportBar{document, playbackEngine};

    PianoRollComponent pianoRoll{document.getHistory()};
    PianoRollViewport viewport;
    TrackListComponent trackList;
    juce::Viewport trackListViewport;
    juce::Rectangle<int> trackListHeaderBounds;

    ControllerLaneComponent controllerLane{document.getHistory()};

    ControllerLaneViewport controllerLaneViewport;

    Divider controllerLaneDivider{Divider::Horizontal};
    Divider trackListDivider{Divider::Vertical, Divider::Gutter};
    Divider eventListDivider{Divider::Vertical, Divider::Gutter};
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
    PanelFrame trackListFrame;
    PanelFrame pianoRollFrame;
    PanelFrame eventListFrame;

    juce::ApplicationCommandManager commandManager;

    MainMenuModel mainMenuModel{commandManager, pluginCatalog, trackOutput, midiOutput,
                                [this] { showAudioSettings(); }};

    juce::MenuBarComponent menuBar;

    ToolButton editToolButton{ToolButton::EditTool};
    ToolButton selectToolButton{ToolButton::SelectTool};

    juce::ComboBox quantizeComboBox;

    juce::Rectangle<int> toolBarBounds;
    int toolBarSeparatorX = 0;

    std::unique_ptr<juce::VBlankAttachment> vblankAttachment;
    bool fileDragOver = false;
    bool updatingFromEventList = false;
    bool fileOperationInProgress = false;
    std::unique_ptr<juce::FileChooser> midiFileChooser;
    juce::ScopedMessageBox alertBox;

    static constexpr int menuBarHeight = 30;
    static constexpr int transportBarHeight = 64;
    static constexpr int toolBarHeight = 32;
    static constexpr int dividerThickness = 5;
    static constexpr int panelGap = 6;
    static constexpr int panelPadding = 4;
    static constexpr int zoomStripLength = 100;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
