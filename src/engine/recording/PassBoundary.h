#pragma once

// Starts `pass`. Notes still held from earlier passes end at closeTick; TakeBuilder releases hold pedals just before
// it.
struct PassBoundary
{
    int pass = 0;
    int closeTick = 0;
    // Whether the loop wrapped into `pass`, rather than playback jumping there.
    bool loopWrap = false;
    // Where `pass` starts: the loop start after a wrap, the target after a jump.
    int startTick = 0;

    bool operator==(const PassBoundary&) const = default;
};
