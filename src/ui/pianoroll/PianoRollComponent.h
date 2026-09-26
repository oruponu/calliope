#pragma once

#include "model/MidiSequence.h"
#include "ui/pianoroll/EditClipboard.h"
#include "ui/pianoroll/TimelineGeometry.h"
#include "ui/pianoroll/strips/ChordStrip.h"
#include "ui/pianoroll/strips/KeySignatureStrip.h"
#include "ui/pianoroll/strips/LoopStrip.h"
#include "ui/pianoroll/strips/RulerStrip.h"
#include "ui/pianoroll/strips/TempoTrackStrip.h"
#include "ui/pianoroll/strips/TimeSignatureStrip.h"
#include <functional>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <set>
#include <variant>
#include <vector>

class PianoRollComponent : public juce::Component, public MidiSequence::Listener, private juce::Timer
{
public:
    explicit PianoRollComponent(juce::UndoManager& undoManagerRef);
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
    void deleteSelectedKeySignatures();
    void deleteSelectedChords();
    bool duplicateSelectedNotesWithPitchOffset(int deltaNote);
    void moveSelectionToAdjacentNote(int direction);
    void cutSelection();
    void copySelection();
    void paste(int atTick);
    bool hasSelection() const
    {
        return !selectedNotes.empty() || tempoStrip.hasSelection() || timeSigStrip.hasSelection() ||
               keyStrip.hasSelection() || chordStrip.hasSelection();
    }
    bool hasClipboardContent() const
    {
        return clipboard.hasNotes() || clipboard.hasTempoPoints() || clipboard.hasTimeSignatures() ||
               clipboard.hasKeySignatures() || clipboard.hasChords();
    }
    bool hasSelectedNotes() const { return !selectedNotes.empty(); }
    bool hasNotesInActiveTrack() const;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void modifierKeysChanged(const juce::ModifierKeys& modifiers) override;
    bool keyPressed(const juce::KeyPress& key) override;

    static constexpr int keyboardWidth = 72;
    static constexpr int defaultNoteHeight = 14;
    static constexpr int defaultBeatWidth = 80;
    static constexpr int totalNotes = 128;
    static constexpr int snapTicks = 480;
    static constexpr int loopStripHeight = LoopStrip::height;
    static constexpr int rulerHeight = RulerStrip::height;
    static constexpr int tempoTrackHeight = TempoTrackStrip::height;
    static constexpr int timeSignatureTrackHeight = TimeSignatureStrip::height;
    static constexpr int keySignatureTrackHeight = KeySignatureStrip::height;
    static constexpr int chordTrackHeight = ChordStrip::height;
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
    void timelineMetadataChanged() override;
    void updateStripPositions();
    void repaintStrips();

    void drawKeyboard(juce::Graphics& g);
    void drawGrid(juce::Graphics& g);
    void drawNotes(juce::Graphics& g);
    void drawMoveGhosts(juce::Graphics& g);
    void drawPlayhead(juce::Graphics& g);
    void drawLoopRegion(juce::Graphics& g);

    int tickToWidth(int durationTicks) const;
    int roundTickToGrid(int tick) const;
    int floorTickToGrid(int tick) const;

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
    juce::UndoManager& undoManager;
    double playheadTick = 0.0;
    std::set<int> selectedTrackIndices = {0};
    int activeTrackIndex = 0;

    bool isNoteSelected(const NoteRef& ref) const;
    void clearNoteSelection();
    void clearTempoSelection();
    void clearTimeSignatureSelection();
    void clearKeySignatureSelection();
    void clearChordSelection();
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

    struct ResizeTarget
    {
        NoteRef ref;
        int startTick = 0;
        int duration = 0;
    };

    struct MoveTarget
    {
        NoteRef ref;
        int startTick = 0;
        int noteNumber = 0;
    };

    struct Idle
    {
    };
    struct KeyboardPreviewing
    {
    };
    struct RubberBand
    {
        juce::Point<int> start;
        juce::Rectangle<int> rect;
    };
    struct Resizing
    {
        std::vector<ResizeTarget> targets;
        ResizeEdge edge = ResizeEdge::None;
        int anchorStartTick = 0;
        int anchorEndTick = 0;
        bool isCreatingNote = false;
    };
    struct Moving
    {
        std::vector<MoveTarget> targets;
        int anchorStartTick = 0;
        int dragStartTick = 0;
        int dragStartNote = 0;
        int deltaTick = 0;
        int deltaNote = 0;
    };

    std::variant<Idle, KeyboardPreviewing, RubberBand, Resizing, Moving> drag;
    void resetNoteDrag();
    void beginResize(const NoteRef& hit, ResizeEdge edge);
    void beginMove(const NoteRef& anchor, const juce::MouseEvent& e);
    int contentBeats = 0;

    EditClipboard clipboard;
    LoopStrip loopStrip{geometry};
    RulerStrip ruler{geometry};
    TempoTrackStrip tempoStrip{geometry, clipboard, undoManager};
    TimeSignatureStrip timeSigStrip{geometry, clipboard, undoManager};
    KeySignatureStrip keyStrip{geometry, clipboard, undoManager};
    ChordStrip chordStrip{geometry, clipboard, undoManager};

    MidiNote previewNote;
    bool isPreviewing = false;
    void startNotePreview(const MidiNote& note);
    void stopNotePreview();
    void timerCallback() override;
    static constexpr int previewHoldMs = 300;

    int quantizeDenominator = 4;

    bool loopEnabled = false;
    int loopStartTick = 0;
    int loopEndTick = 0;
};
