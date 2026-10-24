#pragma once

#include "edit/ControllerPointEditResult.h"
#include "edit/EditableNotes.h"
#include "edit/NoteRef.h"
#include "engine/RecordingLayer.h"
#include "engine/RecordingLayerEditor.h"
#include "model/ControllerKind.h"
#include "model/GridDivision.h"
#include "model/MidiSequence.h"
#include "model/TimelineGrid.h"
#include "ui/NoteDragPreview.h"
#include "ui/pianoroll/ControllerValueDraft.h"
#include "ui/pianoroll/ControllerValueEditor.h"
#include "ui/pianoroll/EditClipboard.h"
#include "ui/pianoroll/VelocityBarGrab.h"
#include "ui/pianoroll/strips/CalloutEditorSession.h"
#include "ui/pianoroll/strips/IndexSelection.h"
#include "ui/pianoroll/strips/RangeSelectGesture.h"
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
    ControllerLaneComponent(UndoHistory& undoHistoryRef, EditClipboard& clipboardRef)
        : undoHistory(undoHistoryRef), clipboard(clipboardRef)
    {
    }
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
    // Whether the piano roll's Edit tool is chosen; holding Ctrl (Cmd on macOS) inverts it, as in the piano roll.
    void setEditToolSelected(bool editTool);
    void setSnapEnabled(bool enabled);
    void clearPointSelection();
    // Whether points of the shown controller are selected on the active track.
    bool hasPointSelection() const;
    void copySelectedPoints();
    void cutSelectedPoints();
    // Whether the clipboard's points can go into the shown controller: same value range, so control change to control
    // change or pitch bend to pitch bend.
    bool canPastePoints() const;
    void pastePoints(int atTick);
    void recordingLayerChanged();
    void updateSize();

    DisplayMode getDisplayMode() const { return displayMode; }
    int getCCNumber() const { return ccNumber; }

    std::function<void(const juce::MouseEvent&, const juce::MouseWheelDetails&)> onMouseWheel;
    std::function<void(const NoteDragPreview&)> onNoteDragPreviewChanged;
    // Called when the user selects controller points, so other selections can be cleared.
    std::function<void()> onPointSelectionTaken;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;
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
    struct VelocityBars
    {
        std::vector<NoteRef> refs;
        std::vector<VelocityBar> bars;
    };
    VelocityBars activeVelocityBars() const;
    NoteRef velocityTopAt(int x, int y) const;
    NoteDragPreview dragPreview() const;
    // Only a changed preview is sent, so an idle lane never clears the piano roll's.
    void publishDragPreview();
    void drawControlChange(juce::Graphics& g);
    void drawPitchBend(juce::Graphics& g);
    void drawControllerPoints(juce::Graphics& g, const ControllerKind& kind, int trackIndex, juce::Colour colour,
                              float alpha);
    void drawProgramChange(juce::Graphics& g);
    void drawStepGraph(juce::Graphics& g, const std::vector<std::pair<int, int>>& events, juce::Colour colour,
                       float alpha, std::function<int(int)> toY);
    std::vector<std::pair<int, int>>
    recordedValues(int trackIndex, const std::function<std::optional<int>(const MidiEvent&)>& valueOf) const;
    void drawProgramBlocks(juce::Graphics& g, const std::vector<std::pair<int, int>>& events, juce::Colour colour,
                           float alpha);
    void drawPlayhead(juce::Graphics& g);
    void drawLoopRegion(juce::Graphics& g);
    void drawDragValueTip(juce::Graphics& g);

    int xToTick(int x) const;
    std::optional<ControllerKind> editedKind() const;
    const std::vector<MidiEvent>& displayedEvents(int trackIndex) const;
    std::set<int> displayedSelection() const;
    int hitTestPoint(int x, int y) const;
    int pointY(const ControllerKind& kind, int value) const;
    int valueAtY(const ControllerKind& kind, int y) const;
    TimelineGrid snapGrid(const juce::ModifierKeys& mods) const;
    bool hasActiveTrack() const;
    bool isOnLabelPanel(int x) const;

    void pointMouseDown(const juce::MouseEvent& e, const ControllerKind& kind);
    void applyPointEdit(const juce::String& transactionName, const std::vector<MidiEvent>& before,
                        const ControllerPointEditResult& result);
    void selectPoints(std::set<int> indices);
    void resetPointEditing();
    void openValueEditor(int eventIndex);
    void commitValueEdit(int displayedValue);
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
    EditClipboard& clipboard;
    struct Idle
    {
    };
    struct VelocityDragging
    {
        int trackIndex = -1;
        int lastDragX = -1;
        std::map<NoteRef, int> preview;
        // Set when the Select tool grabbed one bar's top; only that note changes then.
        std::optional<VelocityTopGrab> topGrab;
    };
    struct ValueTip
    {
        juce::Point<int> mouse;
        MidiEvent anchor;
    };
    struct PointDragging
    {
        int trackIndex = -1;
        std::vector<MidiEvent> before;
        std::set<int> moved;
        int anchor = -1;
        bool changed = false;
        ControllerPointEditResult preview;
        std::optional<ValueTip> tip;
    };
    struct PointRangeSelecting
    {
        RangeSelectGesture gesture;
    };
    std::variant<Idle, VelocityDragging, PointDragging, PointRangeSelecting> drag;
    IndexSelection pointSelection;
    // The point the first click of a double-click pressed, so a point that click added is not deleted.
    int doubleClickPoint = -1;
    CalloutEditorSession<ControllerValueEditor, ControllerValueDraft> valueEditSession;
    bool editToolSelected = false;
    bool snapEnabled = true;
    NoteDragPreview publishedPreview;

    static constexpr int topPadding = 6;
    static constexpr int bottomPadding = 6;
    static constexpr int velocityBarWidth = 7;
    static constexpr int velocityTopTolerance = 4;
    static constexpr float pointHitRadius = 6.0f;
    static constexpr int lineHitTolerance = 5;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControllerLaneComponent)
};
