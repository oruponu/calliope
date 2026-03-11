#pragma once

#include "../model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <set>
#include <vector>

class ControllerLaneComponent : public juce::Component
{
public:
    ControllerLaneComponent() = default;

    enum class DisplayMode
    {
        Velocity,
        ControlChange,
        PitchBend,
        ProgramChange
    };

    void setSequence(MidiSequence* seq);
    void setUndoManager(juce::UndoManager* um);
    void setSelectedTracks(int activeIndex, const std::set<int>& selectedIndices);
    void setDisplayMode(DisplayMode mode);
    void setCCNumber(int cc);
    void setPlayheadTick(double tick);
    void setContentBeats(int beats);
    void updateSize();

    DisplayMode getDisplayMode() const { return displayMode; }
    int getCCNumber() const { return ccNumber; }

    std::function<void()> onDataChanged;
    std::function<void(const juce::MouseEvent&, const juce::MouseWheelDetails&)> onMouseWheel;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override;

    int tickToX(int tick) const;

    static constexpr int leftPanelWidth = 72;
    static constexpr int beatWidth = 80;

private:
    void drawLeftPanel(juce::Graphics& g);
    void drawGrid(juce::Graphics& g);
    void drawVelocity(juce::Graphics& g);
    void drawControlChange(juce::Graphics& g);
    void drawPitchBend(juce::Graphics& g);
    void drawProgramChange(juce::Graphics& g);
    void drawStepGraph(juce::Graphics& g, const std::vector<std::pair<int, int>>& events, juce::Colour colour,
                       float alpha, std::function<int(int)> toY);
    void drawPlayhead(juce::Graphics& g);

    int xToTick(int x) const;
    int valueToY(int value) const;
    int yToValue(int y) const;

    int getDrawAreaTop() const;
    int getDrawAreaBottom() const;
    int getDrawAreaHeight() const;

    MidiSequence* sequence = nullptr;
    int activeTrackIndex = 0;
    std::set<int> selectedTrackIndices;
    DisplayMode displayMode = DisplayMode::Velocity;
    int ccNumber = 1;
    double playheadTick = 0.0;
    int contentBeats = 16;

    juce::UndoManager* undoManager = nullptr;
    bool isDragging = false;
    int lastDragX = -1;
    std::vector<int> velocitySnapshot;

    static constexpr int topPadding = 6;
    static constexpr int bottomPadding = 6;
    static constexpr int velocityBarWidth = 7;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControllerLaneComponent)
};
