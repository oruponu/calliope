#pragma once

#include "model/GridDivision.h"

class TimelineGeometry
{
public:
    static constexpr int snapTicks = 480;

    explicit TimelineGeometry(int timelineStartXIn) : startX(timelineStartXIn) {}

    void setTicksPerQuarterNote(int ppqIn) { ppq = ppqIn; }
    void setBeatWidth(int w) { beatWidth = w; }
    void setGridDivision(GridDivision d) { division = d; }
    GridDivision gridDivision() const { return division; }

    int getTicksPerQuarterNote() const { return ppq; }
    int getBeatWidth() const { return beatWidth; }
    int timelineStartX() const { return startX; }

    int tickToX(int tick) const
    {
        if (ppq == 0)
            return startX;
        double beatsFromTick = static_cast<double>(tick) / ppq;
        return startX + static_cast<int>(beatsFromTick * beatWidth);
    }

    int tickToWidth(int durationTicks) const
    {
        if (ppq == 0)
            return 0;
        double beats = static_cast<double>(durationTicks) / ppq;
        return static_cast<int>(beats * beatWidth);
    }

    int xToTick(int x) const
    {
        if (ppq == 0)
            return 0;
        double beats = static_cast<double>(x - startX) / beatWidth;
        return static_cast<int>(beats * ppq);
    }

    int gridTicks() const
    {
        if (ppq == 0)
            return snapTicks;
        return division.stepTicks(ppq);
    }

private:
    int ppq = 0;
    int beatWidth = 80;
    GridDivision division;
    int startX;
};
