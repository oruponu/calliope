#pragma once

#include "audio/MidiDeviceOutput.h"
#include "audio/VstPluginHost.h"
#include "engine/PlaybackEngine.h"
#include "model/MidiSequence.h"
#include "ui/PianoRollComponent.h"
#include "ui/ControllerLaneComponent.h"
#include "ui/EventListComponent.h"
#include "ui/TrackListComponent.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

class MainComponent : public juce::Component,
                      public juce::MenuBarModel,
                      public juce::ApplicationCommandTarget,
                      public juce::FileDragAndDropTarget
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

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
    class TransportButton : public juce::Component
    {
    public:
        enum Type
        {
            ReturnToStart,
            Stop,
            Play,
            Loop
        };
        TransportButton(Type t) : type(t) { setRepaintsOnMouseActivity(true); }
        Type getType() const { return type; }
        void setActive(bool a)
        {
            active = a;
            repaint();
        }
        bool isActive() const { return active; }
        void paint(juce::Graphics& g) override;
        void mouseUp(const juce::MouseEvent& e) override;
        std::function<void()> onClick;

    private:
        Type type;
        bool active = false;
    };

    class ToolButton : public juce::Component
    {
    public:
        enum Type
        {
            EditTool,
            SelectTool
        };
        ToolButton(Type t) : type(t) { setRepaintsOnMouseActivity(true); }
        Type getType() const { return type; }
        void setActive(bool a)
        {
            active = a;
            repaint();
        }
        bool isActive() const { return active; }
        void paint(juce::Graphics& g) override;
        void mouseUp(const juce::MouseEvent& e) override;
        std::function<void()> onClick;

    private:
        Type type;
        bool active = false;
    };

    void setActiveTool(PianoRollComponent::EditMode mode);
    void onVBlank();
    void scrollToPlayhead(int tick);
    void updateTransportDisplay();
    void newFile();
    void saveFile();
    void loadFile();
    void stopPlayback();
    void onSequenceLoaded();
    void updateTitleBar();

    MidiSequence sequence;
    juce::UndoManager undoManager;
    PlaybackEngine playbackEngine;
    MidiDeviceOutput midiOutput;
    juce::AudioDeviceManager audioDeviceManager;
    juce::AudioProcessorGraph audioGraph;
    juce::AudioProcessorPlayer audioPlayer;
    VstPluginHost pluginHost;

    class PianoRollViewport : public juce::Viewport
    {
    public:
        std::function<void()> onReachedEnd;
        std::function<void()> onVisibleAreaChanged;
        std::function<void(const juce::MouseEvent&, const juce::MouseWheelDetails&)> onZoom;

        void visibleAreaChanged(const juce::Rectangle<int>&) override
        {
            if (onVisibleAreaChanged)
                onVisibleAreaChanged();
        }

        void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
        {
            if (e.mods.isCtrlDown() && onZoom)
            {
                onZoom(e, wheel);
                return;
            }
            if (auto* content = getViewedComponent())
            {
                int speed = 600;
                int newX = getViewPositionX() - juce::roundToInt(wheel.deltaX * speed);
                int newY = getViewPositionY() - juce::roundToInt(wheel.deltaY * speed);
                newX = juce::jlimit(0, juce::jmax(0, content->getWidth() - getViewWidth()), newX);
                newY = juce::jlimit(0, juce::jmax(0, content->getHeight() - getViewHeight()), newY);
                setViewPosition(newX, newY);
            }
            if (isAtRightEdge() && onReachedEnd)
                onReachedEnd();
        }

    private:
        bool isAtRightEdge() const
        {
            if (auto* content = getViewedComponent())
                return getViewPositionX() + getViewWidth() >= content->getWidth() - 1;
            return false;
        }
    };

    PianoRollComponent pianoRoll;
    PianoRollViewport viewport;
    TrackListComponent trackList;
    juce::Viewport trackListViewport;

    ControllerLaneComponent controllerLane;

    class ControllerLaneViewport : public juce::Viewport
    {
    public:
        std::function<void()> onVisibleAreaChanged;
        std::function<void()> onReachedEnd;

        ControllerLaneViewport() { getHorizontalScrollBar().addMouseListener(&scrollBarListener, false); }

        ~ControllerLaneViewport() override { getHorizontalScrollBar().removeMouseListener(&scrollBarListener); }

        void visibleAreaChanged(const juce::Rectangle<int>&) override
        {
            pendingExtend = isAtRightEdge();
            if (onVisibleAreaChanged)
                onVisibleAreaChanged();
        }

    private:
        bool isAtRightEdge() const
        {
            if (auto* content = getViewedComponent())
                return getViewPositionX() + getViewWidth() >= content->getWidth() - 1;
            return false;
        }

        struct ScrollBarListener : public juce::MouseListener
        {
            ControllerLaneViewport& owner;
            explicit ScrollBarListener(ControllerLaneViewport& o) : owner(o) {}
            void mouseUp(const juce::MouseEvent&) override
            {
                if (owner.pendingExtend && owner.onReachedEnd)
                {
                    owner.pendingExtend = false;
                    owner.onReachedEnd();
                }
            }
        };

        ScrollBarListener scrollBarListener{*this};
        bool pendingExtend = false;
    };

    ControllerLaneViewport controllerLaneViewport;

    class Divider : public juce::Component
    {
    public:
        Divider() { setMouseCursor(juce::MouseCursor::UpDownResizeCursor); }
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent&) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        std::function<void()> onDragStart;
        std::function<void(int deltaY)> onDrag;
    };

    class ZoomStrip : public juce::Component
    {
    public:
        enum Orientation
        {
            Horizontal,
            Vertical
        };

        explicit ZoomStrip(Orientation o) : orientation(o)
        {
            setRepaintsOnMouseActivity(true);
            addAndMakeVisible(slider);
            slider.setSliderStyle(o == Horizontal ? juce::Slider::LinearHorizontal : juce::Slider::LinearVertical);
            slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        }

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseUp(const juce::MouseEvent& e) override;

        juce::Slider slider;
        std::function<void()> onZoomIn;
        std::function<void()> onZoomOut;

    private:
        Orientation orientation;
        juce::Rectangle<int> minusBounds, plusBounds;
    };

    Divider controllerLaneDivider;
    ZoomStrip horizontalZoomStrip{ZoomStrip::Horizontal};
    ZoomStrip verticalZoomStrip{ZoomStrip::Vertical};
    int controllerLaneHeight = 120;
    int controllerLaneHeightOnDragStart = 120;
    bool syncingScroll = false;

    EventListComponent eventList;

    void setHorizontalZoom(int newBeatWidth, int anchorXInViewport);
    void setVerticalZoom(int newNoteHeight, int anchorYInViewport);
    void zoomHorizontal(float factor, int anchorXInViewport);
    void zoomVertical(float factor, int anchorYInViewport);
    void refreshAllViews();

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
        zoomInHorizontal,
        zoomOutHorizontal,
        zoomInVertical,
        zoomOutVertical,
        zoomReset,
        toggleLoop
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

    juce::Label positionHeaderLabel{"", "Bar"};
    juce::Label positionLabel;

    juce::Label timeSigHeaderLabel{"", "Beat"};
    juce::Label timeSigValueLabel;

    juce::Label keyHeaderLabel{"", "Key"};
    juce::Label keyValueLabel;

    juce::Label tempoHeaderLabel{"", "Tempo"};
    juce::Label tempoValueLabel;

    juce::File currentFile;
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::VBlankAttachment> vblankAttachment;
    bool fileDragOver = false;
    bool updatingFromEventList = false;

    static constexpr int menuBarHeight = 24;
    static constexpr int transportBarHeight = 64;
    static constexpr int toolBarHeight = 32;
    static constexpr int trackListWidth = 180;
    static constexpr int eventListWidth = 280;
    static constexpr int dividerHeight = 5;
    static constexpr int zoomStripLength = 100;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
