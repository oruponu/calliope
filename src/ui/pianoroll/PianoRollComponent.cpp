#include "ui/pianoroll/PianoRollComponent.h"
#include "edit/NoteEdit.h"
#include "edit/NoteEdits.h"
#include "edit/NoteQuantize.h"
#include "ui/pianoroll/GridLineColours.h"
#include "ui/theme/TrackColours.h"
#include "undo/NoteActions.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <ranges>
#include <utility>
#include <variant>
#include <vector>

PianoRollComponent::PianoRollComponent(UndoHistory& undoHistoryRef) : undoHistory(undoHistoryRef)
{
    addAndMakeVisible(loopStrip);
    loopStrip.onLoopRegionChanged = [this](int startTick, int endTick)
    {
        setLoopRegion(loopEnabled, startTick, endTick);
        if (onLoopRegionChanged)
            onLoopRegionChanged(startTick, endTick);
    };
    addAndMakeVisible(ruler);
    ruler.onSeek = [this](int tick)
    {
        setPlayheadTick(tick);
        if (onPlayheadMoved)
            onPlayheadMoved(tick);
    };
    ruler.onWheelZoom = [this](const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
    {
        if (onRulerWheel)
            onRulerWheel(e, w);
    };
    ruler.onDragZoom = [this](const juce::MouseEvent& e, int deltaY)
    {
        if (onRulerDrag)
            onRulerDrag(e, deltaY);
    };
    addAndMakeVisible(tempoStrip);
    tempoStrip.onSelectionTaken = [this]
    {
        clearNoteSelection();
        timeSigStrip.clearTimeSignatureSelection();
        keyStrip.clearKeySignatureSelection();
        chordStrip.clearChordSelection();
        repaint();
    };
    addAndMakeVisible(timeSigStrip);
    timeSigStrip.onSelectionTaken = [this]
    {
        clearNoteSelection();
        tempoStrip.clearTempoSelection();
        keyStrip.clearKeySignatureSelection();
        chordStrip.clearChordSelection();
        repaint();
    };
    timeSigStrip.onTimeSignaturePreview = [this](const std::vector<TimeSignatureChange>* preview)
    {
        displayedTimeline.setTimeSignaturePreview(preview);
        repaint();
    };
    addAndMakeVisible(keyStrip);
    keyStrip.onSelectionTaken = [this]
    {
        clearNoteSelection();
        tempoStrip.clearTempoSelection();
        timeSigStrip.clearTimeSignatureSelection();
        chordStrip.clearChordSelection();
        repaint();
    };
    addAndMakeVisible(chordStrip);
    chordStrip.onSelectionTaken = [this]
    {
        clearNoteSelection();
        tempoStrip.clearTempoSelection();
        timeSigStrip.clearTimeSignatureSelection();
        keyStrip.clearKeySignatureSelection();
        repaint();
    };
}

PianoRollComponent::~PianoRollComponent()
{
    if (sequence != nullptr)
        sequence->removeListener(this);
}

void PianoRollComponent::setSequence(MidiSequence* seq)
{
    if (sequence != nullptr)
        sequence->removeListener(this);
    sequence = seq;
    displayedTimeline.setSequence(seq);
    geometry.setTicksPerQuarterNote(sequence != nullptr ? sequence->getTimeline().getTicksPerQuarterNote() : 0);
    loopStrip.setSequence(seq);
    ruler.setSequence(seq);
    tempoStrip.setSequence(seq);
    timeSigStrip.setSequence(seq);
    keyStrip.setSequence(seq);
    chordStrip.setSequence(seq);
    if (sequence != nullptr)
        sequence->addListener(this);

    contentBeats = 16;
    if (sequence && sequence->getNumTracks() > 0)
    {
        int lastTick = 0;
        for (int t = 0; t < sequence->getNumTracks(); ++t)
        {
            const auto& track = sequence->getTrack(t);
            for (int i = 0; i < track.getNumNotes(); ++i)
            {
                int end = track.getNote(i).endTick();
                if (end > lastTick)
                    lastTick = end;
            }
        }
        contentBeats = std::max(contentBeats, lastTick / sequence->getTimeline().getTicksPerQuarterNote() + 4);
    }

    updateSize();
    repaint();
}

void PianoRollComponent::setPlayheadTick(double tick)
{
    auto toX = [this](double t) -> int
    {
        if (!sequence)
            return keyboardWidth;
        return keyboardWidth + static_cast<int>(t / sequence->getTimeline().getTicksPerQuarterNote() * beatWidth);
    };

    int oldX = toX(playheadTick);
    playheadTick = tick;
    int newX = toX(playheadTick);

    int margin = 2;
    int h = getHeight();
    repaint(oldX - margin, 0, margin * 2 + 2, h);
    repaint(newX - margin, 0, margin * 2 + 2, h);
    if (recordingLayer != nullptr && newX != oldX)
    {
        const int left = std::min(oldX, newX) - margin;
        const int right = std::max(oldX, newX) + margin;
        for (const auto& held : recordingLayer->getHeldNotes())
            repaint(left, noteToY(held.noteNumber), right - left + 2, noteHeight);
        // The ghost of a dragged held note ends at the playhead on its own row.
        if (const auto* moving = std::get_if<Moving>(&drag))
            for (const auto& t : moving->targets)
                if (t.ref.isRecorded())
                    repaint(left, noteToY(t.noteNumber + moving->deltaNote), right - left + 2, noteHeight);
    }
    loopStrip.setPlayheadTick(tick);
    ruler.setPlayheadTick(tick);
    tempoStrip.setPlayheadTick(tick);
    timeSigStrip.setPlayheadTick(tick);
    keyStrip.setPlayheadTick(tick);
    chordStrip.setPlayheadTick(tick);
}

void PianoRollComponent::setRecordingLayer(const RecordingLayer* layer, RecordingLayerEditor* editor)
{
    recordingLayer = layer;
    recordingEditor = editor;
    repaint();
}

void PianoRollComponent::recordingLayerChanged()
{
    pruneRecordedRefs();
    repaint();
}

void PianoRollComponent::recordedNotesCommitted(const std::map<int, NoteRef>& committed)
{
    const auto toSequence = [&committed](const NoteRef& ref)
    {
        if (!ref.isRecorded())
            return ref;
        const auto it = committed.find(ref.noteIndex);
        return it == committed.end() ? NoteRef{} : it->second;
    };

    selectedNote = toSequence(selectedNote);
    std::set<NoteRef> converted;
    for (const auto& ref : selectedNotes)
        if (const auto sequenceRef = toSequence(ref); sequenceRef.isValid())
            converted.insert(sequenceRef);
    if (converted == selectedNotes)
        return;

    selectedNotes = std::move(converted);
    repaint();
    if (onNoteSelectionChanged)
        onNoteSelectionChanged(selectedNotes);
}

void PianoRollComponent::drawRecordingLayer(juce::Graphics& g)
{
    if (!sequence || recordingLayer == nullptr)
        return;

    const auto clip = g.getClipBounds();
    const auto notes = editableNotes();
    for (const int trackIdx : selectedTrackIndices)
    {
        if (trackIdx < 0 || trackIdx >= sequence->getNumTracks())
            continue;
        for (const auto& [ref, note] : notes.onTrack(trackIdx))
            if (ref.isRecorded())
                drawRecordedNote(g, clip, displayedNote(ref, note), TrackColours::getColour(trackIdx),
                                 isNoteSelected(ref));
    }
}

void PianoRollComponent::drawRecordedNote(juce::Graphics& g, const juce::Rectangle<int>& clip, const MidiNote& note,
                                          juce::Colour trackColour, bool isSelected)
{
    using namespace calliope::theme;
    const int x = tickToX(note.startTick);
    const int y = noteToY(note.noteNumber);
    const int w = recordedNoteWidth(note.duration);
    if (y + noteHeight < clip.getY() || y > clip.getBottom() || x + w < clip.getX() || x > clip.getRight())
        return;

    const juce::Rectangle<float> rect(static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(w),
                                      static_cast<float>(noteHeight - 2));
    g.setColour((isSelected ? trackColour.brighter(0.4f) : trackColour).withAlpha(0.55f));
    g.fillRoundedRectangle(rect, 2.0f);
    g.setColour(status::danger);
    g.drawRoundedRectangle(rect, 2.0f, 1.5f);
}

void PianoRollComponent::setEditMode(EditMode mode)
{
    baseEditMode = mode;
    editMode = toolSwapActive ? swapTool(mode) : mode;
    resetNoteDrag();
    repaint();
}

PianoRollComponent::EditMode PianoRollComponent::getEditMode() const
{
    return editMode;
}

void PianoRollComponent::setLoopRegion(bool enabled, int startTick, int endTick)
{
    loopEnabled = enabled;
    loopStartTick = startTick;
    loopEndTick = endTick;
    repaint();
    loopStrip.setLoopRegion(enabled, startTick, endTick);
    ruler.setLoopRegion(enabled, startTick, endTick);
    tempoStrip.setLoopRegion(enabled, startTick, endTick);
    timeSigStrip.setLoopRegion(enabled, startTick, endTick);
    keyStrip.setLoopRegion(enabled, startTick, endTick);
    chordStrip.setLoopRegion(enabled, startTick, endTick);
}

void PianoRollComponent::setSelectedTracks(int activeIndex, const std::set<int>& selectedIndices)
{
    activeTrackIndex = activeIndex;
    selectedTrackIndices = selectedIndices;
    selectedNote = {};
    selectedNotes.clear();
    resetNoteDrag();
    repaint();
}

void PianoRollComponent::setSelectedNotes(const std::set<NoteRef>& notes)
{
    if (!notes.empty())
    {
        clearTempoSelection();
        clearTimeSignatureSelection();
        clearKeySignatureSelection();
        clearChordSelection();
    }
    selectedNotes = notes;
    std::erase_if(selectedNotes,
                  [this](const NoteRef& ref) { return ref.isRecorded() && ref.trackIndex != activeTrackIndex; });
    selectedNote = {};
    repaint();
}

int PianoRollComponent::getActiveTrackIndex() const
{
    return activeTrackIndex;
}

void PianoRollComponent::deleteSelectedNotes()
{
    if (!sequence || selectedNotes.empty())
        return;

    NoteRef survivor;
    if (activeTrackIndex >= 0 && activeTrackIndex < sequence->getNumTracks())
    {
        const auto order = editableNotes().inTimeOrder(activeTrackIndex);
        const int n = static_cast<int>(order.size());
        int minPos = -1;
        int maxPos = -1;
        for (int p = 0; p < n; ++p)
        {
            if (selectedNotes.contains(order[static_cast<size_t>(p)]))
            {
                if (minPos < 0)
                    minPos = p;
                maxPos = p;
            }
        }
        if (maxPos >= 0)
        {
            if (maxPos + 1 < n)
                survivor = order[static_cast<size_t>(maxPos + 1)];
            else if (minPos - 1 >= 0)
                survivor = order[static_cast<size_t>(minPos - 1)];
        }
    }

    // Deleting shifts the indices of the sequence notes after the deleted ones; recorded note ids stay.
    if (survivor.isValid() && !survivor.isRecorded())
        survivor.noteIndex -= static_cast<int>(std::ranges::count_if(selectedNotes,
                                                                     [&survivor](const NoteRef& ref)
                                                                     {
                                                                         return !ref.isRecorded() &&
                                                                                ref.trackIndex == survivor.trackIndex &&
                                                                                ref.noteIndex < survivor.noteIndex;
                                                                     }));

    const auto deleted = selectedNotes;
    selectedNotes.clear();
    selectedNote = {};
    noteEdits().remove(deleted, "Delete Notes");

    if (survivor.isValid())
    {
        selectedNotes.insert(survivor);
        selectedNote = survivor;
        const auto note = noteAt(survivor);
        if (onScrollToNote)
            onScrollToNote(note.startTick, note.noteNumber);
    }

    repaint();
    if (onNoteSelectionChanged)
        onNoteSelectionChanged(selectedNotes);
}

void PianoRollComponent::copySelectedNotes()
{
    if (!sequence || selectedNotes.empty())
        return;

    int minTick = std::numeric_limits<int>::max();
    for (const auto& ref : selectedNotes)
    {
        const auto note = noteAt(ref);
        if (note.startTick < minTick)
            minTick = note.startTick;
    }

    std::vector<MidiNote> notes;
    for (const auto& ref : selectedNotes)
    {
        MidiNote note = noteAt(ref);
        note.startTick -= minTick;
        notes.push_back(note);
    }
    clipboard.setNotes(std::move(notes));
}

void PianoRollComponent::cutSelectedNotes()
{
    if (!sequence || selectedNotes.empty())
        return;

    copySelectedNotes();

    const auto cut = selectedNotes;
    selectedNotes.clear();
    noteEdits().remove(cut, "Cut Notes");
    repaint();
    if (onNoteSelectionChanged)
        onNoteSelectionChanged(selectedNotes);
}

void PianoRollComponent::pasteNotes(int atTick)
{
    if (!sequence || !clipboard.hasNotes() || activeTrackIndex < 0 || activeTrackIndex >= sequence->getNumTracks())
        return;

    std::vector<MidiNote> notesToAdd;
    for (const auto& note : clipboard.getNotes())
    {
        MidiNote n = note;
        n.startTick += atTick;
        notesToAdd.push_back(n);
    }

    clearTempoSelection();
    clearTimeSignatureSelection();
    clearKeySignatureSelection();
    clearChordSelection();
    selectedNotes.clear();

    undoHistory.beginNewTransaction("Paste Notes");
    auto* action = new MultiNoteAddAction(sequence, activeTrackIndex, notesToAdd);
    undoHistory.perform(action);
    int start = action->getAddedStartIndex();
    for (int i = 0; i < action->getAddedCount(); ++i)
        selectedNotes.insert({activeTrackIndex, start + i});

    repaint();
    if (onNoteSelectionChanged)
        onNoteSelectionChanged(selectedNotes);
}

void PianoRollComponent::selectAllNotes()
{
    if (!sequence || activeTrackIndex < 0 || activeTrackIndex >= sequence->getNumTracks())
        return;

    clearTempoSelection();
    clearTimeSignatureSelection();
    clearKeySignatureSelection();
    clearChordSelection();
    selectedNotes.clear();
    for (const auto& [ref, note] : editableNotes().onTrack(activeTrackIndex))
        selectedNotes.insert(ref);

    repaint();
    if (onNoteSelectionChanged)
        onNoteSelectionChanged(selectedNotes);
}

void PianoRollComponent::nudgeSelectedNotesPitch(int deltaNote)
{
    if (!sequence || selectedNotes.empty() || deltaNote == 0)
        return;

    if (!NoteEdits::canShiftPitch(collectSelectedNotes(), deltaNote))
        return;

    std::vector<NoteEdit> edits;
    for (const auto& ref : selectedNotes)
        edits.push_back({ref, {.noteNumber = noteAt(ref).noteNumber + deltaNote}});
    noteEdits().apply(edits, selectedNotes.size() > 1 ? "Move Notes" : "Move Note");

    NoteRef previewRef =
        (selectedNote.isValid() && selectedNotes.contains(selectedNote)) ? selectedNote : *selectedNotes.begin();
    const auto previewNoteRef = noteAt(previewRef);
    startNotePreview(previewNoteRef);
    startTimer(previewHoldMs);

    if (onScrollToNote)
        onScrollToNote(previewNoteRef.startTick, previewNoteRef.noteNumber);

    repaint();
}

void PianoRollComponent::nudgeSelectedNotesTime(int deltaTick)
{
    if (!sequence || selectedNotes.empty() || deltaTick == 0)
        return;

    if (!NoteEdits::canShiftTime(collectSelectedNotes(), deltaTick))
        return;

    std::vector<NoteEdit> edits;
    for (const auto& ref : selectedNotes)
        edits.push_back({ref, {.startTick = noteAt(ref).startTick + deltaTick}});
    noteEdits().apply(edits, selectedNotes.size() > 1 ? "Move Notes" : "Move Note");

    NoteRef anchorRef =
        (selectedNote.isValid() && selectedNotes.contains(selectedNote)) ? selectedNote : *selectedNotes.begin();
    const auto anchorNote = noteAt(anchorRef);

    if (onScrollToNote)
        onScrollToNote(anchorNote.startTick, anchorNote.noteNumber);

    repaint();
}

void PianoRollComponent::quantizeSelectedNotes()
{
    if (!sequence || selectedNotes.empty())
        return;

    const auto notes = editableNotes();
    std::vector<std::pair<NoteRef, MidiNote>> selected;
    for (const auto& ref : selectedNotes)
        if (const auto note = notes.find(ref))
            selected.emplace_back(ref, *note);

    const auto edits = NoteQuantize::startsToGrid(selected, timelineGrid());
    if (edits.empty())
        return;
    noteEdits().apply(edits, edits.size() > 1 ? "Quantize Notes" : "Quantize Note");
    repaint();
}

void PianoRollComponent::deleteSelectedTempoPoints()
{
    tempoStrip.deleteSelectedTempoPoints();
}

void PianoRollComponent::deleteSelectedTimeSignatures()
{
    timeSigStrip.deleteSelectedTimeSignatures();
}

void PianoRollComponent::deleteSelectedKeySignatures()
{
    keyStrip.deleteSelectedKeySignatures();
}

void PianoRollComponent::deleteSelectedChords()
{
    chordStrip.deleteSelectedChords();
}

bool PianoRollComponent::duplicateSelectedNotesWithPitchOffset(int deltaNote)
{
    if (!sequence || selectedNotes.empty() || activeTrackIndex < 0 || activeTrackIndex >= sequence->getNumTracks())
        return false;

    auto notesToAdd = collectSelectedNotes();
    if (!NoteEdits::canShiftPitch(notesToAdd, deltaNote))
        return false;

    for (auto& note : notesToAdd)
        note.noteNumber += deltaNote;

    selectedNotes.clear();

    undoHistory.beginNewTransaction(notesToAdd.size() > 1 ? "Duplicate Notes" : "Duplicate Note");
    auto* action = new MultiNoteAddAction(sequence, activeTrackIndex, notesToAdd);
    undoHistory.perform(action);
    int start = action->getAddedStartIndex();
    for (int i = 0; i < action->getAddedCount(); ++i)
        selectedNotes.insert({activeTrackIndex, start + i});

    if (selectedNotes.empty())
        return false;
    selectedNote = *selectedNotes.begin();

    const auto previewNoteRef = noteAt(selectedNote);
    startNotePreview(previewNoteRef);
    startTimer(previewHoldMs);

    if (onScrollToNote)
        onScrollToNote(previewNoteRef.startTick, previewNoteRef.noteNumber);

    repaint();
    if (onNoteSelectionChanged)
        onNoteSelectionChanged(selectedNotes);

    return true;
}

void PianoRollComponent::moveSelectionToAdjacentNote(int direction)
{
    if (!sequence || activeTrackIndex < 0 || activeTrackIndex >= sequence->getNumTracks())
        return;

    const auto order = editableNotes().inTimeOrder(activeTrackIndex);
    const int n = static_cast<int>(order.size());
    if (n == 0)
        return;

    auto posOf = [&order](const NoteRef& ref)
    {
        const auto it = std::ranges::find(order, ref);
        return it == order.end() ? -1 : static_cast<int>(it - order.begin());
    };

    std::optional<int> refPos;
    if (selectedNote.isValid() && selectedNote.trackIndex == activeTrackIndex && selectedNotes.contains(selectedNote))
    {
        if (const int p = posOf(selectedNote); p >= 0)
            refPos = p;
    }
    else
    {
        int minPos = -1;
        int maxPos = -1;
        for (const auto& ref : selectedNotes)
        {
            const int p = posOf(ref);
            if (p < 0)
                continue;
            if (minPos < 0 || p < minPos)
                minPos = p;
            if (maxPos < 0 || p > maxPos)
                maxPos = p;
        }
        if (maxPos >= 0)
            refPos = direction > 0 ? maxPos : minPos;
    }

    int targetPos;
    if (refPos)
    {
        const int np = *refPos + (direction > 0 ? 1 : -1);
        if (np < 0 || np >= n)
            return;
        targetPos = np;
    }
    else
    {
        targetPos = direction > 0 ? 0 : n - 1;
    }

    const NoteRef target = order[static_cast<size_t>(targetPos)];
    clearTempoSelection();
    clearTimeSignatureSelection();
    clearKeySignatureSelection();
    clearChordSelection();
    selectedNotes.clear();
    selectedNotes.insert(target);
    selectedNote = target;

    const auto note = noteAt(target);
    startNotePreview(note);
    startTimer(previewHoldMs);

    if (onScrollToNote)
        onScrollToNote(note.startTick, note.noteNumber);

    repaint();
    if (onNoteSelectionChanged)
        onNoteSelectionChanged(selectedNotes);
}

void PianoRollComponent::copySelection()
{
    if (tempoStrip.hasSelection())
        tempoStrip.copySelectedTempoPoints();
    else if (timeSigStrip.hasSelection())
        timeSigStrip.copySelectedTimeSignatures();
    else if (keyStrip.hasSelection())
        keyStrip.copySelectedKeySignatures();
    else if (chordStrip.hasSelection())
        chordStrip.copySelectedChords();
    else
        copySelectedNotes();
}

void PianoRollComponent::cutSelection()
{
    if (tempoStrip.hasSelection())
        tempoStrip.cutSelectedTempoPoints();
    else if (timeSigStrip.hasSelection())
        timeSigStrip.cutSelectedTimeSignatures();
    else if (keyStrip.hasSelection())
        keyStrip.cutSelectedKeySignatures();
    else if (chordStrip.hasSelection())
        chordStrip.cutSelectedChords();
    else
        cutSelectedNotes();
}

void PianoRollComponent::paste(int atTick)
{
    if (clipboard.hasTempoPoints())
    {
        clearNoteSelection();
        clearTimeSignatureSelection();
        clearKeySignatureSelection();
        clearChordSelection();
        tempoStrip.pasteTempoPoints(atTick);
        repaint();
    }
    else if (clipboard.hasTimeSignatures())
    {
        clearNoteSelection();
        tempoStrip.clearTempoSelection();
        clearKeySignatureSelection();
        clearChordSelection();
        timeSigStrip.pasteTimeSignatures(atTick);
        repaint();
    }
    else if (clipboard.hasKeySignatures())
    {
        clearNoteSelection();
        tempoStrip.clearTempoSelection();
        clearTimeSignatureSelection();
        clearChordSelection();
        keyStrip.pasteKeySignatures(atTick);
        repaint();
    }
    else if (clipboard.hasChords())
    {
        clearNoteSelection();
        tempoStrip.clearTempoSelection();
        clearTimeSignatureSelection();
        clearKeySignatureSelection();
        chordStrip.pasteChords(atTick);
        repaint();
    }
    else
        pasteNotes(atTick);
}

bool PianoRollComponent::hasNotesInActiveTrack() const
{
    if (!sequence || activeTrackIndex < 0 || activeTrackIndex >= sequence->getNumTracks())
        return false;

    return !editableNotes().onTrack(activeTrackIndex).empty();
}

void PianoRollComponent::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    g.fillAll(surface::surface);
    // At fractional display scales the strips only partly cover their edge pixels, so draw nothing beneath them.
    g.reduceClipRegion(getLocalBounds().withTop(loopStrip.getY() + gridTopOffset));
    drawGrid(g);
    drawLoopRegion(g);
    drawNotes(g);
    drawRecordingLayer(g);
    drawMoveGhosts(g);
    drawRubberBand(g);
    drawPlayhead(g);
    drawKeyboard(g);
}

void PianoRollComponent::resized()
{
    updateStripPositions();
}

void PianoRollComponent::moved()
{
    updateStripPositions();
}

void PianoRollComponent::mouseDown(const juce::MouseEvent& e)
{
    if (!sequence || sequence->getNumTracks() == 0)
        return;

    toolSwapActive = e.mods.isCommandDown();
    updateEffectiveEditMode();

    if (e.y < getRulerTop() + gridTopOffset)
        return;

    if (e.x < getKeyboardLeft() + keyboardWidth)
    {
        int noteNum = keyboardNoteAtPosition(e.x, e.y);
        if (noteNum >= 0)
        {
            startNotePreview(MidiNote{noteNum, 100, 0, 480});
            drag = KeyboardPreviewing{};
        }
        return;
    }

    clearTempoSelection();
    clearTimeSignatureSelection();
    clearKeySignatureSelection();
    clearChordSelection();

    auto hit = hitTestNote(e.x, e.y);

    if (editMode == EditMode::Edit)
    {
        if (hit.isValid())
        {
            if (!e.mods.isRightButtonDown())
            {
                auto edge = edgeAt(e.x, noteAt(hit));
                if (edge != ResizeEdge::None)
                {
                    beginResize(hit, edge);
                    return;
                }
            }

            selectedNote = {};
            selectedNotes.clear();
            noteEdits().remove({hit}, "Delete Note");
            repaint();
            if (onNoteSelectionChanged)
                onNoteSelectionChanged(selectedNotes);
            return;
        }

        if (e.mods.isRightButtonDown())
            return;

        int tick = snapGrid(e.mods).floor(xToTick(e.x));
        int noteNum = yToNote(e.y);
        if (noteNum < 0 || noteNum > 127)
            return;

        int defaultDuration = geometry.gridTicks();
        MidiNote newNote{noteNum, 100, tick, defaultDuration};

        selectedNote = {};
        selectedNotes.clear();
        if (onNoteSelectionChanged)
            onNoteSelectionChanged(selectedNotes);
        startNotePreview(newNote);

        drag = Creating{activeTrackIndex, newNote, newNote.duration};
        repaint();
    }
    else // Select mode
    {
        if (e.mods.isRightButtonDown())
        {
            if (!selectedNotes.empty())
            {
                const auto deleted = selectedNotes;
                selectedNote = {};
                selectedNotes.clear();
                noteEdits().remove(deleted, "Delete Notes");
                repaint();
                if (onNoteSelectionChanged)
                    onNoteSelectionChanged(selectedNotes);
            }
            return;
        }

        if (hit.isValid())
        {
            const auto edge = edgeAt(e.x, noteAt(hit));
            if (edge != ResizeEdge::None && !e.mods.isShiftDown())
            {
                beginResize(hit, edge);
                return;
            }

            if (e.mods.isShiftDown())
            {
                if (edge != ResizeEdge::None)
                    drag = PendingResize{hit, edge};
                else
                {
                    toggleNoteSelection(hit);
                    drag = Idle{};
                }
            }
            else
            {
                if (!selectedNotes.contains(hit))
                {
                    selectedNotes.clear();
                    selectedNotes.insert(hit);
                }
                beginMove(hit, e);
            }
            const auto note = noteAt(hit);
            startNotePreview(note);
            repaint();
            if (onNoteSelectionChanged)
                onNoteSelectionChanged(selectedNotes);
        }
        else
        {
            if (!e.mods.isShiftDown())
                selectedNotes.clear();

            drag = RubberBand{e.getPosition(), {}};
            repaint();
        }
    }
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!sequence)
        return;

    if (std::holds_alternative<KeyboardPreviewing>(drag))
    {
        int noteNum = keyboardNoteAtPosition(e.x, e.y);
        if (noteNum >= 0 && noteNum != previewNote.noteNumber)
            startNotePreview(MidiNote{noteNum, 100, 0, 480});
        return;
    }

    if (auto* band = std::get_if<RubberBand>(&drag))
    {
        auto current = e.getPosition();
        int x = std::min(band->start.x, current.x);
        int y = std::min(band->start.y, current.y);
        int w = std::abs(current.x - band->start.x);
        int h = std::abs(current.y - band->start.y);
        band->rect = {x, y, w, h};
        repaint();
        return;
    }

    // Without a snap to absorb it, a click's jitter would otherwise move or resize notes.
    if (!e.mouseWasDraggedSinceMouseDown())
        return;

    if (const auto* pending = std::get_if<PendingResize>(&drag))
    {
        const auto [hit, edge] = *pending;
        beginResize(hit, edge);
    }

    if (auto* resizing = std::get_if<Resizing>(&drag))
    {
        if (resizing->targets.empty())
            return;

        const TimelineGrid grid = snapGrid(e.mods);
        int currentTick = grid.round(xToTick(e.x));

        if (resizing->edge == ResizeEdge::Right)
        {
            int delta = currentTick - resizing->anchorEndTick;
            for (auto& t : resizing->targets)
            {
                const auto resized =
                    NoteEdits::afterEndResize({.startTick = t.startTick, .duration = t.duration}, delta, grid);
                t.previewStartTick = resized.startTick;
                t.previewDuration = resized.duration;
            }
        }
        else if (resizing->edge == ResizeEdge::Left)
        {
            int delta = currentTick - resizing->anchorStartTick;
            for (auto& t : resizing->targets)
            {
                const auto resized =
                    NoteEdits::afterStartResize({.startTick = t.startTick, .duration = t.duration}, delta, grid);
                t.previewStartTick = resized.startTick;
                t.previewDuration = resized.duration;
            }
        }

        repaint();
        return;
    }

    if (auto* creating = std::get_if<Creating>(&drag))
    {
        const TimelineGrid grid = snapGrid(e.mods);
        int currentTick = grid.round(xToTick(e.x));
        creating->previewDuration =
            NoteEdits::afterEndResize(creating->note, currentTick - creating->note.endTick(), grid).duration;
        repaint();
        return;
    }

    auto* moving = std::get_if<Moving>(&drag);
    if (moving == nullptr || moving->targets.empty())
        return;

    int currentTick = xToTick(e.x);
    int currentNote = yToNote(e.y);
    int rawDeltaTick = currentTick - moving->dragStartTick;
    int deltaNote = currentNote - moving->dragStartNote;
    int snappedDeltaTick = snapGrid(e.mods).round(moving->anchorStartTick + rawDeltaTick) - moving->anchorStartTick;

    const auto origins =
        moving->targets |
        std::views::transform([](const MoveTarget& t)
                              { return MidiNote{.noteNumber = t.noteNumber, .startTick = t.startTick}; }) |
        std::ranges::to<std::vector>();
    const auto shift = NoteEdits::clampShift(origins, {snappedDeltaTick, deltaNote});

    moving->deltaTick = shift.deltaTick;
    moving->deltaNote = shift.deltaNote;

    const auto anchorNote = noteAt(selectedNote);
    int previewPitch = anchorNote.noteNumber + moving->deltaNote;
    if (previewPitch != previewNote.noteNumber)
    {
        MidiNote ghost = anchorNote;
        ghost.noteNumber = previewPitch;
        startNotePreview(ghost);
    }

    repaint();
}

void PianoRollComponent::mouseUp(const juce::MouseEvent&)
{
    stopNotePreview();
    auto state = std::exchange(drag, Idle{});

    if (const auto* band = std::get_if<RubberBand>(&state))
    {
        if (!band->rect.isEmpty())
        {
            auto found = findNotesInRect(band->rect);
            for (const auto& ref : found)
                selectedNotes.insert(ref);
        }
        repaint();
        if (onNoteSelectionChanged)
            onNoteSelectionChanged(selectedNotes);
        return;
    }

    if (const auto* pending = std::get_if<PendingResize>(&state))
    {
        toggleNoteSelection(pending->hit);
        repaint();
        if (onNoteSelectionChanged)
            onNoteSelectionChanged(selectedNotes);
        return;
    }

    if (const auto* resizing = std::get_if<Resizing>(&state))
    {
        std::vector<NoteEdit> edits;
        for (const auto& t : resizing->targets)
        {
            if (t.previewStartTick == t.startTick && t.previewDuration == t.duration)
                continue;
            if (resizing->edge == ResizeEdge::Left)
                edits.push_back({t.ref, {.startTick = t.previewStartTick, .keepEnd = true}});
            else
                edits.push_back({t.ref, {.endTick = t.previewStartTick + t.previewDuration}});
        }
        if (!edits.empty())
            noteEdits().apply(edits, edits.size() > 1 ? "Resize Notes" : "Resize Note");
        return;
    }

    if (const auto* creating = std::get_if<Creating>(&state))
    {
        MidiNote newNote = creating->note;
        newNote.duration = creating->previewDuration;

        undoHistory.beginNewTransaction("Add Note");
        auto* action = new NoteAddAction(sequence, creating->trackIndex, newNote);
        undoHistory.perform(action);
        selectedNote = {creating->trackIndex, action->getAddedIndex()};
        selectedNotes.clear();
        selectedNotes.insert(selectedNote);
        repaint();
        if (onNoteSelectionChanged)
            onNoteSelectionChanged(selectedNotes);
        return;
    }

    if (const auto* moving = std::get_if<Moving>(&state))
    {
        if (moving->deltaTick != 0 || moving->deltaNote != 0)
        {
            std::vector<NoteEdit> edits;
            for (const auto& t : moving->targets)
                edits.push_back({t.ref,
                                 {.startTick = t.startTick + moving->deltaTick,
                                  .noteNumber = t.noteNumber + moving->deltaNote,
                                  .heldLength = t.duration}});
            noteEdits().apply(edits, moving->targets.size() > 1 ? "Move Notes" : "Move Note");
        }
    }
}

void PianoRollComponent::mouseMove(const juce::MouseEvent& e)
{
    if (!sequence || e.x < getKeyboardLeft() + keyboardWidth || e.y < getRulerTop() + gridTopOffset)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    auto hit = hitTestNote(e.x, e.y);

    if (hit.isValid())
    {
        auto edge = edgeAt(e.x, noteAt(hit));
        if (edge != ResizeEdge::None)
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        else
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void PianoRollComponent::modifierKeysChanged(const juce::ModifierKeys& modifiers)
{
    const bool altDown = modifiers.isAltDown();
    if (altKeyDown && !altDown)
        altDuplicateDone = false;
    altKeyDown = altDown;

    const bool modifierDown = modifiers.isCommandDown();
    if (modifierDown == toolSwapActive)
        return;

    toolSwapActive = modifierDown;

    if (!std::holds_alternative<Idle>(drag))
        return;

    updateEffectiveEditMode();
}

bool PianoRollComponent::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::leftKey)
    {
        moveSelectionToAdjacentNote(-1);
        return true;
    }
    if (key == juce::KeyPress::rightKey)
    {
        moveSelectionToAdjacentNote(1);
        return true;
    }

    if (key == juce::KeyPress::backspaceKey || key == juce::KeyPress::deleteKey)
    {
        if (tempoStrip.hasSelection())
        {
            deleteSelectedTempoPoints();
            return true;
        }
        if (timeSigStrip.hasSelection())
        {
            deleteSelectedTimeSignatures();
            return true;
        }
        if (keyStrip.hasSelection())
        {
            deleteSelectedKeySignatures();
            return true;
        }
        if (chordStrip.hasSelection())
        {
            deleteSelectedChords();
            return true;
        }
        if (selectedNotes.empty())
            return false;
        deleteSelectedNotes();
        return true;
    }

    if (key.getModifiers().isCommandDown())
    {
        if (key.getKeyCode() == juce::KeyPress::upKey)
        {
            if (onScrollVertical)
                onScrollVertical(-defaultNoteHeight);
            return true;
        }
        if (key.getKeyCode() == juce::KeyPress::downKey)
        {
            if (onScrollVertical)
                onScrollVertical(defaultNoteHeight);
            return true;
        }
        if (key.getKeyCode() == juce::KeyPress::leftKey)
        {
            if (onScrollHorizontal)
                onScrollHorizontal(-defaultNoteHeight);
            return true;
        }
        if (key.getKeyCode() == juce::KeyPress::rightKey)
        {
            if (onScrollHorizontal)
                onScrollHorizontal(defaultNoteHeight);
            return true;
        }
    }

    if (key.getModifiers().isAltDown())
    {
        const bool isUp = key.getKeyCode() == juce::KeyPress::upKey;
        const bool isDown = key.getKeyCode() == juce::KeyPress::downKey;
        if (isUp || isDown)
        {
            const int magnitude = key.getModifiers().isShiftDown() ? 12 : 1;
            const int delta = isUp ? magnitude : -magnitude;
            if (!selectedNotes.empty())
            {
                if (altDuplicateDone)
                    nudgeSelectedNotesPitch(delta);
                else if (duplicateSelectedNotesWithPitchOffset(delta))
                    altDuplicateDone = true;
            }
            return true;
        }

        const bool isLeft = key.getKeyCode() == juce::KeyPress::leftKey;
        const bool isRight = key.getKeyCode() == juce::KeyPress::rightKey;
        if (isLeft || isRight)
        {
            if (!selectedNotes.empty() && sequence)
            {
                const int grid = geometry.gridTicks();
                nudgeSelectedNotesTime(isRight ? grid : -grid);
            }
            return true;
        }
    }

    if (key.getModifiers().isShiftDown())
    {
        if (key.getKeyCode() == juce::KeyPress::upKey)
        {
            if (!selectedNotes.empty())
                nudgeSelectedNotesPitch(12);
            return true;
        }
        if (key.getKeyCode() == juce::KeyPress::downKey)
        {
            if (!selectedNotes.empty())
                nudgeSelectedNotesPitch(-12);
            return true;
        }
    }

    if (key == juce::KeyPress::upKey)
    {
        if (!selectedNotes.empty())
            nudgeSelectedNotesPitch(1);
        return true;
    }
    if (key == juce::KeyPress::downKey)
    {
        if (!selectedNotes.empty())
            nudgeSelectedNotesPitch(-1);
        return true;
    }
    return false;
}

void PianoRollComponent::setBeatWidth(int w)
{
    beatWidth = juce::jlimit(minBeatWidth, maxBeatWidth, w);
    geometry.setBeatWidth(beatWidth);
    repaintStrips();
    updateSize();
    repaint();
    if (onZoomChanged)
        onZoomChanged();
}

void PianoRollComponent::setNoteHeight(int h)
{
    noteHeight = juce::jlimit(minNoteHeight, maxNoteHeight, h);
    updateSize();
    repaint();
    if (onZoomChanged)
        onZoomChanged();
}

void PianoRollComponent::setGridDivision(GridDivision division)
{
    geometry.setGridDivision(division);
    repaintStrips();
    repaint();
}

void PianoRollComponent::setSnapEnabled(bool enabled)
{
    geometry.setSnapEnabled(enabled);
}

int PianoRollComponent::tickToX(int tick) const
{
    return geometry.tickToX(tick);
}

int PianoRollComponent::noteToY(int noteNumber) const
{
    return gridTopOffset + (totalNotes - 1 - noteNumber) * noteHeight;
}

int PianoRollComponent::xToTick(int x) const
{
    return geometry.xToTick(x);
}

int PianoRollComponent::yToNote(int y) const
{
    return totalNotes - 1 - ((y - gridTopOffset) / noteHeight);
}

void PianoRollComponent::updateSize()
{
    if (!sequence)
        return;

    int width = keyboardWidth + contentBeats * beatWidth;

    if (auto* vp = findParentComponentOfClass<juce::Viewport>())
        width = std::max(width, vp->getMaximumVisibleWidth());

    int height = gridTopOffset + totalNotes * noteHeight;
    setSize(width, height);
}

void PianoRollComponent::extendContent()
{
    contentBeats += 32;
    updateSize();
}

void PianoRollComponent::notesChanged(int)
{
    cancelEditDrag();
    repaint();
}
void PianoRollComponent::tracksChanged()
{
    cancelEditDrag();
    repaint();
}
void PianoRollComponent::tempoChanged()
{
    cancelEditDrag();
    repaint();
}
void PianoRollComponent::timelineMetadataChanged()
{
    cancelEditDrag();
    repaint();
}
void PianoRollComponent::sequenceReset()
{
    cancelEditDrag();
    repaint();
}

void PianoRollComponent::updateStripPositions()
{
    int viewX = 0;
    int viewY = 0;
    if (findParentComponentOfClass<juce::Viewport>() != nullptr)
    {
        const auto viewPosition = getLocalPoint(getParentComponent(), juce::Point<int>{});
        viewX = viewPosition.x;
        viewY = viewPosition.y;
    }

    loopStrip.setBounds(0, viewY, getWidth(), LoopStrip::height);
    loopStrip.setViewLeftX(viewX);
    ruler.setBounds(0, viewY + LoopStrip::height, getWidth(), RulerStrip::height);
    ruler.setViewLeftX(viewX);
    tempoStrip.setBounds(0, viewY + LoopStrip::height + RulerStrip::height, getWidth(), TempoTrackStrip::height);
    tempoStrip.setViewLeftX(viewX);
    timeSigStrip.setBounds(0, viewY + LoopStrip::height + RulerStrip::height + TempoTrackStrip::height, getWidth(),
                           TimeSignatureStrip::height);
    timeSigStrip.setViewLeftX(viewX);
    keyStrip.setBounds(
        0, viewY + LoopStrip::height + RulerStrip::height + TempoTrackStrip::height + TimeSignatureStrip::height,
        getWidth(), KeySignatureStrip::height);
    keyStrip.setViewLeftX(viewX);
    chordStrip.setBounds(0,
                         viewY + LoopStrip::height + RulerStrip::height + TempoTrackStrip::height +
                             TimeSignatureStrip::height + KeySignatureStrip::height,
                         getWidth(), ChordStrip::height);
    chordStrip.setViewLeftX(viewX);
}

void PianoRollComponent::repaintStrips()
{
    loopStrip.repaint();
    ruler.repaint();
    tempoStrip.repaint();
    timeSigStrip.repaint();
    keyStrip.repaint();
    chordStrip.repaint();
}

void PianoRollComponent::drawKeyboard(juce::Graphics& g)
{
    using namespace calliope::theme;
    int kbLeft = getKeyboardLeft();
    int blackKeyW = keyboardWidth * 55 / 100;

    auto clip = g.getClipBounds();

    g.setColour(text::t1);
    g.fillRect(kbLeft, clip.getY(), keyboardWidth, clip.getHeight());

    double whiteKeyH = 12.0 * noteHeight / 7.0;
    int minOctave = std::max(0, yToNote(clip.getBottom())) / 12;
    int maxOctave = std::min(totalNotes - 1, yToNote(clip.getY())) / 12;

    g.setColour(text::t2);
    for (int oct = minOctave; oct <= maxOctave; ++oct)
    {
        double octBottom = static_cast<double>(noteToY(oct * 12) + noteHeight);
        for (int wk = 0; wk <= 7; ++wk)
        {
            int y = static_cast<int>(octBottom - wk * whiteKeyH + 0.5);
            if (y >= clip.getY() && y <= clip.getBottom())
                g.drawHorizontalLine(y, static_cast<float>(kbLeft), static_cast<float>(kbLeft + keyboardWidth));
        }
    }

    static const int bkSemitones[] = {1, 3, 6, 8, 10};
    int bkH = static_cast<int>(whiteKeyH * 0.65);
    for (int oct = minOctave; oct <= maxOctave; ++oct)
    {
        int base = oct * 12;
        for (int i = 0; i < 5; ++i)
        {
            double centerY = noteToY(base + bkSemitones[i]) + noteHeight / 2.0;
            int bkTop = static_cast<int>(centerY - bkH / 2.0);
            if (bkTop + bkH < clip.getY() || bkTop > clip.getBottom())
                continue;

            g.setColour(surface::bg);
            g.fillRect(kbLeft, bkTop, blackKeyW, bkH);
        }
    }

    g.setColour(border::strong);
    g.setFont(font::sans(font::sizeXS));
    for (int oct = minOctave; oct <= maxOctave; ++oct)
    {
        int base = oct * 12;
        double octBottom = static_cast<double>(noteToY(base) + noteHeight);
        int cTop = static_cast<int>(octBottom - whiteKeyH + 0.5);
        int cH = static_cast<int>(whiteKeyH);
        if (cTop + cH >= clip.getY() && cTop <= clip.getBottom())
            g.drawText(getNoteName(base), kbLeft, cTop, keyboardWidth - 4, cH, juce::Justification::centredRight);
    }

    if (isPreviewing)
    {
        int pn = previewNote.noteNumber;
        if (isBlackKey(pn))
        {
            double centerY = noteToY(pn) + noteHeight / 2.0;
            int bkTop = static_cast<int>(centerY - bkH / 2.0);
            g.setColour(accent::base.withAlpha(0.5f));
            g.fillRect(kbLeft, bkTop, blackKeyW, bkH);
        }
        else
        {
            static const int whiteKeyIndex[] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
            int oct = pn / 12;
            int wk = whiteKeyIndex[pn % 12];
            if (wk >= 0)
            {
                double octBottom = static_cast<double>(noteToY(oct * 12) + noteHeight);
                int wkTop = static_cast<int>(octBottom - (wk + 1) * whiteKeyH + 0.5);
                int wkBottom = static_cast<int>(octBottom - wk * whiteKeyH + 0.5);
                g.setColour(accent::base.withAlpha(0.3f));
                g.fillRect(kbLeft, wkTop, keyboardWidth, wkBottom - wkTop);
            }
        }
    }

    g.setColour(border::normal);
    g.drawVerticalLine(kbLeft + keyboardWidth - 1, static_cast<float>(clip.getY()),
                       static_cast<float>(clip.getBottom()));
}

void PianoRollComponent::drawGrid(juce::Graphics& g)
{
    using namespace calliope::theme;
    auto clip = g.getClipBounds();
    int visibleLeft = clip.getX();
    int visibleRight = clip.getRight();
    int visibleTop = clip.getY();
    int visibleBottom = clip.getBottom();

    int gridLeft = std::max(keyboardWidth, visibleLeft);
    int gridRight = visibleRight;

    int minNote = std::max(0, yToNote(visibleBottom));
    int maxNote = std::min(totalNotes - 1, yToNote(visibleTop));

    for (int note = minNote; note <= maxNote; ++note)
    {
        int y = noteToY(note);
        bool black = isBlackKey(note);

        g.setColour(black ? surface::bg : surface::surface2);
        g.fillRect(gridLeft, y, gridRight - gridLeft, noteHeight);
    }

    for (int note = minNote; note <= maxNote; ++note)
    {
        int y = noteToY(note) + noteHeight;
        bool isC = (note % 12 == 0);

        g.setColour(isC ? border::strong : border::normal);
        g.drawHorizontalLine(y, static_cast<float>(gridLeft), static_cast<float>(gridRight));
    }

    for (const auto& line : timelineGrid().lines(xToTick(gridLeft - 1), xToTick(visibleRight + 1)))
    {
        const int x = tickToX(line.tick);
        if (x < visibleLeft || x > visibleRight)
            continue;
        g.setColour(gridLineColour(line.kind));
        g.drawVerticalLine(x, static_cast<float>(visibleTop), static_cast<float>(visibleBottom));
    }
}

void PianoRollComponent::drawNotes(juce::Graphics& g)
{
    if (!sequence)
        return;

    auto clip = g.getClipBounds();

    for (int trackIdx : selectedTrackIndices)
    {
        if (trackIdx == activeTrackIndex)
            continue;
        if (trackIdx < 0 || trackIdx >= sequence->getNumTracks())
            continue;

        const auto& track = sequence->getTrack(trackIdx);
        float alpha = 0.4f;

        for (int i = 0; i < track.getNumNotes(); ++i)
        {
            const auto note = displayedNote({trackIdx, i});
            auto baseColour = TrackColours::getColour(trackIdx).withAlpha(alpha);
            int x = tickToX(note.startTick);
            int y = noteToY(note.noteNumber);
            int w = tickToWidth(note.duration);

            if (y + noteHeight < clip.getY() || y > clip.getBottom())
                continue;

            bool isSelected = isNoteSelected({trackIdx, i});

            if (track.getChannel() == 10)
            {
                float diameter = static_cast<float>(noteHeight - 2);
                float cx = static_cast<float>(x);
                float cy = static_cast<float>(y + 1) + diameter * 0.5f;
                float left = cx - diameter * 0.5f;

                if (left + diameter < clip.getX() || left > clip.getRight())
                    continue;

                g.setColour(isSelected ? baseColour.brighter(0.4f) : baseColour);
                g.fillEllipse(left, cy - diameter * 0.5f, diameter, diameter);

                g.setColour((isSelected ? baseColour.brighter(0.7f) : baseColour.darker(0.3f)).withAlpha(alpha));
                g.drawEllipse(left, cy - diameter * 0.5f, diameter, diameter, 1.0f);
            }
            else
            {
                if (x + w < clip.getX() || x > clip.getRight())
                    continue;

                g.setColour(isSelected ? baseColour.brighter(0.4f) : baseColour);
                g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(w),
                                       static_cast<float>(noteHeight - 2), 2.0f);

                g.setColour((isSelected ? baseColour.brighter(0.7f) : baseColour.darker(0.3f)).withAlpha(alpha));
                g.drawRoundedRectangle(static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(w),
                                       static_cast<float>(noteHeight - 2), 2.0f, 1.0f);
            }
        }
    }

    if (activeTrackIndex >= 0 && activeTrackIndex < sequence->getNumTracks() &&
        selectedTrackIndices.contains(activeTrackIndex))
    {
        const auto& track = sequence->getTrack(activeTrackIndex);
        const auto baseColour = TrackColours::getColour(activeTrackIndex);
        const bool isDrum = track.getChannel() == 10;

        for (int i = 0; i < track.getNumNotes(); ++i)
            drawActiveTrackNote(g, clip, displayedNote({activeTrackIndex, i}), baseColour,
                                isNoteSelected({activeTrackIndex, i}), isDrum);

        if (const auto* creating = std::get_if<Creating>(&drag);
            creating != nullptr && creating->trackIndex == activeTrackIndex)
        {
            MidiNote preview = creating->note;
            preview.duration = creating->previewDuration;
            drawActiveTrackNote(g, clip, preview, baseColour, true, isDrum);
        }
    }
}

void PianoRollComponent::drawActiveTrackNote(juce::Graphics& g, const juce::Rectangle<int>& clip, const MidiNote& note,
                                             juce::Colour baseColour, bool isSelected, bool isDrum)
{
    int x = tickToX(note.startTick);
    int y = noteToY(note.noteNumber);
    int w = tickToWidth(note.duration);

    if (y + noteHeight < clip.getY() || y > clip.getBottom())
        return;

    if (isDrum)
    {
        float diameter = static_cast<float>(noteHeight - 2);
        float cx = static_cast<float>(x);
        float cy = static_cast<float>(y + 1) + diameter * 0.5f;
        float left = cx - diameter * 0.5f;

        if (left + diameter < clip.getX() || left > clip.getRight())
            return;

        g.setColour(isSelected ? baseColour.brighter(0.4f) : baseColour);
        g.fillEllipse(left, cy - diameter * 0.5f, diameter, diameter);

        g.setColour(isSelected ? baseColour.brighter(0.7f) : baseColour.darker(0.3f));
        g.drawEllipse(left, cy - diameter * 0.5f, diameter, diameter, 1.0f);
    }
    else
    {
        if (x + w < clip.getX() || x > clip.getRight())
            return;

        g.setColour(isSelected ? baseColour.brighter(0.4f) : baseColour);
        g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(w),
                               static_cast<float>(noteHeight - 2), 2.0f);

        g.setColour(isSelected ? baseColour.brighter(0.7f) : baseColour.darker(0.3f));
        g.drawRoundedRectangle(static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(w),
                               static_cast<float>(noteHeight - 2), 2.0f, 1.0f);
    }
}

EditableNotes PianoRollComponent::editableNotes() const
{
    return {*sequence, recordingLayer, static_cast<int>(playheadTick)};
}

MidiNote PianoRollComponent::noteAt(const NoteRef& ref) const
{
    return editableNotes().find(ref).value_or(MidiNote{});
}

LayeredNoteEdits PianoRollComponent::noteEdits()
{
    return {*sequence, undoHistory, recordingEditor};
}

MidiNote PianoRollComponent::displayedNote(const NoteRef& ref) const
{
    return displayedNote(ref, noteAt(ref));
}

MidiNote PianoRollComponent::displayedNote(const NoteRef& ref, MidiNote note) const
{
    if (const auto* resizing = std::get_if<Resizing>(&drag))
    {
        // beginResize builds the targets from a std::set, so they are sorted by ref.
        auto it = std::ranges::lower_bound(resizing->targets, ref, {}, &ResizeTarget::ref);
        if (it != resizing->targets.end() && it->ref == ref)
        {
            // A resize of the start keeps the current end, which for a held note follows the playhead.
            const int end = note.endTick();
            note.startTick = it->previewStartTick;
            note.duration =
                resizing->edge == ResizeEdge::Left ? std::max(1, end - it->previewStartTick) : it->previewDuration;
        }
    }
    return note;
}

void PianoRollComponent::drawMoveGhosts(juce::Graphics& g)
{
    const auto* moving = std::get_if<Moving>(&drag);
    if (!sequence || moving == nullptr || moving->targets.empty())
        return;
    if (moving->deltaTick == 0 && moving->deltaNote == 0)
        return;

    auto clip = g.getClipBounds();
    const auto notes = editableNotes();
    const int playhead = static_cast<int>(playheadTick);

    for (const auto& t : moving->targets)
    {
        if (t.ref.trackIndex < 0 || t.ref.trackIndex >= sequence->getNumTracks())
            continue;

        const auto& track = sequence->getTrack(t.ref.trackIndex);
        const auto note = notes.find(t.ref).value_or(MidiNote{});
        const int startTick = t.startTick + moving->deltaTick;
        // Show the length the drop gives: a held note dropped before the playhead stays held up to the playhead, and
        // one dropped at the playhead or later ends with its length at the grab.
        int duration = note.duration;
        if (notes.isHeld(t.ref))
            duration = startTick < playhead ? std::max(1, playhead - startTick) : t.duration;

        int x = tickToX(startTick);
        int y = noteToY(t.noteNumber + moving->deltaNote);
        int w = tickToWidth(duration);

        if (y + noteHeight < clip.getY() || y > clip.getBottom())
            continue;

        auto baseColour = TrackColours::getColour(t.ref.trackIndex);
        auto fillColour = baseColour.brighter(0.2f).withAlpha(0.6f);
        auto outlineColour = baseColour.brighter(0.8f);

        if (track.getChannel() == 10)
        {
            float diameter = static_cast<float>(noteHeight - 2);
            float cx = static_cast<float>(x);
            float cy = static_cast<float>(y + 1) + diameter * 0.5f;
            float left = cx - diameter * 0.5f;

            if (left + diameter < clip.getX() || left > clip.getRight())
                continue;

            g.setColour(fillColour);
            g.fillEllipse(left, cy - diameter * 0.5f, diameter, diameter);
            g.setColour(outlineColour);
            g.drawEllipse(left, cy - diameter * 0.5f, diameter, diameter, 1.0f);
        }
        else
        {
            if (x + w < clip.getX() || x > clip.getRight())
                continue;

            g.setColour(fillColour);
            g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(w),
                                   static_cast<float>(noteHeight - 2), 2.0f);
            g.setColour(outlineColour);
            g.drawRoundedRectangle(static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(w),
                                   static_cast<float>(noteHeight - 2), 2.0f, 1.0f);
        }
    }
}

void PianoRollComponent::drawPlayhead(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (!sequence)
        return;
    float x =
        static_cast<float>(keyboardWidth + playheadTick / sequence->getTimeline().getTicksPerQuarterNote() * beatWidth);
    auto clip = g.getClipBounds();
    if (x < static_cast<float>(clip.getX()) - 1.0f || x > static_cast<float>(clip.getRight()) + 1.0f)
        return;
    g.setColour(text::t1);
    g.drawLine(x, static_cast<float>(clip.getY()), x, static_cast<float>(clip.getBottom()), 1.0f);
}

void PianoRollComponent::drawLoopRegion(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (!sequence || loopEndTick <= loopStartTick)
        return;

    auto clip = g.getClipBounds();
    float x1 = static_cast<float>(tickToX(loopStartTick));
    float x2 = static_cast<float>(tickToX(loopEndTick));

    if (x2 < static_cast<float>(clip.getX()) || x1 > static_cast<float>(clip.getRight()))
        return;

    int gridTopY = getRulerTop() + gridTopOffset;
    float gridTop = static_cast<float>(gridTopY);
    float bottom = static_cast<float>(clip.getBottom());

    g.setColour(loopEnabled ? accent::soft : surface::hover);
    g.fillRect(x1, gridTop, x2 - x1, bottom - gridTop);

    g.setColour(loopEnabled ? accent::base.withAlpha(0.7f) : text::t4);
    g.drawVerticalLine(static_cast<int>(x1), gridTop, bottom);
    g.drawVerticalLine(static_cast<int>(x2), gridTop, bottom);
}

int PianoRollComponent::tickToWidth(int durationTicks) const
{
    return geometry.tickToWidth(durationTicks);
}

int PianoRollComponent::recordedNoteWidth(int durationTicks) const
{
    return std::max(2, tickToWidth(durationTicks));
}

TimelineGrid PianoRollComponent::timelineGrid() const
{
    return {displayedTimeline.get(), geometry.gridDivision()};
}

TimelineGrid PianoRollComponent::snapGrid(const juce::ModifierKeys& mods) const
{
    if (geometry.snaps(mods.isShiftDown()))
        return timelineGrid();
    return TimelineGrid::everyTick(displayedTimeline.get());
}

NoteRef PianoRollComponent::hitTestNote(int x, int y) const
{
    if (!sequence)
        return {};

    if (activeTrackIndex >= 0 && activeTrackIndex < sequence->getNumTracks() &&
        selectedTrackIndices.contains(activeTrackIndex))
    {
        // Recorded notes are drawn over the sequence's notes, so they are hit first.
        NoteRef sequenceHit;
        for (const auto& [ref, note] : editableNotes().onTrack(activeTrackIndex))
        {
            int nx = tickToX(note.startTick);
            int ny = noteToY(note.noteNumber);
            int nw = ref.isRecorded() ? recordedNoteWidth(note.duration) : tickToWidth(note.duration);
            if (x < nx || x > nx + nw || y < ny || y >= ny + noteHeight)
                continue;
            if (ref.isRecorded())
                return ref;
            if (!sequenceHit.isValid())
                sequenceHit = ref;
        }
        if (sequenceHit.isValid())
            return sequenceHit;
    }

    for (int trackIdx : selectedTrackIndices)
    {
        if (trackIdx == activeTrackIndex)
            continue;
        if (trackIdx < 0 || trackIdx >= sequence->getNumTracks())
            continue;

        const auto& track = sequence->getTrack(trackIdx);
        for (int i = 0; i < track.getNumNotes(); ++i)
        {
            const auto& note = track.getNote(i);
            int nx = tickToX(note.startTick);
            int ny = noteToY(note.noteNumber);
            int nw = tickToWidth(note.duration);

            if (x >= nx && x <= nx + nw && y >= ny && y < ny + noteHeight)
                return {trackIdx, i};
        }
    }

    return {};
}

int PianoRollComponent::keyboardNoteAtPosition(int x, int y) const
{
    int kbLeft = getKeyboardLeft();
    int blackKeyW = keyboardWidth * 55 / 100;
    double whiteKeyH = 12.0 * noteHeight / 7.0;
    int bkH = static_cast<int>(whiteKeyH * 0.65);

    int roughNote = std::clamp(yToNote(y), 0, totalNotes - 1);
    int oct = roughNote / 12;
    int minOct = std::max(0, oct - 1);
    int maxOct = std::min((totalNotes - 1) / 12, oct + 1);

    if (x - kbLeft < blackKeyW)
    {
        static const int bkSemitones[] = {1, 3, 6, 8, 10};
        for (int o = minOct; o <= maxOct; ++o)
        {
            for (int i = 0; i < 5; ++i)
            {
                int noteNum = o * 12 + bkSemitones[i];
                if (noteNum > 127)
                    continue;
                double centerY = noteToY(noteNum) + noteHeight / 2.0;
                int bkTop = static_cast<int>(centerY - bkH / 2.0);
                if (y >= bkTop && y < bkTop + bkH)
                    return noteNum;
            }
        }
    }

    static const int whiteKeySemitones[] = {0, 2, 4, 5, 7, 9, 11};
    for (int o = minOct; o <= maxOct; ++o)
    {
        double octBottom = static_cast<double>(noteToY(o * 12) + noteHeight);
        for (int wk = 0; wk < 7; ++wk)
        {
            int wkTop = static_cast<int>(octBottom - (wk + 1) * whiteKeyH + 0.5);
            int wkBottom = static_cast<int>(octBottom - wk * whiteKeyH + 0.5);
            if (y >= wkTop && y < wkBottom)
                return std::clamp(o * 12 + whiteKeySemitones[wk], 0, 127);
        }
    }

    return -1;
}

PianoRollComponent::ResizeEdge PianoRollComponent::edgeAt(int x, const MidiNote& note) const
{
    int leftX = tickToX(note.startTick);
    int rightX = tickToX(note.endTick());

    bool nearRight = std::abs(x - rightX) <= resizeEdgeWidth;
    bool nearLeft = std::abs(x - leftX) <= resizeEdgeWidth;

    if (nearRight)
        return ResizeEdge::Right;
    if (nearLeft && x < (leftX + rightX) / 2)
        return ResizeEdge::Left;
    return ResizeEdge::None;
}

int PianoRollComponent::getKeyboardLeft() const
{
    if (auto* vp = findParentComponentOfClass<juce::Viewport>())
        return vp->getViewPositionX();
    return 0;
}

int PianoRollComponent::getRulerTop() const
{
    if (auto* vp = findParentComponentOfClass<juce::Viewport>())
        return vp->getViewPositionY();
    return 0;
}

bool PianoRollComponent::isBlackKey(int noteNumber)
{
    int n = noteNumber % 12;
    return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
}

juce::String PianoRollComponent::getNoteName(int noteNumber)
{
    static const char* names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (noteNumber / 12) - 1;
    return juce::String(names[noteNumber % 12]) + juce::String(octave);
}

bool PianoRollComponent::isNoteSelected(const NoteRef& ref) const
{
    return selectedNotes.contains(ref);
}

std::vector<MidiNote> PianoRollComponent::collectSelectedNotes() const
{
    return selectedNotes | std::views::transform([this](const NoteRef& ref) { return noteAt(ref); }) |
           std::ranges::to<std::vector>();
}

void PianoRollComponent::clearNoteSelection()
{
    if (selectedNotes.empty() && !selectedNote.isValid())
        return;

    selectedNotes.clear();
    selectedNote = {};
    if (onNoteSelectionChanged)
        onNoteSelectionChanged(selectedNotes);
}

void PianoRollComponent::clearTempoSelection()
{
    tempoStrip.clearTempoSelection();
}

void PianoRollComponent::clearTimeSignatureSelection()
{
    timeSigStrip.clearTimeSignatureSelection();
}

void PianoRollComponent::clearKeySignatureSelection()
{
    keyStrip.clearKeySignatureSelection();
}

void PianoRollComponent::clearChordSelection()
{
    chordStrip.clearChordSelection();
}

void PianoRollComponent::drawRubberBand(juce::Graphics& g)
{
    using namespace calliope::theme;
    const auto* band = std::get_if<RubberBand>(&drag);
    if (band == nullptr || band->rect.isEmpty())
        return;

    g.setColour(accent::soft);
    g.fillRect(band->rect);
    g.setColour(accent::base.withAlpha(0.6f));
    g.drawRect(band->rect, 1);
}

std::vector<NoteRef> PianoRollComponent::findNotesInRect(const juce::Rectangle<int>& rect) const
{
    std::vector<NoteRef> result;
    if (!sequence || activeTrackIndex < 0 || activeTrackIndex >= sequence->getNumTracks())
        return result;

    for (const auto& [ref, note] : editableNotes().onTrack(activeTrackIndex))
    {
        const int width = ref.isRecorded() ? recordedNoteWidth(note.duration) : tickToWidth(note.duration);
        juce::Rectangle<int> noteRect(tickToX(note.startTick), noteToY(note.noteNumber), width, noteHeight);
        if (rect.intersects(noteRect))
            result.push_back(ref);
    }
    return result;
}

PianoRollComponent::EditMode PianoRollComponent::swapTool(EditMode mode)
{
    return mode == EditMode::Edit ? EditMode::Select : EditMode::Edit;
}

void PianoRollComponent::updateEffectiveEditMode()
{
    const auto desired = toolSwapActive ? swapTool(baseEditMode) : baseEditMode;
    if (desired == editMode)
        return;

    editMode = desired;
    repaint();
}

void PianoRollComponent::resetNoteDrag()
{
    if (!std::holds_alternative<KeyboardPreviewing>(drag))
        drag = Idle{};
}

void PianoRollComponent::toggleNoteSelection(const NoteRef& ref)
{
    if (selectedNotes.contains(ref))
        selectedNotes.erase(ref);
    else
        selectedNotes.insert(ref);
}

void PianoRollComponent::cancelEditDrag()
{
    if (std::holds_alternative<PendingResize>(drag) || std::holds_alternative<Moving>(drag) ||
        std::holds_alternative<Resizing>(drag) || std::holds_alternative<Creating>(drag))
        drag = Idle{};
}

void PianoRollComponent::pruneRecordedRefs()
{
    if (!sequence)
        return;
    const auto notes = editableNotes();
    const auto isGone = [&notes](const NoteRef& ref) { return ref.isRecorded() && !notes.find(ref).has_value(); };

    // A segment that recorded nothing ends without changing the sequence, so nothing else stops a drag of its notes.
    if (const auto* moving = std::get_if<Moving>(&drag);
        moving != nullptr && std::ranges::any_of(moving->targets, isGone, &MoveTarget::ref))
        drag = Idle{};
    if (const auto* resizing = std::get_if<Resizing>(&drag);
        resizing != nullptr && std::ranges::any_of(resizing->targets, isGone, &ResizeTarget::ref))
        drag = Idle{};
    if (const auto* pending = std::get_if<PendingResize>(&drag); pending != nullptr && isGone(pending->hit))
        drag = Idle{};

    if (isGone(selectedNote))
        selectedNote = {};
    if (std::erase_if(selectedNotes, isGone) > 0)
    {
        if (onNoteSelectionChanged)
            onNoteSelectionChanged(selectedNotes);
    }
}

void PianoRollComponent::beginResize(const NoteRef& hit, ResizeEdge edge)
{
    std::set<NoteRef> targets;
    if (selectedNotes.contains(hit))
        targets = selectedNotes;
    else
    {
        selectedNotes.clear();
        selectedNotes.insert(hit);
        targets.insert(hit);
        if (onNoteSelectionChanged)
            onNoteSelectionChanged(selectedNotes);
    }

    std::vector<ResizeTarget> resizeTargets;
    for (const auto& ref : targets)
    {
        const auto note = noteAt(ref);
        resizeTargets.push_back({ref, note.startTick, note.duration, note.startTick, note.duration});
    }

    const auto anchor = noteAt(hit);
    selectedNote = hit;
    drag = Resizing{std::move(resizeTargets), edge, anchor.startTick, anchor.endTick()};
    repaint();
}

void PianoRollComponent::beginMove(const NoteRef& anchor, const juce::MouseEvent& e)
{
    std::vector<MoveTarget> moveTargets;
    for (const auto& ref : selectedNotes)
    {
        const auto note = noteAt(ref);
        moveTargets.push_back({ref, note.startTick, note.noteNumber, note.duration});
    }

    const auto anchorNote = noteAt(anchor);
    selectedNote = anchor;
    drag = Moving{std::move(moveTargets), anchorNote.startTick, xToTick(e.x), yToNote(e.y)};
}

void PianoRollComponent::startNotePreview(const MidiNote& note)
{
    stopNotePreview();
    previewNote = note;
    isPreviewing = true;
    if (onNotePreview)
        onNotePreview(previewNote);
    repaint();
}

void PianoRollComponent::stopNotePreview()
{
    if (!isPreviewing)
        return;
    if (onNotePreviewEnd)
        onNotePreviewEnd(previewNote);
    isPreviewing = false;
    repaint();
}

void PianoRollComponent::timerCallback()
{
    stopTimer();
    stopNotePreview();
}
