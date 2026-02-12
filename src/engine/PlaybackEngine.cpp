#include "PlaybackEngine.h"
#include <algorithm>

PlaybackEngine::PlaybackEngine() = default;

PlaybackEngine::~PlaybackEngine()
{
    stop();
}

void PlaybackEngine::setSequence(const MidiSequence* seq)
{
    sequence = seq;
}

void PlaybackEngine::play()
{
    if (!sequence || playing)
        return;

    playing = true;
    lastCallbackTimeMs = juce::Time::getMillisecondCounterHiRes();
    startTimer(1);
}

void PlaybackEngine::stop()
{
    if (!playing)
        return;

    stopTimer();
    playing = false;
    sendAllNoteOffs();
}

bool PlaybackEngine::isPlaying() const
{
    return playing;
}

double PlaybackEngine::getCurrentTick() const
{
    return tickPosition;
}

void PlaybackEngine::setPositionInTicks(int tick)
{
    tickPosition = static_cast<double>(tick);
}

void PlaybackEngine::addListener(Listener* listener)
{
    listeners.push_back(listener);
}

void PlaybackEngine::removeListener(Listener* listener)
{
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

void PlaybackEngine::hiResTimerCallback()
{
    if (!playing || !sequence)
        return;

    double now = juce::Time::getMillisecondCounterHiRes();
    double elapsedMs = now - lastCallbackTimeMs;
    lastCallbackTimeMs = now;

    double ticksPerMs =
        (sequence->getTempoAt(static_cast<int>(tickPosition)) * sequence->getTicksPerQuarterNote()) / 60000.0;
    double elapsedTicks = elapsedMs * ticksPerMs;

    int previousTick = static_cast<int>(tickPosition);
    tickPosition += elapsedTicks;
    int currentTick = static_cast<int>(tickPosition);

    if (currentTick > previousTick)
    {
        processEvents(previousTick, currentTick);
        processNoteOns(previousTick, currentTick);
        processNoteOffs(currentTick);
    }
}

void PlaybackEngine::processNoteOns(int fromTick, int toTick)
{
    bool anySolo = sequence->isAnySolo();
    for (int trackIdx = 0; trackIdx < sequence->getNumTracks(); ++trackIdx)
    {
        const auto& track = sequence->getTrack(trackIdx);
        if (track.isMuted())
            continue;
        if (anySolo && !track.isSolo())
            continue;
        for (int noteIdx = 0; noteIdx < track.getNumNotes(); ++noteIdx)
        {
            const auto& note = track.getNote(noteIdx);
            if (note.startTick >= fromTick && note.startTick < toTick)
            {
                activeNotes.push_back({trackIdx, &note});
                for (auto* listener : listeners)
                    listener->onNoteOn(trackIdx, note);
            }
        }
    }
}

void PlaybackEngine::processEvents(int fromTick, int toTick)
{
    bool anySolo = sequence->isAnySolo();
    for (int trackIdx = 0; trackIdx < sequence->getNumTracks(); ++trackIdx)
    {
        const auto& track = sequence->getTrack(trackIdx);
        if (track.isMuted())
            continue;
        if (anySolo && !track.isSolo())
            continue;
        for (int eventIdx = 0; eventIdx < track.getNumEvents(); ++eventIdx)
        {
            const auto& event = track.getEvent(eventIdx);
            if (event.tick >= fromTick && event.tick < toTick)
            {
                for (auto* listener : listeners)
                    listener->onMidiEvent(trackIdx, event);
            }
        }
    }
}

void PlaybackEngine::processNoteOffs(int toTick)
{
    for (auto it = activeNotes.begin(); it != activeNotes.end();)
    {
        if (it->note->endTick() <= toTick)
        {
            for (auto* listener : listeners)
                listener->onNoteOff(it->trackIndex, *(it->note));
            it = activeNotes.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void PlaybackEngine::sendAllNoteOffs()
{
    for (const auto& active : activeNotes)
    {
        for (auto* listener : listeners)
            listener->onNoteOff(active.trackIndex, *(active.note));
    }
    activeNotes.clear();
}
