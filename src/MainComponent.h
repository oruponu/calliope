#pragma once

#include "audio/MidiDeviceOutput.h"
#include "audio/VstPluginHost.h"
#include "engine/PlaybackEngine.h"
#include "document/Document.h"
#include "model/MidiSequence.h"
#include "ui/PianoRollComponent.h"
#include "ui/ControllerLaneComponent.h"
#include "ui/EventListComponent.h"
#include "ui/TrackListComponent.h"
#include "ui/WheelLabel.h"
#include "ui/PianoRollViewport.h"
#include "ui/ControllerLaneViewport.h"
#include "ui/Divider.h"
#include "ui/ZoomStrip.h"
#include "ui/FocusBorder.h"
#include "ui/TransportButton.h"
#include "ui/ToolButton.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

class MainComponent : public juce::Component,
                      public juce::MenuBarModel,
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

    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

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
    void tempoChanged() override;
    void timelineMetadataChanged() override;

    enum class PositionUnit
    {
        Bar,
        Beat,
        Tick
    };

    enum class TimeSigUnit
    {
        Numerator,
        Denominator
    };

    void setActiveTool(PianoRollComponent::EditMode mode);
    void onVBlank();
    void scrollToPlayhead(int tick);
    void jumpToTick(int tick);
    void commitPositionEdit();
    void nudgePosition(PositionUnit unit, int direction);
    void commitTempoEdit();
    void nudgeTempo(int direction);
    void setTempoAtPlayhead(double bpm);
    void commitTimeSignatureEdit();
    void nudgeTimeSignature(TimeSigUnit unit, int direction);
    void setTimeSignatureAtPlayhead(int numerator, int denominator);
    void commitKeySignatureEdit();
    void nudgeKeySignature(int direction);
    void setKeySignatureAtPlayhead(int sharpsOrFlats, bool isMinor);
    void scrollNoteIntoView(int startTick, int noteNumber);
    void scrollViewVertically(int deltaY);
    void scrollViewHorizontally(int deltaX);
    void updateTransportDisplay();
    void newFile();
    void saveFile();
    void loadFile();
    void loadPlugin();
    void managePlugins();
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
    juce::KnownPluginList knownPluginList;
    juce::Array<juce::PluginDescription> pluginMenuSnapshot;

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

    enum CommandID
    {
        newFile_ = 1,
        openFile,
        saveFile_,
        quitApp,
        togglePlay,
        returnToStart,
        prevBar,
        nextBar,
        switchToEditTool,
        switchToSelectTool,
        undoAction,
        redoAction,
        cutAction,
        copyAction,
        pasteAction,
        selectAllAction,
        moveNotesUp,
        moveNotesDown,
        moveSelectionPrev,
        moveSelectionNext,
        scrollViewUp,
        scrollViewDown,
        scrollViewLeft,
        scrollViewRight,
        zoomInHorizontal,
        zoomOutHorizontal,
        zoomInVertical,
        zoomOutVertical,
        zoomReset,
        toggleLoop,
        loadPlugin_,
        managePlugins_,
        audioSettings_
    };

    juce::ApplicationCommandManager commandManager;

    juce::MenuBarComponent menuBar;

    TransportButton returnToStartButton{TransportButton::ReturnToStart};
    TransportButton stopButton{TransportButton::Stop};
    TransportButton playButton{TransportButton::Play};
    TransportButton loopButton{TransportButton::Loop};

    ToolButton editToolButton{ToolButton::EditTool};
    ToolButton selectToolButton{ToolButton::SelectTool};

    juce::ComboBox quantizeComboBox;

    juce::Label positionHeaderLabel{"", "POSITION"};
    WheelLabel positionBarLabel;
    WheelLabel positionBeatLabel;
    WheelLabel positionTickLabel;
    juce::Label positionDot1{"", "."};
    juce::Label positionDot2{"", "."};

    juce::Label timeSigHeaderLabel{"", "TIME"};
    WheelLabel timeSigNumLabel;
    WheelLabel timeSigDenLabel;
    juce::Label timeSigSlashLabel{"", "/"};

    juce::Label keyHeaderLabel{"", "KEY"};
    WheelLabel keyValueLabel;

    juce::Label tempoHeaderLabel{"", "TEMPO"};
    WheelLabel tempoValueLabel;

    juce::Rectangle<int> positionBoxBounds;
    juce::Rectangle<int> infoBoxBounds;
    juce::Rectangle<int> toolBarBounds;
    int infoDividerX1 = 0;
    int infoDividerX2 = 0;
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
