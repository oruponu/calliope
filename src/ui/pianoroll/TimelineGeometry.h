#pragma once

class TimelineGeometry
{
public:
    static constexpr int snapTicks = 480;

    explicit TimelineGeometry(int timelineStartXIn) : startX(timelineStartXIn) {}

    void setTicksPerQuarterNote(int ppqIn) { ppq = ppqIn; }
    void setBeatWidth(int w) { beatWidth = w; }
    void setQuantizeDenominator(int d) { quantizeDenominator = d; }

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
        return ppq * 4 / quantizeDenominator;
    }

    int roundTickToGrid(int tick) const
    {
        const int grid = gridTicks();
        return ((tick + grid / 2) / grid) * grid;
    }

    int floorTickToGrid(int tick) const
    {
        const int grid = gridTicks();
        return (tick / grid) * grid;
    }

private:
    int ppq = 0;
    int beatWidth = 80;
    int quantizeDenominator = 4;
    int startX;
};
