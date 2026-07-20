#pragma once

#include "model/MidiSequence.h"
#include "ui/pianoroll/EditClipboard.h"
#include "ui/pianoroll/TimeSignatureEditor.h"
#include "ui/pianoroll/TimelineGeometry.h"
#include "ui/pianoroll/strips/LoopStrip.h"
#include "ui/pianoroll/strips/RulerStrip.h"
#include <functional>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <set>
#include <vector>

class PianoRollComponent : public juce::Component, public MidiSequence::Listener, private juce::Timer
{
public:
    PianoRollComponent();
    ~PianoRollComponent() override;

    enum class EditMode
    {
        Edit,
        Select
    };

    struct NoteRef
    {
        int trackIndex = -1;
        int noteIndex = -1;
        bool isValid() const { return trackIndex >= 0 && noteIndex >= 0; }
        auto operator<=>(const NoteRef&) const = default;
    };

    void setSequence(MidiSequence* seq);
    void setUndoManager(juce::UndoManager* um);
    void setPlayheadTick(double tick);
    void setEditMode(EditMode mode);
    EditMode getEditMode() const;
    void setLoopRegion(bool enabled, int startTick, int endTick);

    std::function<void(int startTick, int endTick)> onLoopRegionChanged;
    std::function<void()> onTempoChanged;
    void paint(juce::Graphics& g) override;
    void resized() override;
    void moved() override;

    std::function<void(int tick)> onPlayheadMoved;
    std::function<void()> onNotesChanged;
    std::function<void(const std::set<NoteRef>& selected)> onNoteSelectionChanged;
    std::function<void(const MidiNote&)> onNotePreview;
    std::function<void(const MidiNote&)> onNotePreviewEnd;
    std::function<void(int startTick, int noteNumber)> onScrollToNote;
    std::function<void(int deltaY)> onScrollVertical;
    std::function<void(int deltaX)> onScrollHorizontal;
    std::function<void()> onZoomChanged;
    std::function<void(const juce::MouseEvent&, const juce::MouseWheelDetails&)> onRulerWheel;
    std::function<void(const juce::MouseEvent&, int deltaY)> onRulerDrag;

    void setSelectedTracks(int activeIndex, const std::set<int>& selectedIndices);
    void setSelectedNotes(const std::set<NoteRef>& notes);
    int getActiveTrackIndex() const;

    void copySelectedNotes();
    void cutSelectedNotes();
    void deleteSelectedNotes();
    void pasteNotes(int atTick);
    void selectAllNotes();
    void nudgeSelectedNotesPitch(int deltaNote);
    void nudgeSelectedNotesTime(int deltaTick);
    void deleteSelectedTempoPoints();
    void deleteSelectedTimeSignatures();
    bool duplicateSelectedNotesWithPitchOffset(int deltaNote);
    void moveSelectionToAdjacentNote(int direction);
    void cutSelection();
    void copySelection();
    void paste(int atTick);
    bool hasSelection() const
    {
        return !selectedNotes.empty() || !selectedTempoIndices.empty() || !selectedTimeSigIndices.empty();
    }
    bool hasClipboardContent() const
    {
        return clipboard.hasNotes() || clipboard.hasTempoPoints() || clipboard.hasTimeSignatures();
    }
    bool hasSelectedNotes() const { return !selectedNotes.empty(); }
    bool hasNotesInActiveTrack() const;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void modifierKeysChanged(const juce::ModifierKeys& modifiers) override;
    bool keyPressed(const juce::KeyPress& key) override;

    static constexpr int keyboardWidth = 72;
    static constexpr int defaultNoteHeight = 14;
    static constexpr int defaultBeatWidth = 80;
    static constexpr int totalNotes = 128;
    static constexpr int snapTicks = 480;
    static constexpr int loopStripHeight = LoopStrip::height;
    static constexpr int rulerHeight = RulerStrip::height;
    static constexpr int tempoTrackHeight = 48;
    static constexpr int timeSignatureTrackHeight = 24;
    static constexpr int keySignatureTrackHeight = 24;
    static constexpr int chordTrackHeight = 24;
    static constexpr int gridTopOffset = loopStripHeight + rulerHeight + tempoTrackHeight + timeSignatureTrackHeight +
                                         keySignatureTrackHeight + chordTrackHeight;
    static constexpr int resizeEdgeWidth = 6;

    static constexpr int minBeatWidth = 4;
    static constexpr int maxBeatWidth = 400;
    static constexpr int minNoteHeight = 4;
    static constexpr int maxNoteHeight = 40;

    int noteHeight = defaultNoteHeight;
    int beatWidth = defaultBeatWidth;

    void setBeatWidth(int w);
    void setNoteHeight(int h);
    int getBeatWidth() const { return beatWidth; }
    int getNoteHeight() const { return noteHeight; }

    void setQuantizeDenominator(int denom);
    int getQuantizeDenominator() const { return quantizeDenominator; }

    int tickToX(int tick) const;
    int noteToY(int noteNumber) const;
    int xToTick(int x) const;
    int yToNote(int y) const;
    int getContentBeats() const { return contentBeats; }
    void updateSize();
    void extendContent();

private:
    void notesChanged(int trackIndex) override;
    void tracksChanged() override;
    void tempoChanged() override;
    void timelineMetadataChanged() override;
    void updateStripPositions();
    void repaintStrips();

    enum class DragMode
    {
        None,
        Moving,
        Resizing,
        RubberBand
    };

    void drawKeyboard(juce::Graphics& g);
    void drawTempoTrack(juce::Graphics& g);
    void drawTempoRangeSelection(juce::Graphics& g);
    void drawTimeSignatureTrack(juce::Graphics& g);
    void drawTimeSignatureRangeSelection(juce::Graphics& g);
    void drawKeySignatureTrack(juce::Graphics& g);
    void drawChordTrack(juce::Graphics& g);
    void drawGrid(juce::Graphics& g);
    void drawNotes(juce::Graphics& g);
    void drawMoveGhosts(juce::Graphics& g);
    void drawPlayhead(juce::Graphics& g);
    void drawLoopRegion(juce::Graphics& g);
    void drawLoopOverlay(juce::Graphics& g, int top, int height, float fillAlpha);
    void drawTrackGridLines(juce::Graphics& g, int visibleLeft, int visibleRight, float top, float bottom);

    int tickToWidth(int durationTicks) const;
    int roundTickToGrid(int tick) const;
    int floorTickToGrid(int tick) const;

    float tempoBpmToY(double bpm) const;
    double tempoYToBpm(int y) const;
    bool hitTestTempoLine(int x, int y, int& outTick, double& outBpm) const;
    int hitTestTempoPoint(int x, int y) const;

    enum class ResizeEdge
    {
        None,
        Left,
        Right
    };

    NoteRef hitTestNote(int x, int y) const;
    int keyboardNoteAtPosition(int x, int y) const;
    ResizeEdge edgeAt(int x, const MidiNote& note) const;

    int getKeyboardLeft() const;
    int getRulerTop() const;

    static bool isBlackKey(int noteNumber);
    static juce::String getNoteName(int noteNumber);

    TimelineGeometry geometry{keyboardWidth};
    MidiSequence* sequence = nullptr;
    juce::UndoManager* undoManager = nullptr;
    double playheadTick = 0.0;
    std::set<int> selectedTrackIndices = {0};
    int activeTrackIndex = 0;

    bool isNoteSelected(const NoteRef& ref) const;
    void clearNoteSelection();
    void clearTempoSelection();
    void copySelectedTempoPoints();
    void cutSelectedTempoPoints();
    void pasteTempoPoints(int atTick);
    void deleteSelectedTempoPointsImpl(const juce::String& transactionName);
    int hitTestTimeSignaturePoint(int x, int y) const;
    juce::Rectangle<int> timeSignatureLabelRect(int index) const;
    void clearTimeSignatureSelection();
    void copySelectedTimeSignatures();
    void cutSelectedTimeSignatures();
    void pasteTimeSignatures(int atTick);
    void deleteSelectedTimeSignaturesImpl(const juce::String& transactionName);
    void openTimeSignatureEditor(int tick, int num, int den, bool isNew, juce::Rectangle<int> anchorInLocal);
    void commitTimeSignatureEdit(int num, int den);
    void cancelTimeSignatureEdit();
    void closeTimeSignatureEditor();
    void drawRubberBand(juce::Graphics& g);
    std::vector<NoteRef> findNotesInRect(const juce::Rectangle<int>& rect) const;

    static EditMode swapTool(EditMode mode);
    void updateEffectiveEditMode();

    EditMode editMode = EditMode::Select;
    EditMode baseEditMode = EditMode::Select;
    bool toolSwapActive = false;
    bool altKeyDown = false;
    bool altDuplicateDone = false;
    NoteRef selectedNote;
    std::set<NoteRef> selectedNotes;
    DragMode dragMode = DragMode::None;
    int dragStartTick = 0;
    int dragStartNote = 0;

    struct ResizeTarget
    {
        NoteRef ref;
        int startTick = 0;
        int duration = 0;
    };
    std::vector<ResizeTarget> resizeTargets;

    struct MoveTarget
    {
        NoteRef ref;
        int startTick = 0;
        int noteNumber = 0;
    };
    std::vector<MoveTarget> moveTargets;
    int moveAnchorStartTick = 0;
    int moveDeltaTick = 0;
    int moveDeltaNote = 0;

    ResizeEdge resizeEdge = ResizeEdge::None;
    int resizeAnchorStartTick = 0;
    int resizeAnchorEndTick = 0;
    bool isCreatingNote = false;
    void beginResize(const NoteRef& hit, ResizeEdge edge);
    void beginMove(const NoteRef& anchor, const juce::MouseEvent& e);
    int contentBeats = 0;
    juce::Point<int> rubberBandStart;
    juce::Rectangle<int> rubberBandRect;

    EditClipboard clipboard;
    LoopStrip loopStrip{geometry};
    RulerStrip ruler{geometry};

    MidiNote previewNote;
    bool isPreviewing = false;
    void startNotePreview(const MidiNote& note);
    void stopNotePreview();
    void timerCallback() override;
    static constexpr int previewHoldMs = 300;

    bool isKeyboardDragging = false;

    int quantizeDenominator = 4;

    bool loopEnabled = false;
    int loopStartTick = 0;
    int loopEndTick = 0;

    bool isTempoPointDragging = false;
    int tempoDragIndex = -1;
    bool tempoDragMoved = false;
    std::vector<TempoChange> tempoDragBefore;
    std::vector<int> tempoDragGroup;

    std::set<int> selectedTimeSigIndices;
    bool isTimeSigEditing = false;
    int timeSigEditTick = 0;
    int timeSigDraftNum = 4;
    int timeSigDraftDen = 4;
    bool timeSigEditIsNew = false;
    juce::Component::SafePointer<juce::CallOutBox> timeSigCallout;
    juce::Component::SafePointer<TimeSignatureEditor> timeSigEditor;
    bool isTimeSigPointDragging = false;
    int timeSigDragIndex = -1;
    bool timeSigDragMoved = false;
    std::vector<TimeSignatureChange> timeSigDragBefore;
    int timeSigDragGrabOffset = 0;
    std::vector<int> timeSigDragGroup;
    bool isTimeSigRangeSelecting = false;
    int timeSigSelectStartX = 0;
    int timeSigSelectCurrentX = 0;
    std::set<int> timeSigSelectBase;
    std::set<int> selectedTempoIndices;
    bool isTempoRangeSelecting = false;
    int tempoSelectStartX = 0;
    int tempoSelectCurrentX = 0;
    std::set<int> tempoSelectBase;
};
