#include "MidiSequence.h"
#include <algorithm>

MidiSequence::MidiSequence()
{
    tempoChanges.push_back({0, 120.0});
    timeSignatureChanges.push_back({0, 4, 4});
}

void MidiSequence::clear()
{
    tracks.clear();
    tempoChanges.clear();
    tempoChanges.push_back({0, 120.0});
    timeSignatureChanges.clear();
    timeSignatureChanges.push_back({0, 4, 4});
    keySignatureChanges.clear();
    ticksPerQuarterNote = defaultTicksPerQuarterNote;
}

MidiTrack& MidiSequence::addTrack()
{
    tracks.emplace_back();
    return tracks.back();
}

void MidiSequence::removeTrack(int index)
{
    tracks.erase(tracks.begin() + index);
}

MidiTrack& MidiSequence::getTrack(int index)
{
    return tracks[index];
}

const MidiTrack& MidiSequence::getTrack(int index) const
{
    return tracks[index];
}

int MidiSequence::getNumTracks() const
{
    return static_cast<int>(tracks.size());
}

bool MidiSequence::isAnySolo() const
{
    for (const auto& track : tracks)
    {
        if (track.isSolo())
            return true;
    }
    return false;
}

void MidiSequence::setBpm(double newBpm)
{
    if (!tempoChanges.empty() && tempoChanges[0].tick == 0)
        tempoChanges[0].bpm = newBpm;
    else
        tempoChanges.insert(tempoChanges.begin(), {0, newBpm});
}

double MidiSequence::getBpm() const
{
    if (!tempoChanges.empty() && tempoChanges[0].tick == 0)
        return tempoChanges[0].bpm;
    return 120.0;
}

int MidiSequence::getTicksPerQuarterNote() const
{
    return ticksPerQuarterNote;
}

void MidiSequence::setTicksPerQuarterNote(int ppq)
{
    ticksPerQuarterNote = ppq;
}

double MidiSequence::getTempoAt(int tick) const
{
    double bpm = 120.0;
    for (auto it = tempoChanges.rbegin(); it != tempoChanges.rend(); ++it)
    {
        if (it->tick <= tick)
        {
            bpm = it->bpm;
            break;
        }
    }
    return bpm;
}

TimeSignatureChange MidiSequence::getTimeSignatureAt(int tick) const
{
    TimeSignatureChange ts = {0, 4, 4};
    for (auto it = timeSignatureChanges.rbegin(); it != timeSignatureChanges.rend(); ++it)
    {
        if (it->tick <= tick)
        {
            ts = *it;
            break;
        }
    }
    return ts;
}

const std::vector<TempoChange>& MidiSequence::getTempoChanges() const
{
    return tempoChanges;
}

const std::vector<TimeSignatureChange>& MidiSequence::getTimeSignatureChanges() const
{
    return timeSignatureChanges;
}

void MidiSequence::addTempoChange(int tick, double bpm)
{
    for (auto& tc : tempoChanges)
    {
        if (tc.tick == tick)
        {
            tc.bpm = bpm;
            return;
        }
    }
    tempoChanges.push_back({tick, bpm});
    std::sort(tempoChanges.begin(), tempoChanges.end(),
              [](const TempoChange& a, const TempoChange& b) { return a.tick < b.tick; });
}

void MidiSequence::addTimeSignatureChange(int tick, int num, int den)
{
    for (auto& ts : timeSignatureChanges)
    {
        if (ts.tick == tick)
        {
            ts.numerator = num;
            ts.denominator = den;
            return;
        }
    }
    timeSignatureChanges.push_back({tick, num, den});
    std::sort(timeSignatureChanges.begin(), timeSignatureChanges.end(),
              [](const TimeSignatureChange& a, const TimeSignatureChange& b) { return a.tick < b.tick; });
}

KeySignatureChange MidiSequence::getKeySignatureAt(int tick) const
{
    KeySignatureChange ks = {0, 0, false};
    for (auto it = keySignatureChanges.rbegin(); it != keySignatureChanges.rend(); ++it)
    {
        if (it->tick <= tick)
        {
            ks = *it;
            break;
        }
    }
    return ks;
}

const std::vector<KeySignatureChange>& MidiSequence::getKeySignatureChanges() const
{
    return keySignatureChanges;
}

void MidiSequence::addKeySignatureChange(int tick, int sharpsOrFlats, bool isMinor)
{
    for (auto& ks : keySignatureChanges)
    {
        if (ks.tick == tick)
        {
            ks.sharpsOrFlats = sharpsOrFlats;
            ks.isMinor = isMinor;
            return;
        }
    }
    keySignatureChanges.push_back({tick, sharpsOrFlats, isMinor});
    std::sort(keySignatureChanges.begin(), keySignatureChanges.end(),
              [](const KeySignatureChange& a, const KeySignatureChange& b) { return a.tick < b.tick; });
}

std::string MidiSequence::keySignatureToString(int sharpsOrFlats, bool isMinor)
{
    static const char* majorKeys[] = {"Cb", "Gb", "Db", "Ab", "Eb", "Bb", "F", "C",
                                      "G",  "D",  "A",  "E",  "B",  "F#", "C#"};
    static const char* minorKeys[] = {"Abm", "Ebm", "Bbm", "Fm",  "Cm",  "Gm",  "Dm", "Am",
                                      "Em",  "Bm",  "F#m", "C#m", "G#m", "D#m", "A#m"};

    int index = sharpsOrFlats + 7;
    if (index < 0 || index > 14)
        return "--";

    return isMinor ? minorKeys[index] : majorKeys[index];
}

double MidiSequence::ticksToSeconds(int ticks) const
{
    double seconds = 0.0;
    int prevTick = 0;
    double currentBpm = 120.0;

    for (const auto& tc : tempoChanges)
    {
        if (tc.tick >= ticks)
            break;

        if (tc.tick > prevTick)
        {
            double ticksPerSecond = (currentBpm / 60.0) * ticksPerQuarterNote;
            seconds += (tc.tick - prevTick) / ticksPerSecond;
            prevTick = tc.tick;
        }

        currentBpm = tc.bpm;
    }

    double ticksPerSecond = (currentBpm / 60.0) * ticksPerQuarterNote;
    seconds += (ticks - prevTick) / ticksPerSecond;

    return seconds;
}

int MidiSequence::secondsToTicks(double seconds) const
{
    double accSeconds = 0.0;
    int prevTick = 0;
    double currentBpm = 120.0;

    for (const auto& tc : tempoChanges)
    {
        double ticksPerSecond = (currentBpm / 60.0) * ticksPerQuarterNote;
        double segmentSeconds = (tc.tick - prevTick) / ticksPerSecond;

        if (accSeconds + segmentSeconds >= seconds)
        {
            double remainingSeconds = seconds - accSeconds;
            return prevTick + static_cast<int>(remainingSeconds * ticksPerSecond);
        }

        accSeconds += segmentSeconds;
        prevTick = tc.tick;
        currentBpm = tc.bpm;
    }

    double ticksPerSecond = (currentBpm / 60.0) * ticksPerQuarterNote;
    double remainingSeconds = seconds - accSeconds;
    return prevTick + static_cast<int>(remainingSeconds * ticksPerSecond);
}

BarBeatTick MidiSequence::tickToBarBeatTick(int tick) const
{
    int bar = 1;
    int pos = 0;

    for (size_t i = 0; i < timeSignatureChanges.size(); ++i)
    {
        const auto& ts = timeSignatureChanges[i];
        int ticksPerBeat = ticksPerQuarterNote * 4 / ts.denominator;
        int ticksPerBar = ticksPerBeat * ts.numerator;

        int nextChangeTick = (i + 1 < timeSignatureChanges.size()) ? timeSignatureChanges[i + 1].tick : tick + 1;

        if (tick < nextChangeTick)
        {
            int ticksInThisSection = tick - pos;
            int barsInSection = ticksInThisSection / ticksPerBar;
            int remainder = ticksInThisSection % ticksPerBar;
            bar += barsInSection;
            int beat = remainder / ticksPerBeat + 1;
            int tickInBeat = remainder % ticksPerBeat;
            return {bar, beat, tickInBeat};
        }

        int sectionTicks = nextChangeTick - pos;
        int barsInSection = sectionTicks / ticksPerBar;
        bar += barsInSection;
        pos = nextChangeTick;
    }

    return {bar, 1, 0};
}

int MidiSequence::barStartToTick(int targetBar) const
{
    if (targetBar <= 1)
        return 0;

    int bar = 1;
    int pos = 0;

    for (size_t i = 0; i < timeSignatureChanges.size(); ++i)
    {
        const auto& ts = timeSignatureChanges[i];
        int ticksPerBeat = ticksPerQuarterNote * 4 / ts.denominator;
        int ticksPerBar = ticksPerBeat * ts.numerator;

        if (i + 1 < timeSignatureChanges.size())
        {
            int nextChangeTick = timeSignatureChanges[i + 1].tick;
            int sectionTicks = nextChangeTick - pos;
            int barsInSection = sectionTicks / ticksPerBar;

            if (bar + barsInSection >= targetBar)
                return pos + (targetBar - bar) * ticksPerBar;

            bar += barsInSection;
            pos = nextChangeTick;
        }
        else
        {
            return pos + (targetBar - bar) * ticksPerBar;
        }
    }

    return 0;
}
