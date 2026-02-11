#pragma once

#include "../model/MidiSequence.h"
#include <juce_events/juce_events.h>
#include <vector>

class PlaybackEngine : private juce::HighResolutionTimer
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void onNoteOn(int trackIndex, const MidiNote& note) = 0;
        virtual void onNoteOff(int trackIndex, const MidiNote& note) = 0;
    };

    PlaybackEngine();
    ~PlaybackEngine() override;

    void setSequence(const MidiSequence* seq);

    void play();
    void stop();
    bool isPlaying() const;

    int getCurrentTick() const;
    void setPositionInTicks(int tick);

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    void hiResTimerCallback() override;
    void processNoteOns(int fromTick, int toTick);
    void processNoteOffs(int toTick);
    void sendAllNoteOffs();

    const MidiSequence* sequence = nullptr;
    std::atomic<bool> playing{false};
    double tickPosition = 0.0;
    double lastCallbackTimeMs = 0.0;

    struct ActiveNote
    {
        int trackIndex;
        const MidiNote* note;
    };
    std::vector<ActiveNote> activeNotes;

    std::vector<Listener*> listeners;
};
