#include "PianoRollComponent.h"
#include <algorithm>

void PianoRollComponent::setSequence(MidiSequence* seq)
{
    sequence = seq;
    updateSize();
    repaint();
}

void PianoRollComponent::setPlayheadTick(int tick)
{
    playheadTick = tick;
    repaint();
}

void PianoRollComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(30, 30, 30));
    drawGrid(g);
    drawNotes(g);
    drawPlayhead(g);
    drawKeyboard(g);
    drawHeader(g);
}

void PianoRollComponent::mouseDown(const juce::MouseEvent& e)
{
    if (!sequence || sequence->getNumTracks() == 0)
        return;

    if (e.y < getHeaderTop() + headerHeight)
    {
        if (e.x >= getKeyboardLeft() + keyboardWidth)
        {
            int tick = snapTick(xToTick(e.x));
            playheadTick = std::max(0, tick);
            repaint();
            if (onPlayheadMoved)
                onPlayheadMoved(playheadTick);
        }
        return;
    }

    if (e.x < getKeyboardLeft() + keyboardWidth)
        return;

    auto hit = hitTestNote(e.x, e.y);

    if (e.mods.isRightButtonDown())
    {
        if (hit.isValid())
        {
            sequence->getTrack(hit.trackIndex).removeNote(hit.noteIndex);
            selectedNote = {};
            repaint();
        }
        return;
    }

    if (hit.isValid())
    {
        selectedNote = hit;
        const auto& note = sequence->getTrack(hit.trackIndex).getNote(hit.noteIndex);
        originalStartTick = note.startTick;
        originalNoteNumber = note.noteNumber;
        originalDuration = note.duration;
        dragStartTick = xToTick(e.x);
        dragStartNote = yToNote(e.y);

        dragMode = isOnRightEdge(e.x, note) ? DragMode::Resizing : DragMode::Moving;
    }
    else
    {
        int tick = snapTick(xToTick(e.x));
        int noteNum = yToNote(e.y);
        if (noteNum < 0 || noteNum > 127)
            return;

        auto& track = sequence->getTrack(0);
        track.addNote({noteNum, 100, tick, snapTicks});
        selectedNote = {0, track.getNumNotes() - 1};
        dragMode = DragMode::None;
        updateSize();
        repaint();
    }
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!sequence || !selectedNote.isValid() || dragMode == DragMode::None)
        return;

    auto& note = sequence->getTrack(selectedNote.trackIndex).getNote(selectedNote.noteIndex);

    if (dragMode == DragMode::Moving)
    {
        int currentTick = xToTick(e.x);
        int currentNote = yToNote(e.y);
        int deltaTick = currentTick - dragStartTick;
        int deltaNote = currentNote - dragStartNote;

        int newStart = snapTick(originalStartTick + deltaTick);
        int newNote = std::clamp(originalNoteNumber + deltaNote, 0, 127);

        note.startTick = std::max(0, newStart);
        note.noteNumber = newNote;
    }
    else if (dragMode == DragMode::Resizing)
    {
        int currentTick = xToTick(e.x);
        int newDuration = snapTick(currentTick - originalStartTick);
        note.duration = std::max(snapTicks, newDuration);
    }

    repaint();
}

void PianoRollComponent::mouseUp(const juce::MouseEvent&)
{
    if (dragMode != DragMode::None)
        updateSize();

    dragMode = DragMode::None;
}

void PianoRollComponent::mouseMove(const juce::MouseEvent& e)
{
    if (!sequence || e.x < getKeyboardLeft() + keyboardWidth || e.y < getHeaderTop() + headerHeight)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    auto hit = hitTestNote(e.x, e.y);
    if (hit.isValid())
    {
        const auto& note = sequence->getTrack(hit.trackIndex).getNote(hit.noteIndex);
        if (isOnRightEdge(e.x, note))
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        else
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void PianoRollComponent::drawKeyboard(juce::Graphics& g)
{
    int kbLeft = getKeyboardLeft();
    int blackKeyW = keyboardWidth * 55 / 100;

    auto clip = g.getClipBounds();

    g.setColour(juce::Colour(235, 235, 235));
    g.fillRect(kbLeft, clip.getY(), keyboardWidth, clip.getHeight());

    double whiteKeyH = 12.0 * noteHeight / 7.0;
    int minOctave = std::max(0, yToNote(clip.getBottom())) / 12;
    int maxOctave = std::min(totalNotes - 1, yToNote(clip.getY())) / 12;

    g.setColour(juce::Colour(180, 180, 180));
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

            g.setColour(juce::Colour(25, 25, 25));
            g.fillRect(kbLeft, bkTop, blackKeyW, bkH);
        }
    }

    g.setColour(juce::Colour(80, 80, 80));
    g.setFont(10.0f);
    for (int oct = minOctave; oct <= maxOctave; ++oct)
    {
        int base = oct * 12;
        double octBottom = static_cast<double>(noteToY(base) + noteHeight);
        int cTop = static_cast<int>(octBottom - whiteKeyH + 0.5);
        int cH = static_cast<int>(whiteKeyH);
        if (cTop + cH >= clip.getY() && cTop <= clip.getBottom())
            g.drawText(getNoteName(base), kbLeft, cTop, keyboardWidth - 4, cH, juce::Justification::centredRight);
    }

    g.setColour(juce::Colour(60, 60, 60));
    g.drawVerticalLine(kbLeft + keyboardWidth - 1, static_cast<float>(clip.getY()),
                       static_cast<float>(clip.getBottom()));
}

void PianoRollComponent::drawHeader(juce::Graphics& g)
{
    if (!sequence)
        return;

    int hTop = getHeaderTop();
    int kbLeft = getKeyboardLeft();

    auto clip = g.getClipBounds();
    int visibleLeft = clip.getX();
    int visibleRight = clip.getRight();

    g.setColour(juce::Colour(50, 50, 55));
    g.fillRect(kbLeft, hTop, getWidth() - kbLeft, headerHeight);

    int ppq = sequence->getTicksPerQuarterNote();
    int totalTicks = getTotalBeats() * ppq;
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

        for (int beat = 0; beat < beatsInBar && tick + beat * ticksPerBeat <= totalTicks; ++beat)
        {
            int x = tickToX(tick + beat * ticksPerBeat);
            if (x > visibleRight)
                break;
            if (x < visibleLeft - 30) // 小節番号のテキスト幅
                continue;

            bool isBar = (beat == 0);

            g.setColour(isBar ? juce::Colour(90, 90, 100) : juce::Colour(60, 60, 65));
            g.drawVerticalLine(x, static_cast<float>(hTop), static_cast<float>(hTop + headerHeight));

            if (isBar)
            {
                g.setColour(juce::Colour(180, 180, 190));
                g.setFont(11.0f);
                g.drawText(juce::String(barNumber), x + 4, hTop + 2, 30, headerHeight - 4,
                           juce::Justification::centredLeft);
            }
        }

        tick = barEndTick;
        barNumber++;
    }

    int phX = tickToX(playheadTick);
    if (phX >= visibleLeft && phX <= visibleRight)
    {
        g.setColour(juce::Colours::white);
        g.drawVerticalLine(phX, static_cast<float>(hTop), static_cast<float>(hTop + headerHeight));
    }

    g.setColour(juce::Colour(70, 70, 80));
    g.drawHorizontalLine(hTop + headerHeight - 1, static_cast<float>(kbLeft), static_cast<float>(getWidth()));
}

void PianoRollComponent::drawGrid(juce::Graphics& g)
{
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

        g.setColour(black ? juce::Colour(35, 35, 40) : juce::Colour(45, 45, 50));
        g.fillRect(gridLeft, y, gridRight - gridLeft, noteHeight);
    }

    for (int note = minNote; note <= maxNote; ++note)
    {
        int y = noteToY(note) + noteHeight;
        bool isC = (note % 12 == 0);

        g.setColour(isC ? juce::Colour(80, 80, 90) : juce::Colour(55, 55, 60));
        g.drawHorizontalLine(y, static_cast<float>(gridLeft), static_cast<float>(gridRight));
    }

    int ppq = sequence->getTicksPerQuarterNote();
    int totalTicks = getTotalBeats() * ppq;
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

        for (int beat = 0; beat < beatsInBar && tick + beat * ticksPerBeat <= totalTicks; ++beat)
        {
            int x = tickToX(tick + beat * ticksPerBeat);
            if (x > visibleRight)
                break;
            if (x < visibleLeft)
                continue;

            bool isBar = (beat == 0);

            g.setColour(isBar ? juce::Colour(90, 90, 100) : juce::Colour(55, 55, 60));
            g.drawVerticalLine(x, static_cast<float>(visibleTop), static_cast<float>(visibleBottom));
        }

        tick = barEndTick;
    }
}

void PianoRollComponent::drawNotes(juce::Graphics& g)
{
    if (!sequence)
        return;

    static const juce::Colour trackColours[] = {
        {100, 160, 255}, // ブルー
        {100, 210, 140}, // グリーン
        {255, 170, 80},  // オレンジ
        {180, 130, 255}, // パープル
        {255, 120, 160}, // ピンク
        {80, 200, 220},  // シアン
        {220, 200, 100}, // イエロー
        {200, 140, 120}, // ブラウン
    };
    static const int numTrackColours = sizeof(trackColours) / sizeof(trackColours[0]);

    auto clip = g.getClipBounds();

    for (int trackIdx = 0; trackIdx < sequence->getNumTracks(); ++trackIdx)
    {
        const auto& track = sequence->getTrack(trackIdx);

        for (int i = 0; i < track.getNumNotes(); ++i)
        {
            const auto& note = track.getNote(i);
            auto baseColour = trackColours[(note.channel - 1) % numTrackColours];
            int x = tickToX(note.startTick);
            int y = noteToY(note.noteNumber);
            int w = tickToWidth(note.duration);

            if (x + w < clip.getX() || x > clip.getRight() || y + noteHeight < clip.getY() || y > clip.getBottom())
                continue;

            bool isSelected = (selectedNote.trackIndex == trackIdx && selectedNote.noteIndex == i);

            if (note.channel == 10)
            {
                float diameter = static_cast<float>(noteHeight - 2);
                float cx = static_cast<float>(x);
                float cy = static_cast<float>(y + 1) + diameter * 0.5f;

                g.setColour(isSelected ? baseColour.brighter(0.4f) : baseColour);
                g.fillEllipse(cx - diameter * 0.5f, cy - diameter * 0.5f, diameter, diameter);

                g.setColour(isSelected ? baseColour.brighter(0.7f) : baseColour.darker(0.3f));
                g.drawEllipse(cx - diameter * 0.5f, cy - diameter * 0.5f, diameter, diameter, 1.0f);
            }
            else
            {
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

void PianoRollComponent::drawPlayhead(juce::Graphics& g)
{
    int x = tickToX(playheadTick);
    auto clip = g.getClipBounds();
    if (x < clip.getX() || x > clip.getRight())
        return;
    g.setColour(juce::Colours::white);
    g.drawVerticalLine(x, static_cast<float>(clip.getY()), static_cast<float>(clip.getBottom()));
}

void PianoRollComponent::updateSize()
{
    if (!sequence)
        return;

    int totalBeats = getTotalBeats();
    int width = keyboardWidth + totalBeats * beatWidth;
    int height = headerHeight + totalNotes * noteHeight;
    setSize(width, height);
}

int PianoRollComponent::noteToY(int noteNumber) const
{
    return headerHeight + (totalNotes - 1 - noteNumber) * noteHeight;
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
    return totalNotes - 1 - ((y - headerHeight) / noteHeight);
}

int PianoRollComponent::snapTick(int tick) const
{
    return ((tick + snapTicks / 2) / snapTicks) * snapTicks;
}

int PianoRollComponent::getTotalBeats() const
{
    if (!sequence || sequence->getNumTracks() == 0)
        return 16;

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

    int beats = lastTick / sequence->getTicksPerQuarterNote() + 4;
    return beats;
}

PianoRollComponent::NoteRef PianoRollComponent::hitTestNote(int x, int y) const
{
    if (!sequence)
        return {};

    for (int trackIdx = 0; trackIdx < sequence->getNumTracks(); ++trackIdx)
    {
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

bool PianoRollComponent::isOnRightEdge(int x, const MidiNote& note) const
{
    int rightX = tickToX(note.endTick());
    return std::abs(x - rightX) <= resizeEdgeWidth;
}

int PianoRollComponent::getKeyboardLeft() const
{
    if (auto* vp = findParentComponentOfClass<juce::Viewport>())
        return vp->getViewPositionX();
    return 0;
}

int PianoRollComponent::getHeaderTop() const
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
