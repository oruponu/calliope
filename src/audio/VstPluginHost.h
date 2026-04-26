#pragma once

#include "../engine/PlaybackEngine.h"

class VstPluginHost : public PlaybackEngine::Listener
{
public:
    void onNoteOn(int trackIndex, const MidiNote& note) override;
    void onNoteOff(int trackIndex, const MidiNote& note) override;
    void onMidiEvent(int trackIndex, const MidiEvent& event) override;
};
