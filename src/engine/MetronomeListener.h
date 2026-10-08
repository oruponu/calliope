#pragma once

class MetronomeListener
{
public:
    virtual ~MetronomeListener() = default;
    // Called on the playback timer thread.
    virtual void onMetronomeClick(bool accent) = 0;
};
