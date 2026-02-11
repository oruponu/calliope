#include "PianoRollComponent.h"

void PianoRollComponent::setSequence(const MidiSequence* seq)
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
}

void PianoRollComponent::drawKeyboard(juce::Graphics& g)
{
    int blackKeyWidth = keyboardWidth * 55 / 100;

    g.setColour(juce::Colour(235, 235, 235));
    g.fillRect(0, 0, keyboardWidth, getHeight());

    // 白鍵の境界線（EとF、BとCの隣接箇所のみ）
    for (int note = 0; note < totalNotes; ++note)
    {
        int semitone = note % 12;
        if (semitone == 0 || semitone == 5)
        {
            int y = noteToY(note) + noteHeight;
            g.setColour(juce::Colour(180, 180, 180));
            g.drawHorizontalLine(y, 0.0f, static_cast<float>(keyboardWidth));
        }
    }

    for (int note = 0; note < totalNotes; ++note)
    {
        if (!isBlackKey(note))
            continue;

        int y = noteToY(note);

        g.setColour(juce::Colour(25, 25, 25));
        g.fillRect(0, y, blackKeyWidth, noteHeight);

        g.setColour(juce::Colour(60, 60, 60));
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(blackKeyWidth));
    }

    for (int note = 0; note < totalNotes; ++note)
    {
        if (note % 12 == 0)
        {
            int y = noteToY(note);
            g.setColour(juce::Colour(80, 80, 80));
            g.setFont(10.0f);
            g.drawText(getNoteName(note), blackKeyWidth + 3, y, keyboardWidth - blackKeyWidth - 5, noteHeight,
                       juce::Justification::centredLeft);
        }
    }

    g.setColour(juce::Colour(60, 60, 60));
    g.drawVerticalLine(keyboardWidth - 1, 0.0f, static_cast<float>(getHeight()));
}

void PianoRollComponent::drawGrid(juce::Graphics& g)
{
    int gridLeft = keyboardWidth;
    int gridWidth = getWidth() - keyboardWidth;
    int totalBeats = getTotalBeats();

    for (int note = 0; note < totalNotes; ++note)
    {
        int y = noteToY(note);
        bool black = isBlackKey(note);

        g.setColour(black ? juce::Colour(35, 35, 40) : juce::Colour(45, 45, 50));
        g.fillRect(gridLeft, y, gridWidth, noteHeight);
    }

    for (int note = 0; note < totalNotes; ++note)
    {
        int y = noteToY(note) + noteHeight;
        bool isC = (note % 12 == 0);

        g.setColour(isC ? juce::Colour(80, 80, 90) : juce::Colour(55, 55, 60));
        g.drawHorizontalLine(y, static_cast<float>(gridLeft), static_cast<float>(gridLeft + gridWidth));
    }

    for (int beat = 0; beat <= totalBeats; ++beat)
    {
        int x = tickToX(beat * sequence->getTicksPerQuarterNote());
        bool isBar = (beat % beatsPerBar == 0);

        g.setColour(isBar ? juce::Colour(90, 90, 100) : juce::Colour(55, 55, 60));
        g.drawVerticalLine(x, 0.0f, static_cast<float>(getHeight()));
    }
}

void PianoRollComponent::drawNotes(juce::Graphics& g)
{
    if (!sequence)
        return;

    for (int trackIdx = 0; trackIdx < sequence->getNumTracks(); ++trackIdx)
    {
        const auto& track = sequence->getTrack(trackIdx);

        for (int i = 0; i < track.getNumNotes(); ++i)
        {
            const auto& note = track.getNote(i);
            int x = tickToX(note.startTick);
            int y = noteToY(note.noteNumber);
            int w = tickToWidth(note.duration);

            g.setColour(juce::Colour(100, 160, 255));
            g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(w),
                                   static_cast<float>(noteHeight - 2), 2.0f);

            g.setColour(juce::Colour(70, 130, 220));
            g.drawRoundedRectangle(static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(w),
                                   static_cast<float>(noteHeight - 2), 2.0f, 1.0f);
        }
    }
}

void PianoRollComponent::drawPlayhead(juce::Graphics& g)
{
    int x = tickToX(playheadTick);
    g.setColour(juce::Colour(255, 80, 80));
    g.drawVerticalLine(x, 0.0f, static_cast<float>(getHeight()));
}

void PianoRollComponent::updateSize()
{
    if (!sequence)
        return;

    int totalBeats = getTotalBeats();
    int width = keyboardWidth + totalBeats * beatWidth;
    int height = totalNotes * noteHeight;
    setSize(width, height);
}

int PianoRollComponent::noteToY(int noteNumber) const
{
    return (totalNotes - 1 - noteNumber) * noteHeight;
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
