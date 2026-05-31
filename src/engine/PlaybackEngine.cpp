#include "PlaybackEngine.h"

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
    sendAllNoteOffs();
    tickPosition = static_cast<double>(tick);
}

void PlaybackEngine::setLoopEnabled(bool enabled)
{
    loopEnabled = enabled;
}

bool PlaybackEngine::isLoopEnabled() const
{
    return loopEnabled;
}

void PlaybackEngine::setLoopRange(int startTick, int endTick)
{
    loopStartTick = startTick;
    loopEndTick = endTick;
}

int PlaybackEngine::getLoopStartTick() const
{
    return loopStartTick;
}

int PlaybackEngine::getLoopEndTick() const
{
    return loopEndTick;
}

void PlaybackEngine::addListener(Listener* listener)
{
    listeners.push_back(listener);
}

void PlaybackEngine::removeListener(Listener* listener)
{
    std::erase(listeners, listener);
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

    if (loopEnabled && loopEndTick > loopStartTick && tickPosition >= loopEndTick)
    {
        processEvents(previousTick, loopEndTick);
        processNoteOns(previousTick, loopEndTick);
        processNoteOffs(loopEndTick);
        sendAllNoteOffs();
        tickPosition = static_cast<double>(loopStartTick) + (tickPosition - loopEndTick);
        return;
    }

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
    std::erase_if(activeNotes,
                  [&](const ActiveNote& active)
                  {
                      if (active.note->endTick() > toTick)
                          return false;
                      for (auto* listener : listeners)
                          listener->onNoteOff(active.trackIndex, *(active.note));
                      return true;
                  });
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

void PlaybackEngine::releaseActiveNotesForTrack(int trackIndex)
{
    std::erase_if(activeNotes,
                  [&](const ActiveNote& active)
                  {
                      if (active.trackIndex != trackIndex)
                          return false;
                      for (auto* listener : listeners)
                          listener->onNoteOff(active.trackIndex, *(active.note));
                      return true;
                  });
}
