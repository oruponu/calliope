#pragma once

#include "audio/MidiDeviceOutput.h"
#include "engine/PlaybackEngine.h"
#include "model/MidiSequence.h"
#include "ui/PianoRollComponent.h"
#include "ui/TrackListComponent.h"
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
            Play
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

    void onVBlank();
    void scrollToPlayhead(int tick);
    void updateTransportDisplay();
    void saveFile();
    void loadFile();
    void onSequenceLoaded();

    MidiSequence sequence;
    PlaybackEngine playbackEngine;
    MidiDeviceOutput midiOutput;

    class PianoRollViewport : public juce::Viewport
    {
    public:
        std::function<void()> onReachedEnd;
        std::function<void()> onVisibleAreaChanged;

        PianoRollViewport() { getHorizontalScrollBar().addMouseListener(&scrollBarListener, false); }

        ~PianoRollViewport() override { getHorizontalScrollBar().removeMouseListener(&scrollBarListener); }

        void visibleAreaChanged(const juce::Rectangle<int>&) override
        {
            pendingExtend = isAtRightEdge();
            if (onVisibleAreaChanged)
                onVisibleAreaChanged();
        }

        void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
        {
            juce::Viewport::mouseWheelMove(e, wheel);
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

        struct ScrollBarListener : public juce::MouseListener
        {
            PianoRollViewport& owner;
            explicit ScrollBarListener(PianoRollViewport& o) : owner(o) {}
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

    PianoRollComponent pianoRoll;
    PianoRollViewport viewport;
    TrackListComponent trackList;
    juce::Viewport trackListViewport;

    enum CommandID
    {
        openFile = 1,
        saveFile_,
        togglePlay,
        returnToStart,
        prevBar,
        nextBar
    };

    juce::ApplicationCommandManager commandManager;

    juce::MenuBarComponent menuBar;

    TransportButton returnToStartButton{TransportButton::ReturnToStart};
    TransportButton stopButton{TransportButton::Stop};
    TransportButton playButton{TransportButton::Play};

    juce::Label positionHeaderLabel{"", "Bar"};
    juce::Label positionLabel;

    juce::Label timeSigHeaderLabel{"", "Beat"};
    juce::Label timeSigValueLabel;

    juce::Label keyHeaderLabel{"", "Key"};
    juce::Label keyValueLabel;

    juce::Label tempoHeaderLabel{"", "Tempo"};
    juce::Label tempoValueLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::VBlankAttachment> vblankAttachment;
    bool fileDragOver = false;

    static constexpr int menuBarHeight = 24;
    static constexpr int transportBarHeight = 64;
    static constexpr int trackListWidth = 180;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
