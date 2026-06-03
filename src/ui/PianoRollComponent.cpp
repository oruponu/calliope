#include "PianoRollComponent.h"
#include "../model/UndoActions.h"
#include "TrackColours.h"
#include <algorithm>
#include <vector>

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

void PianoRollComponent::moveSelectionToAdjacentNote(int direction)
{
    if (!sequence || activeTrackIndex < 0 || activeTrackIndex >= sequence->getNumTracks())
        return;

    auto& track = sequence->getTrack(activeTrackIndex);
    const int n = track.getNumNotes();
    if (n == 0)
        return;

    std::vector<int> order(n);
    for (int i = 0; i < n; ++i)
        order[i] = i;
    std::sort(order.begin(), order.end(),
              [&track](int a, int b)
              {
                  const auto& na = track.getNote(a);
                  const auto& nb = track.getNote(b);
                  if (na.startTick != nb.startTick)
                      return na.startTick < nb.startTick;
                  if (na.noteNumber != nb.noteNumber)
                      return na.noteNumber < nb.noteNumber;
                  return a < b;
              });

    auto posOf = [&order](int noteIndex)
    {
        for (int p = 0; p < static_cast<int>(order.size()); ++p)
            if (order[p] == noteIndex)
                return p;
        return -1;
    };

    int refIndex = -1;
    if (selectedNote.isValid() && selectedNote.trackIndex == activeTrackIndex && selectedNotes.contains(selectedNote))
    {
        refIndex = selectedNote.noteIndex;
    }
    else
    {
        int minPos = -1;
        int maxPos = -1;
        for (const auto& ref : selectedNotes)
        {
            if (ref.trackIndex != activeTrackIndex)
                continue;
            int p = posOf(ref.noteIndex);
            if (p < 0)
                continue;
            if (minPos < 0 || p < minPos)
                minPos = p;
            if (maxPos < 0 || p > maxPos)
                maxPos = p;
        }
        if (maxPos >= 0)
            refIndex = order[direction > 0 ? maxPos : minPos];
    }

    int targetPos;
    if (refIndex >= 0)
    {
        int np = posOf(refIndex) + (direction > 0 ? 1 : -1);
        if (np < 0 || np >= n)
            return;
        targetPos = np;
    }
    else
    {
        targetPos = direction > 0 ? 0 : n - 1;
    }

    NoteRef target{activeTrackIndex, order[targetPos]};
    selectedNotes.clear();
    selectedNotes.insert(target);
    selectedNote = target;

    const auto& note = track.getNote(target.noteIndex);
    startNotePreview(note);
    startTimer(previewHoldMs);

    if (onScrollToNote)
        onScrollToNote(note.startTick, note.noteNumber);

    repaint();
    if (onNoteSelectionChanged)
        onNoteSelectionChanged(selectedNotes);
}

void PianoRollComponent::nudgeSelectedNotesPitch(int deltaNote)
{
    if (!sequence || selectedNotes.empty() || deltaNote == 0)
        return;

    int minNote = 127;
    int maxNote = 0;
    for (const auto& ref : selectedNotes)
    {
        const auto& note = sequence->getTrack(ref.trackIndex).getNote(ref.noteIndex);
        minNote = std::min(minNote, note.noteNumber);
        maxNote = std::max(maxNote, note.noteNumber);
    }

    if (deltaNote > 0 && maxNote + deltaNote > 127)
        return;
    if (deltaNote < 0 && minNote + deltaNote < 0)
        return;

    if (undoManager)
        undoManager->beginNewTransaction(selectedNotes.size() > 1 ? "Move Notes" : "Move Note");

    for (const auto& ref : selectedNotes)
    {
        auto& note = sequence->getTrack(ref.trackIndex).getNote(ref.noteIndex);
        MidiNote beforeNote = note;
        MidiNote afterNote = note;
        afterNote.noteNumber = note.noteNumber + deltaNote;

        if (undoManager)
            undoManager->perform(new NoteModifyAction(sequence, ref.trackIndex, ref.noteIndex, beforeNote, afterNote));
        else
            note = afterNote;
    }

    NoteRef previewRef =
        (selectedNote.isValid() && selectedNotes.contains(selectedNote)) ? selectedNote : *selectedNotes.begin();
    startNotePreview(sequence->getTrack(previewRef.trackIndex).getNote(previewRef.noteIndex));
    startTimer(previewHoldMs);

    if (onNotesChanged)
        onNotesChanged();

    repaint();
}

void PianoRollComponent::setSequence(MidiSequence* seq)
{
    sequence = seq;

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
        contentBeats = std::max(contentBeats, lastTick / sequence->getTicksPerQuarterNote() + 4);
    }

    updateSize();
    repaint();
}

void PianoRollComponent::setUndoManager(juce::UndoManager* um)
{
    undoManager = um;
}

void PianoRollComponent::setSelectedTracks(int activeIndex, const std::set<int>& selectedIndices)
{
    activeTrackIndex = activeIndex;
    selectedTrackIndices = selectedIndices;
    selectedNote = {};
    selectedNotes.clear();
    dragMode = DragMode::None;
    repaint();
}

void PianoRollComponent::setSelectedNotes(const std::set<NoteRef>& notes)
{
    selectedNotes = notes;
    selectedNote = {};
    repaint();
}

void PianoRollComponent::setEditMode(EditMode mode)
{
    baseEditMode = mode;
    editMode = toolSwapActive ? swapTool(mode) : mode;
    dragMode = DragMode::None;
    repaint();
}

PianoRollComponent::EditMode PianoRollComponent::getEditMode() const
{
    return editMode;
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

void PianoRollComponent::modifierKeysChanged(const juce::ModifierKeys& modifiers)
{
    const bool modifierDown = modifiers.isCommandDown();
    if (modifierDown == toolSwapActive)
        return;

    toolSwapActive = modifierDown;

    if (dragMode != DragMode::None || isCreatingNote || isKeyboardDragging || isRulerDragging || isLoopDragging)
        return;

    updateEffectiveEditMode();
}

bool PianoRollComponent::isNoteSelected(const NoteRef& ref) const
{
    return selectedNotes.contains(ref);
}

std::vector<PianoRollComponent::NoteRef> PianoRollComponent::findNotesInRect(const juce::Rectangle<int>& rect) const
{
    std::vector<NoteRef> result;
    if (!sequence || activeTrackIndex < 0 || activeTrackIndex >= sequence->getNumTracks())
        return result;

    const auto& track = sequence->getTrack(activeTrackIndex);
    for (int i = 0; i < track.getNumNotes(); ++i)
    {
        const auto& note = track.getNote(i);
        int nx = tickToX(note.startTick);
        int ny = noteToY(note.noteNumber);
        int nw = tickToWidth(note.duration);
        juce::Rectangle<int> noteRect(nx, ny, nw, noteHeight);
        if (rect.intersects(noteRect))
            result.push_back({activeTrackIndex, i});
    }
    return result;
}

void PianoRollComponent::drawRubberBand(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (dragMode != DragMode::RubberBand || rubberBandRect.isEmpty())
        return;

    g.setColour(accent::soft);
    g.fillRect(rubberBandRect);
    g.setColour(accent::base.withAlpha(0.6f));
    g.drawRect(rubberBandRect, 1);
}

int PianoRollComponent::getActiveTrackIndex() const
{
    return activeTrackIndex;
}

void PianoRollComponent::copySelectedNotes()
{
    if (!sequence || selectedNotes.empty())
        return;

    clipboard.clear();

    int minTick = std::numeric_limits<int>::max();
    for (const auto& ref : selectedNotes)
    {
        const auto& note = sequence->getTrack(ref.trackIndex).getNote(ref.noteIndex);
        if (note.startTick < minTick)
            minTick = note.startTick;
    }

    for (const auto& ref : selectedNotes)
    {
        MidiNote note = sequence->getTrack(ref.trackIndex).getNote(ref.noteIndex);
        note.startTick -= minTick;
        clipboard.push_back(note);
    }
}

void PianoRollComponent::cutSelectedNotes()
{
    if (!sequence || selectedNotes.empty())
        return;

    copySelectedNotes();

    if (undoManager)
    {
        undoManager->beginNewTransaction("Cut Notes");
        undoManager->perform(new MultiNoteDeleteAction(sequence, selectedNotes));
    }
    else
    {
        std::map<int, std::vector<int>> toRemove;
        for (const auto& ref : selectedNotes)
            toRemove[ref.trackIndex].push_back(ref.noteIndex);

        for (auto& [trackIdx, indices] : toRemove)
        {
            std::sort(indices.begin(), indices.end(), std::greater<int>());
            for (int idx : indices)
                sequence->getTrack(trackIdx).removeNote(idx);
        }
    }

    selectedNotes.clear();
    repaint();
    if (onNotesChanged)
        onNotesChanged();
    if (onNoteSelectionChanged)
        onNoteSelectionChanged(selectedNotes);
}

void PianoRollComponent::selectAllNotes()
{
    if (!sequence || activeTrackIndex < 0 || activeTrackIndex >= sequence->getNumTracks())
        return;

    selectedNotes.clear();
    const auto& track = sequence->getTrack(activeTrackIndex);
    for (int i = 0; i < track.getNumNotes(); ++i)
        selectedNotes.insert({activeTrackIndex, i});

    repaint();
    if (onNoteSelectionChanged)
        onNoteSelectionChanged(selectedNotes);
}

bool PianoRollComponent::hasNotesInActiveTrack() const
{
    if (!sequence || activeTrackIndex < 0 || activeTrackIndex >= sequence->getNumTracks())
        return false;

    return sequence->getTrack(activeTrackIndex).getNumNotes() > 0;
}

void PianoRollComponent::pasteNotes(int atTick)
{
    if (!sequence || clipboard.empty() || activeTrackIndex < 0 || activeTrackIndex >= sequence->getNumTracks())
        return;

    std::vector<MidiNote> notesToAdd;
    for (const auto& note : clipboard)
    {
        MidiNote n = note;
        n.startTick += atTick;
        notesToAdd.push_back(n);
    }

    selectedNotes.clear();

    if (undoManager)
    {
        undoManager->beginNewTransaction("Paste Notes");
        auto* action = new MultiNoteAddAction(sequence, activeTrackIndex, notesToAdd);
        undoManager->perform(action);
        int start = action->getAddedStartIndex();
        for (int i = 0; i < action->getAddedCount(); ++i)
            selectedNotes.insert({activeTrackIndex, start + i});
    }
    else
    {
        auto& track = sequence->getTrack(activeTrackIndex);
        int start = track.getNumNotes();
        for (const auto& n : notesToAdd)
            track.addNote(n);
        for (int i = 0; i < static_cast<int>(notesToAdd.size()); ++i)
            selectedNotes.insert({activeTrackIndex, start + i});
    }

    repaint();
    if (onNotesChanged)
        onNotesChanged();
    if (onNoteSelectionChanged)
        onNoteSelectionChanged(selectedNotes);
}

void PianoRollComponent::setPlayheadTick(double tick)
{
    auto toX = [this](double t) -> int
    {
        if (!sequence)
            return keyboardWidth;
        return keyboardWidth + static_cast<int>(t / sequence->getTicksPerQuarterNote() * beatWidth);
    };

    int oldX = toX(playheadTick);
    playheadTick = tick;
    int newX = toX(playheadTick);

    int margin = 2;
    int h = getHeight();
    repaint(oldX - margin, 0, margin * 2 + 2, h);
    repaint(newX - margin, 0, margin * 2 + 2, h);
}

void PianoRollComponent::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    g.fillAll(surface::surface);
    drawGrid(g);
    drawLoopRegion(g);
    drawNotes(g);
    drawMoveGhosts(g);
    drawRubberBand(g);
    drawPlayhead(g);
    drawKeyboard(g);
    drawTempoTrack(g);
    drawTimeSignatureTrack(g);
    drawKeySignatureTrack(g);
    drawChordTrack(g);
    drawRuler(g);
    drawLoopBar(g);
}

void PianoRollComponent::mouseDown(const juce::MouseEvent& e)
{
    if (!sequence || sequence->getNumTracks() == 0)
        return;

    toolSwapActive = e.mods.isCommandDown();
    updateEffectiveEditMode();

    if (e.y < getRulerTop() + loopBarHeight)
    {
        if (e.x >= getKeyboardLeft() + keyboardWidth)
        {
            int tick = roundTickToGrid(xToTick(e.x));
            loopDragStartTick = std::max(0, tick);
            isLoopDragging = true;
        }
        return;
    }

    if (e.y < getRulerTop() + loopBarHeight + rulerHeight)
    {
        if (e.x >= getKeyboardLeft() + keyboardWidth)
        {
            int tick = roundTickToGrid(xToTick(e.x));
            playheadTick = std::max(0, tick);
            repaint();
            if (onPlayheadMoved)
                onPlayheadMoved(static_cast<int>(playheadTick));
            isRulerDragging = true;
            rulerDragStartY = e.y;
            lastRulerDragY = e.y;
        }
        return;
    }

    if (e.y < getRulerTop() + gridTopOffset)
        return;

    if (e.x < getKeyboardLeft() + keyboardWidth)
    {
        int noteNum = keyboardNoteAtPosition(e.x, e.y);
        if (noteNum >= 0)
        {
            startNotePreview(MidiNote{noteNum, 100, 0, 480});
            isKeyboardDragging = true;
        }
        return;
    }

    auto hit = hitTestNote(e.x, e.y);

    if (editMode == EditMode::Edit)
    {
        if (hit.isValid())
        {
            if (!e.mods.isRightButtonDown())
            {
                auto edge = edgeAt(e.x, sequence->getTrack(hit.trackIndex).getNote(hit.noteIndex));
                if (edge != ResizeEdge::None)
                {
                    beginResize(hit, edge);
                    return;
                }
            }

            if (undoManager)
            {
                undoManager->beginNewTransaction("Delete Note");
                undoManager->perform(new NoteDeleteAction(sequence, hit.trackIndex, hit.noteIndex));
            }
            else
            {
                sequence->getTrack(hit.trackIndex).removeNote(hit.noteIndex);
            }
            selectedNote = {};
            selectedNotes.clear();
            repaint();
            if (onNotesChanged)
                onNotesChanged();
            if (onNoteSelectionChanged)
                onNoteSelectionChanged(selectedNotes);
            return;
        }

        if (e.mods.isRightButtonDown())
            return;

        int tick = floorTickToGrid(xToTick(e.x));
        int noteNum = yToNote(e.y);
        if (noteNum < 0 || noteNum > 127)
            return;

        int defaultDuration = sequence ? sequence->getTicksPerQuarterNote() * 4 / quantizeDenominator : snapTicks;
        MidiNote newNote{noteNum, 100, tick, defaultDuration};

        if (undoManager)
        {
            undoManager->beginNewTransaction("Add Note");
            auto* action = new NoteAddAction(sequence, activeTrackIndex, newNote);
            undoManager->perform(action);
            selectedNote = {activeTrackIndex, action->getAddedIndex()};
        }
        else
        {
            auto& track = sequence->getTrack(activeTrackIndex);
            track.addNote(newNote);
            selectedNote = {activeTrackIndex, track.getNumNotes() - 1};
        }
        selectedNotes.clear();
        selectedNotes.insert(selectedNote);
        if (onNoteSelectionChanged)
            onNoteSelectionChanged(selectedNotes);
        startNotePreview(newNote);

        resizeTargets.clear();
        resizeTargets.push_back({selectedNote, newNote.startTick, newNote.duration});
        resizeAnchorStartTick = newNote.startTick;
        resizeAnchorEndTick = newNote.endTick();
        resizeEdge = ResizeEdge::Right;
        isCreatingNote = true;
        dragMode = DragMode::Resizing;

        repaint();
        if (onNotesChanged)
            onNotesChanged();
    }
    else // Select mode
    {
        if (e.mods.isRightButtonDown())
        {
            if (!selectedNotes.empty())
            {
                if (undoManager)
                {
                    undoManager->beginNewTransaction("Delete Notes");
                    undoManager->perform(new MultiNoteDeleteAction(sequence, selectedNotes));
                }
                else
                {
                    std::map<int, std::vector<int>> toRemove;
                    for (const auto& ref : selectedNotes)
                        toRemove[ref.trackIndex].push_back(ref.noteIndex);

                    for (auto& [trackIdx, indices] : toRemove)
                    {
                        std::sort(indices.begin(), indices.end(), std::greater<int>());
                        for (int idx : indices)
                            sequence->getTrack(trackIdx).removeNote(idx);
                    }
                }

                selectedNotes.clear();
                repaint();
                if (onNotesChanged)
                    onNotesChanged();
            }
            return;
        }

        if (hit.isValid())
        {
            if (!e.mods.isShiftDown())
            {
                auto edge = edgeAt(e.x, sequence->getTrack(hit.trackIndex).getNote(hit.noteIndex));
                if (edge != ResizeEdge::None)
                {
                    beginResize(hit, edge);
                    return;
                }
            }

            if (e.mods.isShiftDown())
            {
                if (selectedNotes.contains(hit))
                    selectedNotes.erase(hit);
                else
                    selectedNotes.insert(hit);
                dragMode = DragMode::None;
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
            const auto& note = sequence->getTrack(hit.trackIndex).getNote(hit.noteIndex);
            startNotePreview(note);
            repaint();
            if (onNoteSelectionChanged)
                onNoteSelectionChanged(selectedNotes);
        }
        else
        {
            if (!e.mods.isShiftDown())
                selectedNotes.clear();

            rubberBandStart = e.getPosition();
            rubberBandRect = {};
            dragMode = DragMode::RubberBand;
            repaint();
        }
    }
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!sequence)
        return;

    if (isKeyboardDragging)
    {
        int noteNum = keyboardNoteAtPosition(e.x, e.y);
        if (noteNum >= 0 && noteNum != previewNote.noteNumber)
            startNotePreview(MidiNote{noteNum, 100, 0, 480});
        return;
    }

    if (isLoopDragging)
    {
        int tick = roundTickToGrid(xToTick(e.x));
        tick = std::max(0, tick);
        int start = std::min(loopDragStartTick, tick);
        int end = std::max(loopDragStartTick, tick);
        if (end > start)
        {
            loopStartTick = start;
            loopEndTick = end;
            repaint();
            if (onLoopRegionChanged)
                onLoopRegionChanged(loopStartTick, loopEndTick);
        }
        return;
    }

    if (isRulerDragging)
    {
        if (std::abs(e.y - rulerDragStartY) < 5)
        {
            lastRulerDragY = e.y;
            return;
        }
        int deltaY = e.y - lastRulerDragY;
        lastRulerDragY = e.y;
        if (deltaY != 0 && onRulerDrag)
            onRulerDrag(e, deltaY);
        return;
    }

    if (dragMode == DragMode::RubberBand)
    {
        auto current = e.getPosition();
        int x = std::min(rubberBandStart.x, current.x);
        int y = std::min(rubberBandStart.y, current.y);
        int w = std::abs(current.x - rubberBandStart.x);
        int h = std::abs(current.y - rubberBandStart.y);
        rubberBandRect = {x, y, w, h};
        repaint();
        return;
    }

    if (dragMode == DragMode::Resizing)
    {
        if (resizeTargets.empty())
            return;

        int minDuration = sequence ? sequence->getTicksPerQuarterNote() * 4 / quantizeDenominator : snapTicks;
        int currentTick = roundTickToGrid(xToTick(e.x));

        if (resizeEdge == ResizeEdge::Right)
        {
            int delta = currentTick - resizeAnchorEndTick;
            for (const auto& t : resizeTargets)
            {
                auto& note = sequence->getTrack(t.ref.trackIndex).getNote(t.ref.noteIndex);
                note.startTick = t.startTick;
                note.duration = std::max(minDuration, t.duration + delta);
            }
        }
        else if (resizeEdge == ResizeEdge::Left)
        {
            int delta = currentTick - resizeAnchorStartTick;
            for (const auto& t : resizeTargets)
            {
                auto& note = sequence->getTrack(t.ref.trackIndex).getNote(t.ref.noteIndex);
                int endTick = t.startTick + t.duration;
                int newStart = std::clamp(t.startTick + delta, 0, endTick - minDuration);
                note.startTick = newStart;
                note.duration = endTick - newStart;
            }
        }

        repaint();
        return;
    }

    if (dragMode != DragMode::Moving || moveTargets.empty())
        return;

    int currentTick = xToTick(e.x);
    int currentNote = yToNote(e.y);
    int rawDeltaTick = currentTick - dragStartTick;
    int deltaNote = currentNote - dragStartNote;
    int snappedDeltaTick = roundTickToGrid(moveAnchorStartTick + rawDeltaTick) - moveAnchorStartTick;
    int minStart = moveTargets.front().startTick;
    int minNote = moveTargets.front().noteNumber;
    int maxNote = moveTargets.front().noteNumber;
    for (const auto& t : moveTargets)
    {
        minStart = std::min(minStart, t.startTick);
        minNote = std::min(minNote, t.noteNumber);
        maxNote = std::max(maxNote, t.noteNumber);
    }
    snappedDeltaTick = std::max(snappedDeltaTick, -minStart);
    deltaNote = std::clamp(deltaNote, -minNote, 127 - maxNote);

    moveDeltaTick = snappedDeltaTick;
    moveDeltaNote = deltaNote;

    const auto& anchorNote = sequence->getTrack(selectedNote.trackIndex).getNote(selectedNote.noteIndex);
    int previewPitch = anchorNote.noteNumber + moveDeltaNote;
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
    isKeyboardDragging = false;

    if (isLoopDragging)
    {
        isLoopDragging = false;
        return;
    }
    isRulerDragging = false;

    if (dragMode == DragMode::RubberBand)
    {
        if (!rubberBandRect.isEmpty())
        {
            auto found = findNotesInRect(rubberBandRect);
            for (const auto& ref : found)
                selectedNotes.insert(ref);
        }
        rubberBandRect = {};
        dragMode = DragMode::None;
        repaint();
        if (onNoteSelectionChanged)
            onNoteSelectionChanged(selectedNotes);
        return;
    }

    if (dragMode == DragMode::Resizing)
    {
        if (undoManager)
        {
            bool transactionStarted = false;
            for (const auto& t : resizeTargets)
            {
                auto& note = sequence->getTrack(t.ref.trackIndex).getNote(t.ref.noteIndex);
                if (note.startTick == t.startTick && note.duration == t.duration)
                    continue;

                MidiNote beforeNote{note.noteNumber, note.velocity, t.startTick, t.duration};
                MidiNote afterNote = note;

                if (!transactionStarted)
                {
                    if (!isCreatingNote)
                        undoManager->beginNewTransaction(resizeTargets.size() > 1 ? "Resize Notes" : "Resize Note");
                    transactionStarted = true;
                }
                undoManager->perform(
                    new NoteModifyAction(sequence, t.ref.trackIndex, t.ref.noteIndex, beforeNote, afterNote));
            }
        }
        resizeTargets.clear();
        resizeEdge = ResizeEdge::None;
        isCreatingNote = false;
        dragMode = DragMode::None;
        if (onNotesChanged)
            onNotesChanged();
        return;
    }

    if (dragMode == DragMode::Moving)
    {
        if (moveDeltaTick != 0 || moveDeltaNote != 0)
        {
            if (undoManager)
                undoManager->beginNewTransaction(moveTargets.size() > 1 ? "Move Notes" : "Move Note");

            for (const auto& t : moveTargets)
            {
                auto& note = sequence->getTrack(t.ref.trackIndex).getNote(t.ref.noteIndex);
                MidiNote beforeNote{t.noteNumber, note.velocity, t.startTick, note.duration};
                MidiNote afterNote = beforeNote;
                afterNote.startTick = t.startTick + moveDeltaTick;
                afterNote.noteNumber = t.noteNumber + moveDeltaNote;

                if (undoManager)
                    undoManager->perform(
                        new NoteModifyAction(sequence, t.ref.trackIndex, t.ref.noteIndex, beforeNote, afterNote));
                else
                    note = afterNote;
            }

            if (onNotesChanged)
                onNotesChanged();
        }

        moveTargets.clear();
        moveDeltaTick = 0;
        moveDeltaNote = 0;
    }

    dragMode = DragMode::None;
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
        auto edge = edgeAt(e.x, sequence->getTrack(hit.trackIndex).getNote(hit.noteIndex));
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

void PianoRollComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (e.y >= getRulerTop() + loopBarHeight && e.y < getRulerTop() + loopBarHeight + rulerHeight && onRulerWheel)
    {
        onRulerWheel(e, w);
        return;
    }
    Component::mouseWheelMove(e, w);
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

void PianoRollComponent::drawLoopBar(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (!sequence)
        return;

    int lbTop = getRulerTop();
    int kbLeft = getKeyboardLeft();

    g.setColour(surface::bg2);
    g.fillRect(kbLeft, lbTop, getWidth() - kbLeft, loopBarHeight);

    g.saveState();
    g.reduceClipRegion(kbLeft + keyboardWidth, 0, getWidth(), getHeight());

    if (loopEndTick > loopStartTick)
    {
        float lx1 = static_cast<float>(tickToX(loopStartTick));
        float lx2 = static_cast<float>(tickToX(loopEndTick));

        auto fillColour = loopEnabled ? accent::soft : surface::hover;
        auto edgeColour = loopEnabled ? accent::base : text::t4;

        g.setColour(fillColour);
        g.fillRoundedRectangle(lx1, static_cast<float>(lbTop + 2), lx2 - lx1 + 1.0f,
                               static_cast<float>(loopBarHeight - 4), 2.0f);

        g.setColour(edgeColour);
        g.fillRect(lx1, static_cast<float>(lbTop + 2), 2.0f, static_cast<float>(loopBarHeight - 4));
        g.fillRect(lx2 - 1.0f, static_cast<float>(lbTop + 2), 2.0f, static_cast<float>(loopBarHeight - 4));
    }

    float phX = static_cast<float>(keyboardWidth + playheadTick / sequence->getTicksPerQuarterNote() * beatWidth);
    if (phX >= static_cast<float>(kbLeft + keyboardWidth) && phX <= static_cast<float>(getWidth()))
    {
        g.setColour(text::t1);
        g.drawLine(phX, static_cast<float>(lbTop), phX, static_cast<float>(lbTop + loopBarHeight), 1.0f);
    }

    g.restoreState();

    g.setColour(border::normal);
    g.drawHorizontalLine(lbTop + loopBarHeight - 1, static_cast<float>(kbLeft), static_cast<float>(getWidth()));
}

void PianoRollComponent::drawRuler(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (!sequence)
        return;

    int hTop = getRulerTop() + loopBarHeight;
    int kbLeft = getKeyboardLeft();

    auto clip = g.getClipBounds();
    int visibleLeft = clip.getX();
    int visibleRight = clip.getRight();

    g.setColour(border::normal);
    g.fillRect(kbLeft, hTop, getWidth() - kbLeft, rulerHeight);

    g.saveState();
    g.reduceClipRegion(kbLeft + keyboardWidth, 0, getWidth(), getHeight());

    int ppq = sequence->getTicksPerQuarterNote();
    int quantizeGrid = ppq * 4 / quantizeDenominator;
    int totalTicks = xToTick(getWidth());
    int tick = 0;
    int barNumber = 1;

    while (tick < totalTicks)
    {
        auto ts = sequence->getTimeSignatureAt(tick);
        int ticksPerBeat = ppq * 4 / ts.denominator;
        int beatsInBar = ts.numerator;
        int barEndTick = tick + beatsInBar * ticksPerBeat;

        if (tickToX(tick) > visibleRight)
            break;

        if (tickToX(barEndTick) < visibleLeft)
        {
            tick = barEndTick;
            barNumber++;
            continue;
        }

        int subdivisionsPerBeat = std::max(1, ticksPerBeat / quantizeGrid);

        for (int beat = 0; beat < beatsInBar && tick + beat * ticksPerBeat <= totalTicks; ++beat)
        {
            int beatTick = tick + beat * ticksPerBeat;

            for (int sub = 0; sub < subdivisionsPerBeat; ++sub)
            {
                int subTick = beatTick + sub * quantizeGrid;
                int x = tickToX(subTick);
                if (x > visibleRight)
                    break;
                if (x < visibleLeft - 30)
                    continue;

                if (sub == 0)
                {
                    bool isBar = (beat == 0);
                    g.setColour(isBar ? border::strong : border::normal);
                    g.drawVerticalLine(x, static_cast<float>(hTop), static_cast<float>(hTop + rulerHeight));

                    if (isBar)
                    {
                        g.setColour(text::t2);
                        g.setFont(font::sans(font::sizeSM));
                        g.drawText(juce::String(barNumber), x + 4, hTop + 2, 30, rulerHeight - 4,
                                   juce::Justification::centredLeft);
                    }
                }
                else
                {
                    g.setColour(border::soft);
                    int tickH = rulerHeight / 3;
                    g.drawVerticalLine(x, static_cast<float>(hTop + rulerHeight - tickH),
                                       static_cast<float>(hTop + rulerHeight));
                }
            }
        }

        tick = barEndTick;
        barNumber++;
    }

    float phX = sequence
                    ? static_cast<float>(keyboardWidth + playheadTick / sequence->getTicksPerQuarterNote() * beatWidth)
                    : static_cast<float>(keyboardWidth);
    if (phX >= static_cast<float>(visibleLeft) - 1.0f && phX <= static_cast<float>(visibleRight) + 1.0f)
    {
        g.setColour(text::t1);
        g.drawLine(phX, static_cast<float>(hTop), phX, static_cast<float>(hTop + rulerHeight), 1.0f);
    }

    drawLoopOverlay(g, hTop, rulerHeight, 0.35f);

    g.restoreState();

    g.setColour(border::normal);
    g.drawVerticalLine(kbLeft + keyboardWidth - 1, static_cast<float>(hTop), static_cast<float>(hTop + rulerHeight));

    g.setColour(border::strong);
    g.drawHorizontalLine(hTop + rulerHeight - 1, static_cast<float>(kbLeft), static_cast<float>(getWidth()));
}

void PianoRollComponent::drawTempoTrack(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (!sequence)
        return;

    int hTop = getRulerTop();
    int tTop = hTop + loopBarHeight + rulerHeight;
    int kbLeft = getKeyboardLeft();

    auto clip = g.getClipBounds();
    int visibleLeft = clip.getX();
    int visibleRight = clip.getRight();

    g.setColour(surface::surface2);
    g.fillRect(kbLeft, tTop, getWidth() - kbLeft, tempoTrackHeight);

    g.saveState();
    g.reduceClipRegion(kbLeft + keyboardWidth, 0, getWidth(), getHeight());

    drawTrackGridLines(g, visibleLeft, visibleRight, static_cast<float>(tTop),
                       static_cast<float>(tTop + tempoTrackHeight));

    const auto& tempoChanges = sequence->getTempoChanges();
    if (tempoChanges.empty())
    {
        g.setColour(accent::base);
        g.setFont(font::sans(font::size2XS));
        g.drawText("120.00", kbLeft + keyboardWidth + 4, tTop, 60, tempoTrackHeight, juce::Justification::centredLeft);
    }
    else
    {
        double minBpm = tempoChanges[0].bpm;
        double maxBpm = tempoChanges[0].bpm;
        for (const auto& tc : tempoChanges)
        {
            minBpm = std::min(minBpm, tc.bpm);
            maxBpm = std::max(maxBpm, tc.bpm);
        }

        double range = maxBpm - minBpm;
        if (range < 1.0)
        {
            minBpm -= 10.0;
            maxBpm += 10.0;
            range = 20.0;
        }
        else
        {
            double pad = range * 0.15;
            minBpm -= pad;
            maxBpm += pad;
            range = maxBpm - minBpm;
        }

        int graphTop = tTop + 3;
        int graphBottom = tTop + tempoTrackHeight - 4;
        int graphHeight = graphBottom - graphTop;

        auto bpmToY = [&](double bpm) -> float
        {
            double normalized = (bpm - minBpm) / range;
            return static_cast<float>(graphBottom - normalized * graphHeight);
        };

        juce::Colour amberColour = accent::base;
        g.setColour(amberColour);

        int totalTicks = xToTick(getWidth());

        for (size_t i = 0; i < tempoChanges.size(); ++i)
        {
            int startX = tickToX(tempoChanges[i].tick);
            int endX = (i + 1 < tempoChanges.size()) ? tickToX(tempoChanges[i + 1].tick) : tickToX(totalTicks);
            float y = bpmToY(tempoChanges[i].bpm);

            int drawStartX = std::max(startX, visibleLeft);
            int drawEndX = std::min(endX, visibleRight);

            if (drawEndX < visibleLeft || drawStartX > visibleRight)
                continue;

            g.drawLine(static_cast<float>(drawStartX), y, static_cast<float>(drawEndX), y, 1.5f);

            if (i > 0 && startX >= visibleLeft && startX <= visibleRight)
            {
                float prevY = bpmToY(tempoChanges[i - 1].bpm);
                g.drawLine(static_cast<float>(startX), prevY, static_cast<float>(startX), y, 1.0f);
            }

            if (startX + 4 >= visibleLeft - 60 && startX <= visibleRight)
            {
                g.setFont(font::sans(font::size2XS));
                int textH = 12;
                float midY = static_cast<float>(tTop) + tempoTrackHeight * 0.5f;
                int textY;
                if (y > midY)
                    textY = tTop + 1;
                else
                    textY = tTop + tempoTrackHeight - textH - 2;
                g.drawText(juce::String(tempoChanges[i].bpm, 1), startX + 4, textY, 50, textH,
                           juce::Justification::centredLeft);
            }
        }
    }

    float phX = static_cast<float>(keyboardWidth + playheadTick / sequence->getTicksPerQuarterNote() * beatWidth);
    if (phX >= static_cast<float>(visibleLeft) - 1.0f && phX <= static_cast<float>(visibleRight) + 1.0f)
    {
        g.setColour(text::t1);
        g.drawLine(phX, static_cast<float>(tTop), phX, static_cast<float>(tTop + tempoTrackHeight), 1.0f);
    }

    drawLoopOverlay(g, tTop, tempoTrackHeight, 0.12f);

    g.restoreState();

    g.setColour(text::t3);
    g.setFont(font::sans(font::sizeXS));
    g.drawText("Tempo", kbLeft + 4, tTop, keyboardWidth - 8, tempoTrackHeight, juce::Justification::centredLeft);

    g.setColour(border::normal);
    g.drawVerticalLine(kbLeft + keyboardWidth - 1, static_cast<float>(tTop),
                       static_cast<float>(tTop + tempoTrackHeight));

    g.setColour(border::strong);
    g.drawHorizontalLine(tTop + tempoTrackHeight - 1, static_cast<float>(kbLeft), static_cast<float>(getWidth()));
}

void PianoRollComponent::drawTimeSignatureTrack(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (!sequence)
        return;

    int hTop = getRulerTop();
    int tsTop = hTop + loopBarHeight + rulerHeight + tempoTrackHeight;
    int kbLeft = getKeyboardLeft();

    auto clip = g.getClipBounds();
    int visibleLeft = clip.getX();
    int visibleRight = clip.getRight();

    g.setColour(surface::surface2);
    g.fillRect(kbLeft, tsTop, getWidth() - kbLeft, timeSignatureTrackHeight);

    g.saveState();
    g.reduceClipRegion(kbLeft + keyboardWidth, 0, getWidth(), getHeight());

    drawTrackGridLines(g, visibleLeft, visibleRight, static_cast<float>(tsTop),
                       static_cast<float>(tsTop + timeSignatureTrackHeight));

    const auto& tsChanges = sequence->getTimeSignatureChanges();
    if (tsChanges.empty())
    {
        g.setColour(track::teal);
        g.setFont(font::sans(font::sizeSM));
        g.drawText("4/4", kbLeft + keyboardWidth + 4, tsTop, 40, timeSignatureTrackHeight,
                   juce::Justification::centredLeft);
    }
    else
    {
        juce::Colour tsColour = track::teal;

        for (size_t i = 0; i < tsChanges.size(); ++i)
        {
            int x = tickToX(tsChanges[i].tick);

            if (x > visibleRight)
                break;

            int nextX = (i + 1 < tsChanges.size()) ? tickToX(tsChanges[i + 1].tick) : tickToX(xToTick(getWidth()));
            if (nextX < visibleLeft)
                continue;

            if (i > 0 && x >= visibleLeft && x <= visibleRight)
            {
                g.setColour(tsColour.withAlpha(0.6f));
                g.drawVerticalLine(x, static_cast<float>(tsTop + 2),
                                   static_cast<float>(tsTop + timeSignatureTrackHeight - 2));
            }

            if (x + 4 >= visibleLeft - 40 && x <= visibleRight)
            {
                g.setColour(tsColour);
                g.setFont(font::sans(font::sizeSM));
                juce::String label =
                    juce::String(tsChanges[i].numerator) + "/" + juce::String(tsChanges[i].denominator);
                int textX = (i == 0 && tsChanges[i].tick == 0) ? kbLeft + keyboardWidth + 4 : x + 4;
                g.drawText(label, textX, tsTop, 40, timeSignatureTrackHeight, juce::Justification::centredLeft);
            }
        }
    }

    float phX = static_cast<float>(keyboardWidth + playheadTick / sequence->getTicksPerQuarterNote() * beatWidth);
    if (phX >= static_cast<float>(visibleLeft) - 1.0f && phX <= static_cast<float>(visibleRight) + 1.0f)
    {
        g.setColour(text::t1);
        g.drawLine(phX, static_cast<float>(tsTop), phX, static_cast<float>(tsTop + timeSignatureTrackHeight), 1.0f);
    }

    drawLoopOverlay(g, tsTop, timeSignatureTrackHeight, 0.12f);

    g.restoreState();

    g.setColour(text::t3);
    g.setFont(font::sans(font::sizeXS));
    g.drawText("Time Sig", kbLeft + 4, tsTop, keyboardWidth - 8, timeSignatureTrackHeight,
               juce::Justification::centredLeft);

    g.setColour(border::normal);
    g.drawVerticalLine(kbLeft + keyboardWidth - 1, static_cast<float>(tsTop),
                       static_cast<float>(tsTop + timeSignatureTrackHeight));

    g.setColour(border::strong);
    g.drawHorizontalLine(tsTop + timeSignatureTrackHeight - 1, static_cast<float>(kbLeft),
                         static_cast<float>(getWidth()));
}

void PianoRollComponent::drawKeySignatureTrack(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (!sequence)
        return;

    int hTop = getRulerTop();
    int ksTop = hTop + loopBarHeight + rulerHeight + tempoTrackHeight + timeSignatureTrackHeight;
    int kbLeft = getKeyboardLeft();

    auto clip = g.getClipBounds();
    int visibleLeft = clip.getX();
    int visibleRight = clip.getRight();

    g.setColour(surface::surface2);
    g.fillRect(kbLeft, ksTop, getWidth() - kbLeft, keySignatureTrackHeight);

    g.saveState();
    g.reduceClipRegion(kbLeft + keyboardWidth, 0, getWidth(), getHeight());

    drawTrackGridLines(g, visibleLeft, visibleRight, static_cast<float>(ksTop),
                       static_cast<float>(ksTop + keySignatureTrackHeight));

    const auto& ksChanges = sequence->getKeySignatureChanges();
    if (ksChanges.empty())
    {
        g.setColour(track::sand);
        g.setFont(font::sans(font::sizeSM));
        g.drawText("C", kbLeft + keyboardWidth + 4, ksTop, 40, keySignatureTrackHeight,
                   juce::Justification::centredLeft);
    }
    else
    {
        juce::Colour ksColour = track::sand;

        for (size_t i = 0; i < ksChanges.size(); ++i)
        {
            int x = tickToX(ksChanges[i].tick);

            if (x > visibleRight)
                break;

            int nextX = (i + 1 < ksChanges.size()) ? tickToX(ksChanges[i + 1].tick) : tickToX(xToTick(getWidth()));
            if (nextX < visibleLeft)
                continue;

            if (i > 0 && x >= visibleLeft && x <= visibleRight)
            {
                g.setColour(ksColour.withAlpha(0.6f));
                g.drawVerticalLine(x, static_cast<float>(ksTop + 2),
                                   static_cast<float>(ksTop + keySignatureTrackHeight - 2));
            }

            if (x + 4 >= visibleLeft - 40 && x <= visibleRight)
            {
                g.setColour(ksColour);
                g.setFont(font::sans(font::sizeSM));
                juce::String label =
                    juce::String(MidiSequence::keySignatureToString(ksChanges[i].sharpsOrFlats, ksChanges[i].isMinor));
                int textX = (i == 0 && ksChanges[i].tick == 0) ? kbLeft + keyboardWidth + 4 : x + 4;
                g.drawText(label, textX, ksTop, 60, keySignatureTrackHeight, juce::Justification::centredLeft);
            }
        }
    }

    float phX = static_cast<float>(keyboardWidth + playheadTick / sequence->getTicksPerQuarterNote() * beatWidth);
    if (phX >= static_cast<float>(visibleLeft) - 1.0f && phX <= static_cast<float>(visibleRight) + 1.0f)
    {
        g.setColour(text::t1);
        g.drawLine(phX, static_cast<float>(ksTop), phX, static_cast<float>(ksTop + keySignatureTrackHeight), 1.0f);
    }

    drawLoopOverlay(g, ksTop, keySignatureTrackHeight, 0.12f);

    g.restoreState();

    g.setColour(text::t3);
    g.setFont(font::sans(font::sizeXS));
    g.drawText("Key", kbLeft + 4, ksTop, keyboardWidth - 8, keySignatureTrackHeight, juce::Justification::centredLeft);

    g.setColour(border::normal);
    g.drawVerticalLine(kbLeft + keyboardWidth - 1, static_cast<float>(ksTop),
                       static_cast<float>(ksTop + keySignatureTrackHeight));

    g.setColour(border::strong);
    g.drawHorizontalLine(ksTop + keySignatureTrackHeight - 1, static_cast<float>(kbLeft),
                         static_cast<float>(getWidth()));
}

void PianoRollComponent::drawChordTrack(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (!sequence)
        return;

    int hTop = getRulerTop();
    int ctTop =
        hTop + loopBarHeight + rulerHeight + tempoTrackHeight + timeSignatureTrackHeight + keySignatureTrackHeight;
    int kbLeft = getKeyboardLeft();

    auto clip = g.getClipBounds();
    int visibleLeft = clip.getX();
    int visibleRight = clip.getRight();

    g.setColour(surface::surface2);
    g.fillRect(kbLeft, ctTop, getWidth() - kbLeft, chordTrackHeight);

    g.saveState();
    g.reduceClipRegion(kbLeft + keyboardWidth, 0, getWidth(), getHeight());

    drawTrackGridLines(g, visibleLeft, visibleRight, static_cast<float>(ctTop),
                       static_cast<float>(ctTop + chordTrackHeight));

    const auto& chordChanges = sequence->getChordChanges();
    if (!chordChanges.empty())
    {
        juce::Colour chordColour = track::violet;
        int barY = ctTop + 3;
        int barH = chordTrackHeight - 6;

        for (size_t i = 0; i < chordChanges.size(); ++i)
        {
            auto label = MidiSequence::chordToString(chordChanges[i]);
            if (label.empty())
                continue;

            int x = tickToX(chordChanges[i].tick);
            int nextX =
                (i + 1 < chordChanges.size()) ? tickToX(chordChanges[i + 1].tick) : tickToX(xToTick(getWidth()));

            if (x > visibleRight || nextX < visibleLeft)
                continue;

            g.setColour(chordColour.withAlpha(0.15f));
            g.fillRect(x, barY, nextX - x, barH);
            g.setColour(chordColour.withAlpha(0.5f));
            g.drawRect(x, barY, nextX - x, barH, 1);

            int textX = x + 4;
            int textWidth = nextX - textX - 2;
            if (textWidth > 8)
            {
                g.setColour(chordColour);
                g.setFont(font::sans(font::sizeSM));
                g.drawText(juce::String(label), textX, ctTop, textWidth, chordTrackHeight,
                           juce::Justification::centredLeft);
            }
        }
    }

    float phX = static_cast<float>(keyboardWidth + playheadTick / sequence->getTicksPerQuarterNote() * beatWidth);
    if (phX >= static_cast<float>(visibleLeft) - 1.0f && phX <= static_cast<float>(visibleRight) + 1.0f)
    {
        g.setColour(text::t1);
        g.drawLine(phX, static_cast<float>(ctTop), phX, static_cast<float>(ctTop + chordTrackHeight), 1.0f);
    }

    drawLoopOverlay(g, ctTop, chordTrackHeight, 0.12f);

    g.restoreState();

    g.setColour(text::t3);
    g.setFont(font::sans(font::sizeXS));
    g.drawText("Chord", kbLeft + 4, ctTop, keyboardWidth - 8, chordTrackHeight, juce::Justification::centredLeft);

    g.setColour(border::normal);
    g.drawVerticalLine(kbLeft + keyboardWidth - 1, static_cast<float>(ctTop),
                       static_cast<float>(ctTop + chordTrackHeight));

    g.setColour(border::strong);
    g.drawHorizontalLine(ctTop + chordTrackHeight - 1, static_cast<float>(kbLeft), static_cast<float>(getWidth()));
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

    int ppq = sequence->getTicksPerQuarterNote();
    int quantizeGrid = ppq * 4 / quantizeDenominator;
    int totalTicks = xToTick(getWidth());
    int tick = 0;

    while (tick < totalTicks)
    {
        auto ts = sequence->getTimeSignatureAt(tick);
        int ticksPerBeat = ppq * 4 / ts.denominator;
        int beatsInBar = ts.numerator;
        int barEndTick = tick + beatsInBar * ticksPerBeat;

        if (tickToX(tick) > visibleRight)
            break;

        if (tickToX(barEndTick) < visibleLeft)
        {
            tick = barEndTick;
            continue;
        }

        int subdivisionsPerBeat = std::max(1, ticksPerBeat / quantizeGrid);

        for (int beat = 0; beat < beatsInBar && tick + beat * ticksPerBeat <= totalTicks; ++beat)
        {
            int beatTick = tick + beat * ticksPerBeat;

            for (int sub = 0; sub < subdivisionsPerBeat; ++sub)
            {
                int subTick = beatTick + sub * quantizeGrid;
                int x = tickToX(subTick);
                if (x > visibleRight)
                    break;
                if (x < visibleLeft)
                    continue;

                if (sub == 0)
                {
                    bool isBar = (beat == 0);
                    g.setColour(isBar ? border::strong : border::normal);
                }
                else
                {
                    g.setColour(border::soft);
                }
                g.drawVerticalLine(x, static_cast<float>(visibleTop), static_cast<float>(visibleBottom));
            }
        }

        tick = barEndTick;
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
            const auto& note = track.getNote(i);
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

        for (int i = 0; i < track.getNumNotes(); ++i)
        {
            const auto& note = track.getNote(i);
            auto baseColour = TrackColours::getColour(activeTrackIndex);
            int x = tickToX(note.startTick);
            int y = noteToY(note.noteNumber);
            int w = tickToWidth(note.duration);

            if (y + noteHeight < clip.getY() || y > clip.getBottom())
                continue;

            bool isSelected = isNoteSelected({activeTrackIndex, i});

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

                g.setColour(isSelected ? baseColour.brighter(0.7f) : baseColour.darker(0.3f));
                g.drawEllipse(left, cy - diameter * 0.5f, diameter, diameter, 1.0f);
            }
            else
            {
                if (x + w < clip.getX() || x > clip.getRight())
                    continue;

                g.setColour(isSelected ? baseColour.brighter(0.4f) : baseColour);
                g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(w),
                                       static_cast<float>(noteHeight - 2), 2.0f);

                g.setColour(isSelected ? baseColour.brighter(0.7f) : baseColour.darker(0.3f));
                g.drawRoundedRectangle(static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(w),
                                       static_cast<float>(noteHeight - 2), 2.0f, 1.0f);
            }
        }
    }
}

void PianoRollComponent::drawMoveGhosts(juce::Graphics& g)
{
    if (!sequence || dragMode != DragMode::Moving || moveTargets.empty())
        return;
    if (moveDeltaTick == 0 && moveDeltaNote == 0)
        return;

    auto clip = g.getClipBounds();

    for (const auto& t : moveTargets)
    {
        if (t.ref.trackIndex < 0 || t.ref.trackIndex >= sequence->getNumTracks())
            continue;

        const auto& track = sequence->getTrack(t.ref.trackIndex);
        const auto& note = track.getNote(t.ref.noteIndex);

        int x = tickToX(t.startTick + moveDeltaTick);
        int y = noteToY(t.noteNumber + moveDeltaNote);
        int w = tickToWidth(note.duration);

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
    float x = static_cast<float>(keyboardWidth + playheadTick / sequence->getTicksPerQuarterNote() * beatWidth);
    auto clip = g.getClipBounds();
    if (x < static_cast<float>(clip.getX()) - 1.0f || x > static_cast<float>(clip.getRight()) + 1.0f)
        return;
    g.setColour(text::t1);
    g.drawLine(x, static_cast<float>(clip.getY()), x, static_cast<float>(clip.getBottom()), 1.0f);
}

void PianoRollComponent::setLoopRegion(bool enabled, int startTick, int endTick)
{
    loopEnabled = enabled;
    loopStartTick = startTick;
    loopEndTick = endTick;
    repaint();
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

void PianoRollComponent::drawLoopOverlay(juce::Graphics& g, int top, int height, float fillAlpha)
{
    using namespace calliope::theme;
    if (loopEndTick <= loopStartTick)
        return;

    float lx1 = static_cast<float>(tickToX(loopStartTick));
    float lx2 = static_cast<float>(tickToX(loopEndTick));
    float fTop = static_cast<float>(top);
    float fBottom = static_cast<float>(top + height);

    g.setColour(loopEnabled ? accent::base.withAlpha(fillAlpha) : surface::hover);
    g.fillRect(lx1, fTop, lx2 - lx1, static_cast<float>(height));

    g.setColour(loopEnabled ? accent::base.withAlpha(0.7f) : text::t4);
    g.drawVerticalLine(static_cast<int>(lx1), fTop, fBottom);
    g.drawVerticalLine(static_cast<int>(lx2), fTop, fBottom);
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

int PianoRollComponent::noteToY(int noteNumber) const
{
    return gridTopOffset + (totalNotes - 1 - noteNumber) * noteHeight;
}

int PianoRollComponent::tickToX(int tick) const
{
    if (!sequence)
        return keyboardWidth;

    double beatsFromTick = static_cast<double>(tick) / sequence->getTicksPerQuarterNote();
    return keyboardWidth + static_cast<int>(beatsFromTick * beatWidth);
}

int PianoRollComponent::tickToWidth(int durationTicks) const
{
    if (!sequence)
        return 0;

    double beats = static_cast<double>(durationTicks) / sequence->getTicksPerQuarterNote();
    return static_cast<int>(beats * beatWidth);
}

int PianoRollComponent::xToTick(int x) const
{
    if (!sequence)
        return 0;

    double beats = static_cast<double>(x - keyboardWidth) / beatWidth;
    return static_cast<int>(beats * sequence->getTicksPerQuarterNote());
}

int PianoRollComponent::yToNote(int y) const
{
    return totalNotes - 1 - ((y - gridTopOffset) / noteHeight);
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

int PianoRollComponent::roundTickToGrid(int tick) const
{
    if (!sequence)
        return ((tick + snapTicks / 2) / snapTicks) * snapTicks;

    int ppq = sequence->getTicksPerQuarterNote();
    int grid = ppq * 4 / quantizeDenominator;
    return ((tick + grid / 2) / grid) * grid;
}

int PianoRollComponent::floorTickToGrid(int tick) const
{
    if (!sequence)
        return (tick / snapTicks) * snapTicks;

    int ppq = sequence->getTicksPerQuarterNote();
    int grid = ppq * 4 / quantizeDenominator;
    return (tick / grid) * grid;
}

void PianoRollComponent::setQuantizeDenominator(int denom)
{
    quantizeDenominator = denom;
    repaint();
}

void PianoRollComponent::drawTrackGridLines(juce::Graphics& g, int visibleLeft, int visibleRight, float top,
                                            float bottom)
{
    using namespace calliope::theme;
    int ppq = sequence->getTicksPerQuarterNote();
    int quantizeGrid = ppq * 4 / quantizeDenominator;
    int totalTicks = xToTick(getWidth());
    int tick = 0;

    while (tick < totalTicks)
    {
        auto ts = sequence->getTimeSignatureAt(tick);
        int ticksPerBeat = ppq * 4 / ts.denominator;
        int beatsInBar = ts.numerator;
        int barEndTick = tick + beatsInBar * ticksPerBeat;

        if (tickToX(tick) > visibleRight)
            break;

        if (tickToX(barEndTick) < visibleLeft)
        {
            tick = barEndTick;
            continue;
        }

        int subdivisionsPerBeat = std::max(1, ticksPerBeat / quantizeGrid);

        for (int beat = 0; beat < beatsInBar && tick + beat * ticksPerBeat <= totalTicks; ++beat)
        {
            int beatTick = tick + beat * ticksPerBeat;

            for (int sub = 0; sub < subdivisionsPerBeat; ++sub)
            {
                int subTick = beatTick + sub * quantizeGrid;
                int x = tickToX(subTick);
                if (x > visibleRight)
                    break;
                if (x < visibleLeft)
                    continue;

                if (sub == 0)
                {
                    bool isBar = (beat == 0);
                    g.setColour(isBar ? border::strong : border::normal);
                }
                else
                {
                    g.setColour(border::soft);
                }
                g.drawVerticalLine(x, top, bottom);
            }
        }

        tick = barEndTick;
    }
}

void PianoRollComponent::setBeatWidth(int w)
{
    beatWidth = juce::jlimit(minBeatWidth, maxBeatWidth, w);
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

void PianoRollComponent::extendContent()
{
    contentBeats += 32;
    updateSize();
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

    resizeTargets.clear();
    for (const auto& ref : targets)
    {
        const auto& note = sequence->getTrack(ref.trackIndex).getNote(ref.noteIndex);
        resizeTargets.push_back({ref, note.startTick, note.duration});
    }

    const auto& anchor = sequence->getTrack(hit.trackIndex).getNote(hit.noteIndex);
    resizeAnchorStartTick = anchor.startTick;
    resizeAnchorEndTick = anchor.endTick();
    resizeEdge = edge;
    selectedNote = hit;
    dragMode = DragMode::Resizing;
    repaint();
}

void PianoRollComponent::beginMove(const NoteRef& anchor, const juce::MouseEvent& e)
{
    moveTargets.clear();
    for (const auto& ref : selectedNotes)
    {
        const auto& note = sequence->getTrack(ref.trackIndex).getNote(ref.noteIndex);
        moveTargets.push_back({ref, note.startTick, note.noteNumber});
    }

    const auto& anchorNote = sequence->getTrack(anchor.trackIndex).getNote(anchor.noteIndex);
    moveAnchorStartTick = anchorNote.startTick;
    moveDeltaTick = 0;
    moveDeltaNote = 0;
    dragStartTick = xToTick(e.x);
    dragStartNote = yToNote(e.y);
    selectedNote = anchor;
    dragMode = DragMode::Moving;
}

PianoRollComponent::NoteRef PianoRollComponent::hitTestNote(int x, int y) const
{
    if (!sequence)
        return {};

    if (activeTrackIndex >= 0 && activeTrackIndex < sequence->getNumTracks() &&
        selectedTrackIndices.contains(activeTrackIndex))
    {
        const auto& track = sequence->getTrack(activeTrackIndex);
        for (int i = 0; i < track.getNumNotes(); ++i)
        {
            const auto& note = track.getNote(i);
            int nx = tickToX(note.startTick);
            int ny = noteToY(note.noteNumber);
            int nw = tickToWidth(note.duration);

            if (x >= nx && x <= nx + nw && y >= ny && y < ny + noteHeight)
                return {activeTrackIndex, i};
        }
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
