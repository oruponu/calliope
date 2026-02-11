#pragma once

struct MidiNote
{
    int noteNumber = 60;
    int velocity = 100;
    int startTick = 0;
    int duration = 480;
    int channel = 1; // 1-16

    int endTick() const { return startTick + duration; }
};
