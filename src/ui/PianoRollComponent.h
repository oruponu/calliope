#pragma once

#include "../model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <set>

class PianoRollComponent : public juce::Component
{
public:
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
        bool operator<(const NoteRef& other) const
        {
            if (trackIndex != other.trackIndex)
                return trackIndex < other.trackIndex;
            return noteIndex < other.noteIndex;
        }
        bool operator==(const NoteRef& other) const
        {
            return trackIndex == other.trackIndex && noteIndex == other.noteIndex;
        }
    };

    void setSequence(MidiSequence* seq);
    void setUndoManager(juce::UndoManager* um);
    void setPlayheadTick(double tick);
    void setEditMode(EditMode mode);
    EditMode getEditMode() const;
    void setLoopRegion(bool enabled, int startTick, int endTick);

    std::function<void(int startTick, int endTick)> onLoopRegionChanged;
    void paint(juce::Graphics& g) override;

    std::function<void(int tick)> onPlayheadMoved;
    std::function<void()> onNotesChanged;
    std::function<void(const std::set<NoteRef>& selected)> onNoteSelectionChanged;
    std::function<void(const MidiNote&)> onNotePreview;
    std::function<void(const MidiNote&)> onNotePreviewEnd;
    std::function<void()> onZoomChanged;
    std::function<void(const juce::MouseEvent&, const juce::MouseWheelDetails&)> onRulerWheel;
    std::function<void(const juce::MouseEvent&, int deltaY)> onRulerDrag;

    void setSelectedTracks(int activeIndex, const std::set<int>& selectedIndices);
    void setSelectedNotes(const std::set<NoteRef>& notes);
    int getActiveTrackIndex() const;

    void copySelectedNotes();
    void cutSelectedNotes();
    void pasteNotes(int atTick);
    bool hasClipboardNotes() const { return !clipboard.empty(); }
    bool hasSelectedNotes() const { return !selectedNotes.empty(); }

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override;

    static constexpr int keyboardWidth = 72;
    static constexpr int defaultNoteHeight = 14;
    static constexpr int defaultBeatWidth = 80;
    static constexpr int totalNotes = 128;
    static constexpr int snapTicks = 480;
    static constexpr int loopBarHeight = 14;
    static constexpr int rulerHeight = 24;
    static constexpr int tempoTrackHeight = 48;
    static constexpr int timeSignatureTrackHeight = 24;
    static constexpr int keySignatureTrackHeight = 24;
    static constexpr int chordTrackHeight = 24;
    static constexpr int gridTopOffset = loopBarHeight + rulerHeight + tempoTrackHeight + timeSignatureTrackHeight +
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
    enum class DragMode
    {
        None,
        Moving,
        Resizing,
        RubberBand
    };

    void drawKeyboard(juce::Graphics& g);
    void drawLoopBar(juce::Graphics& g);
    void drawRuler(juce::Graphics& g);
    void drawTempoTrack(juce::Graphics& g);
    void drawTimeSignatureTrack(juce::Graphics& g);
    void drawKeySignatureTrack(juce::Graphics& g);
    void drawChordTrack(juce::Graphics& g);
    void drawGrid(juce::Graphics& g);
    void drawNotes(juce::Graphics& g);
    void drawPlayhead(juce::Graphics& g);
    void drawLoopRegion(juce::Graphics& g);
    void drawLoopOverlay(juce::Graphics& g, int top, int height, float fillAlpha);
    void drawTrackGridLines(juce::Graphics& g, int visibleLeft, int visibleRight, float top, float bottom);

    int tickToWidth(int durationTicks) const;
    int roundTickToGrid(int tick) const;
    int floorTickToGrid(int tick) const;

    NoteRef hitTestNote(int x, int y) const;
    int keyboardNoteAtPosition(int x, int y) const;
    bool isOnRightEdge(int x, const MidiNote& note) const;

    int getKeyboardLeft() const;
    int getRulerTop() const;

    static bool isBlackKey(int noteNumber);
    static juce::String getNoteName(int noteNumber);

    MidiSequence* sequence = nullptr;
    juce::UndoManager* undoManager = nullptr;
    double playheadTick = 0.0;
    std::set<int> selectedTrackIndices = {0};
    int activeTrackIndex = 0;

    bool isNoteSelected(const NoteRef& ref) const;
    void drawRubberBand(juce::Graphics& g);
    std::vector<NoteRef> findNotesInRect(const juce::Rectangle<int>& rect) const;

    EditMode editMode = EditMode::Select;
    NoteRef selectedNote;
    std::set<NoteRef> selectedNotes;
    DragMode dragMode = DragMode::None;
    int dragStartTick = 0;
    int dragStartNote = 0;
    int originalStartTick = 0;
    int originalNoteNumber = 0;
    int originalDuration = 0;
    int contentBeats = 0;
    juce::Point<int> rubberBandStart;
    juce::Rectangle<int> rubberBandRect;

    std::vector<MidiNote> clipboard;

    MidiNote previewNote;
    bool isPreviewing = false;
    void startNotePreview(const MidiNote& note);
    void stopNotePreview();

    bool isKeyboardDragging = false;

    bool isRulerDragging = false;
    int rulerDragStartY = 0;
    int lastRulerDragY = 0;

    int quantizeDenominator = 4;

    bool loopEnabled = false;
    int loopStartTick = 0;
    int loopEndTick = 0;
    bool isLoopDragging = false;
    int loopDragStartTick = 0;
};
