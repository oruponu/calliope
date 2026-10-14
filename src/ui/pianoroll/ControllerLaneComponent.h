#pragma once

#include "edit/EditableNotes.h"
#include "edit/NoteRef.h"
#include "engine/RecordingLayer.h"
#include "engine/RecordingLayerEditor.h"
#include "model/GridDivision.h"
#include "model/MidiSequence.h"
#include "undo/UndoHistory.h"
#include <functional>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include <optional>
#include <set>
#include <variant>
#include <vector>

class ControllerLaneComponent : public juce::Component, public MidiSequence::Listener
{
public:
    explicit ControllerLaneComponent(UndoHistory& undoHistoryRef) : undoHistory(undoHistoryRef) {}
    ~ControllerLaneComponent() override;

    enum class DisplayMode
    {
        Velocity,
        ControlChange,
        PitchBend,
        ProgramChange
    };

    void setSequence(MidiSequence* seq);
    void setSelectedTracks(int activeIndex, const std::set<int>& selectedIndices);
    void setDisplayMode(DisplayMode mode);
    void setCCNumber(int cc);
    void setPlayheadTick(double tick);
    void setLoopRegion(bool enabled, int startTick, int endTick);
    void setContentBeats(int beats);
    void setBeatWidth(int w);
    void setRightPadding(int px);
    void setGridDivision(GridDivision division);
    void setRecordingLayer(const RecordingLayer* layer, RecordingLayerEditor* editor);
    void updateSize();

    DisplayMode getDisplayMode() const { return displayMode; }
    int getCCNumber() const { return ccNumber; }

    std::function<void(const juce::MouseEvent&, const juce::MouseWheelDetails&)> onMouseWheel;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override;

    int tickToX(int tick) const;

    static constexpr int leftPanelWidth = 72;
    static constexpr int defaultBeatWidth = 80;
    int beatWidth = defaultBeatWidth;

private:
    void notesChanged(int trackIndex) override;
    void tracksChanged() override;
    void tempoChanged() override;
    void timelineMetadataChanged() override;
    void sequenceReset() override;

    void drawLeftPanel(juce::Graphics& g);
    void drawGrid(juce::Graphics& g);
    void drawVelocity(juce::Graphics& g);
    EditableNotes editableNotes() const;
    int displayedVelocity(const NoteRef& ref, int velocity) const;
    void drawControlChange(juce::Graphics& g);
    void drawPitchBend(juce::Graphics& g);
    void drawProgramChange(juce::Graphics& g);
    void drawStepGraph(juce::Graphics& g, const std::vector<std::pair<int, int>>& events, juce::Colour colour,
                       float alpha, std::function<int(int)> toY);
    std::vector<std::pair<int, int>>
    recordedValues(int trackIndex, const std::function<std::optional<int>(const MidiEvent&)>& valueOf) const;
    void drawProgramBlocks(juce::Graphics& g, const std::vector<std::pair<int, int>>& events, juce::Colour colour,
                           float alpha);
    void drawPlayhead(juce::Graphics& g);
    void drawLoopRegion(juce::Graphics& g);

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
    bool loopEnabled = false;
    int loopStartTick = 0;
    int loopEndTick = 0;
    int contentBeats = 16;
    int rightPadding = 0;

    GridDivision gridDivision;
    const RecordingLayer* recordingLayer = nullptr;
    RecordingLayerEditor* recordingEditor = nullptr;

    UndoHistory& undoHistory;
    struct Idle
    {
    };
    struct VelocityDragging
    {
        int trackIndex = -1;
        int lastDragX = -1;
        std::map<NoteRef, int> preview;
    };
    std::variant<Idle, VelocityDragging> drag;

    static constexpr int topPadding = 6;
    static constexpr int bottomPadding = 6;
    static constexpr int velocityBarWidth = 7;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControllerLaneComponent)
};
