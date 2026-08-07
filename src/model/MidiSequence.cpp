#include "model/MidiSequence.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <ranges>

namespace
{
const char* const majorKeys[] = {"Cb", "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#"};
const char* const minorKeys[] = {"Abm", "Ebm", "Bbm", "Fm",  "Cm",  "Gm",  "Dm", "Am",
                                 "Em",  "Bm",  "F#m", "C#m", "G#m", "D#m", "A#m"};

const char* const chordNoteNames[] = {"", "C", "D", "E", "F", "G", "A", "B"};
const char* const chordAccidentals[] = {"bbb", "bb", "b", "", "#", "##", "###"};
const char* const chordTypeNames[] = {
    "",      "6",     "M7",     "M7(#11)", "add9",   "M7(9)", "6(9)", "aug", "m",     "m6",   "m7",   "m7b5",
    "madd9", "m7(9)", "m7(11)", "mM7",     "mM7(9)", "dim",   "dim7", "7",   "7sus4", "7b5",  "7(9)", "7(#11)",
    "7(13)", "7(b9)", "7(b13)", "7(#9)",   "M7aug",  "7aug",  "1+8",  "5",   "sus4",  "sus2", ""};

// semitone of each XF note index 1-7 (C, D, E, F, G, A, B)
constexpr int chordNoteSemitones[] = {-1, 0, 2, 4, 5, 7, 9, 11};

// XF root nibbles per semitone
constexpr int sharpRoots[] = {0x31, 0x41, 0x32, 0x42, 0x33, 0x34, 0x44, 0x35, 0x45, 0x36, 0x46, 0x37};
constexpr int flatRoots[] = {0x31, 0x22, 0x32, 0x23, 0x33, 0x34, 0x25, 0x35, 0x26, 0x36, 0x27, 0x37};
constexpr int mixedRoots[] = {0x31, 0x41, 0x32, 0x23, 0x33, 0x34, 0x44, 0x35, 0x26, 0x36, 0x27, 0x37};
} // namespace

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
    chordChanges.clear();
    ticksPerQuarterNote = defaultTicksPerQuarterNote;
}

MidiTrack& MidiSequence::addTrack()
{
    tracks.emplace_back();
    return tracks.back();
}

void MidiSequence::insertTrack(int index, const MidiTrack& track)
{
    tracks.insert(tracks.begin() + index, track);
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
    return std::ranges::any_of(tracks, [](const MidiTrack& track) { return track.isSolo(); });
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
    auto reversed = std::views::reverse(tempoChanges);
    auto it = std::ranges::find_if(reversed, [tick](const TempoChange& tc) { return tc.tick <= tick; });
    return it != reversed.end() ? it->bpm : 120.0;
}

TempoChange MidiSequence::getTempoChangeAt(int tick) const
{
    auto reversed = std::views::reverse(tempoChanges);
    auto it = std::ranges::find_if(reversed, [tick](const TempoChange& tc) { return tc.tick <= tick; });
    return it != reversed.end() ? *it : TempoChange{0, 120.0};
}

TimeSignatureChange MidiSequence::getTimeSignatureAt(int tick) const
{
    auto reversed = std::views::reverse(timeSignatureChanges);
    auto it = std::ranges::find_if(reversed, [tick](const TimeSignatureChange& ts) { return ts.tick <= tick; });
    return it != reversed.end() ? *it : TimeSignatureChange{0, 4, 4};
}

const std::vector<TempoChange>& MidiSequence::getTempoChanges() const
{
    return tempoChanges;
}

const std::vector<TimeSignatureChange>& MidiSequence::getTimeSignatureChanges() const
{
    return timeSignatureChanges;
}

int MidiSequence::addTempoChange(int tick, double bpm)
{
    for (size_t i = 0; i < tempoChanges.size(); ++i)
    {
        if (tempoChanges[i].tick == tick)
        {
            tempoChanges[i].bpm = bpm;
            return static_cast<int>(i);
        }
    }
    tempoChanges.push_back({tick, bpm});
    std::ranges::sort(tempoChanges, {}, &TempoChange::tick);
    auto it = std::ranges::find(tempoChanges, tick, &TempoChange::tick);
    return static_cast<int>(it - tempoChanges.begin());
}

void MidiSequence::addTimeSignatureChange(int tick, int num, int den)
{
    std::vector<int> bars;
    bars.reserve(timeSignatureChanges.size());
    for (const auto& ts : timeSignatureChanges)
        bars.push_back(tickToBarBeatTick(ts.tick).bar);

    bool modified = false;
    for (auto& ts : timeSignatureChanges)
    {
        if (ts.tick == tick)
        {
            ts.numerator = num;
            ts.denominator = den;
            modified = true;
            break;
        }
    }

    if (!modified)
    {
        int targetBar = tickToBarBeatTick(tick).bar;
        size_t insertPos = 0;
        while (insertPos < timeSignatureChanges.size() && timeSignatureChanges[insertPos].tick < tick)
            ++insertPos;
        timeSignatureChanges.insert(timeSignatureChanges.begin() + insertPos, {tick, num, den});
        bars.insert(bars.begin() + insertPos, targetBar);
    }

    if (!timeSignatureChanges.empty())
        timeSignatureChanges[0].tick = 0;
    for (size_t i = 1; i < timeSignatureChanges.size(); ++i)
    {
        int ticksPerBeat = ticksPerQuarterNote * 4 / timeSignatureChanges[i - 1].denominator;
        int ticksPerBar = ticksPerBeat * timeSignatureChanges[i - 1].numerator;
        int barsFromPrev = bars[i] - bars[i - 1];
        timeSignatureChanges[i].tick = timeSignatureChanges[i - 1].tick + barsFromPrev * ticksPerBar;
    }
}

std::vector<TimeSignatureChange>
MidiSequence::buildTimeSignatureChangesAfterMove(const std::vector<TimeSignatureChange>& before,
                                                 const std::vector<int>& movedIndices, int anchorIndex, int targetTick,
                                                 int ppq)
{
    const int count = static_cast<int>(before.size());
    if (anchorIndex <= 0 || anchorIndex >= count)
        return before;

    std::vector<bool> isMoving(before.size(), false);
    int firstMoving = count;
    for (int i : movedIndices)
        if (i >= 1 && i < count)
        {
            isMoving[static_cast<size_t>(i)] = true;
            firstMoving = std::min(firstMoving, i);
        }
    if (!isMoving[static_cast<size_t>(anchorIndex)])
        return before;

    std::vector<int> bars(before.size());
    bars[0] = 1;
    for (size_t i = 1; i < before.size(); ++i)
    {
        int ticksPerBar = ppq * 4 / before[i - 1].denominator * before[i - 1].numerator;
        bars[i] = bars[i - 1] + (before[i].tick - before[i - 1].tick) / ticksPerBar;
    }

    auto rebuildFromFirstMoving = [&](const std::vector<int>& barNumbers)
    {
        auto result = before;
        for (size_t i = static_cast<size_t>(firstMoving); i < result.size(); ++i)
        {
            int ticksPerBar = ppq * 4 / result[i - 1].denominator * result[i - 1].numerator;
            result[i].tick = result[i - 1].tick + (barNumbers[i] - barNumbers[i - 1]) * ticksPerBar;
        }
        return result;
    };
    auto shiftedBars = [&](int delta)
    {
        auto barNumbers = bars;
        for (size_t i = 0; i < barNumbers.size(); ++i)
            if (isMoving[i])
                barNumbers[i] += delta;
        return barNumbers;
    };

    auto base = rebuildFromFirstMoving(bars);
    const int anchorTick0 = base[static_cast<size_t>(anchorIndex)].tick;
    const int slope = rebuildFromFirstMoving(shiftedBars(1))[static_cast<size_t>(anchorIndex)].tick - anchorTick0;
    if (slope <= 0)
        return before;

    int delta = static_cast<int>(std::floor((targetTick - anchorTick0) / static_cast<double>(slope) + 0.5));

    int deltaLo = std::numeric_limits<int>::min();
    int deltaHi = std::numeric_limits<int>::max();
    for (int i = 0; i + 1 < count; ++i)
    {
        bool aMoving = isMoving[static_cast<size_t>(i)];
        bool bMoving = isMoving[static_cast<size_t>(i + 1)];
        int gapBars = bars[static_cast<size_t>(i + 1)] - bars[static_cast<size_t>(i)];
        if (bMoving && !aMoving)
            deltaLo = std::max(deltaLo, 1 - gapBars);
        else if (aMoving && !bMoving)
            deltaHi = std::min(deltaHi, gapBars - 1);
    }
    delta = (deltaLo > deltaHi) ? 0 : std::clamp(delta, deltaLo, deltaHi);

    if (delta == 0)
        return base;
    return rebuildFromFirstMoving(shiftedBars(delta));
}

std::vector<TimeSignatureChange>
MidiSequence::buildTimeSignatureChangesAfterDelete(const std::vector<TimeSignatureChange>& before,
                                                   const std::set<int>& deletedIndices, int ppq)
{
    if (before.empty())
        return before;

    std::vector<int> bars(before.size());
    bars[0] = 1;
    for (size_t i = 1; i < before.size(); ++i)
    {
        int ticksPerBar = ppq * 4 / before[i - 1].denominator * before[i - 1].numerator;
        bars[i] = bars[i - 1] + (before[i].tick - before[i - 1].tick) / ticksPerBar;
    }

    std::vector<TimeSignatureChange> result;
    std::vector<int> resultBars;
    result.reserve(before.size());
    resultBars.reserve(before.size());
    for (size_t i = 0; i < before.size(); ++i)
    {
        const bool remove = i > 0 && deletedIndices.count(static_cast<int>(i)) > 0;
        if (!remove)
        {
            result.push_back(before[i]);
            resultBars.push_back(bars[i]);
        }
    }

    if (result.size() == before.size())
        return before;

    for (size_t i = 1; i < result.size(); ++i)
    {
        int ticksPerBar = ppq * 4 / result[i - 1].denominator * result[i - 1].numerator;
        result[i].tick = result[i - 1].tick + (resultBars[i] - resultBars[i - 1]) * ticksPerBar;
    }
    return result;
}

std::vector<TimeSignatureChange>
MidiSequence::buildTimeSignatureChangesAfterPaste(const std::vector<TimeSignatureChange>& before,
                                                  const std::vector<RelativeTimeSignature>& items, int anchorBar,
                                                  int ppq)
{
    if (before.empty() || items.empty())
        return before;

    std::vector<int> bars(before.size());
    bars[0] = 1;
    for (size_t i = 1; i < before.size(); ++i)
    {
        int ticksPerBar = ppq * 4 / before[i - 1].denominator * before[i - 1].numerator;
        bars[i] = bars[i - 1] + (before[i].tick - before[i - 1].tick) / ticksPerBar;
    }

    auto result = before;
    auto resultBars = bars;
    for (const auto& item : items)
    {
        const int bar = anchorBar + item.barOffset;
        if (bar < 1)
            continue;
        auto it = std::ranges::lower_bound(resultBars, bar);
        const auto pos = it - resultBars.begin();
        if (it != resultBars.end() && *it == bar)
        {
            result[static_cast<size_t>(pos)].numerator = item.numerator;
            result[static_cast<size_t>(pos)].denominator = item.denominator;
        }
        else
        {
            result.insert(result.begin() + pos, {0, item.numerator, item.denominator});
            resultBars.insert(it, bar);
        }
    }

    for (size_t i = 1; i < result.size(); ++i)
    {
        int ticksPerBar = ppq * 4 / result[i - 1].denominator * result[i - 1].numerator;
        result[i].tick = result[i - 1].tick + (resultBars[i] - resultBars[i - 1]) * ticksPerBar;
    }
    return result;
}

KeySignatureChange MidiSequence::getKeySignatureAt(int tick) const
{
    auto reversed = std::views::reverse(keySignatureChanges);
    auto it = std::ranges::find_if(reversed, [tick](const KeySignatureChange& ks) { return ks.tick <= tick; });
    return it != reversed.end() ? *it : KeySignatureChange{0, 0, false};
}

const std::vector<KeySignatureChange>& MidiSequence::getKeySignatureChanges() const
{
    return keySignatureChanges;
}

const std::vector<ChordChange>& MidiSequence::getChordChanges() const
{
    return chordChanges;
}

int MidiSequence::normalizeSharpsOrFlats(int sharpsOrFlats)
{
    if (sharpsOrFlats == 7)
        return -5;
    if (sharpsOrFlats == -7)
        return 5;
    return sharpsOrFlats;
}

void MidiSequence::addKeySignatureChange(int tick, int sharpsOrFlats, bool isMinor)
{
    sharpsOrFlats = normalizeSharpsOrFlats(sharpsOrFlats);

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
    std::ranges::sort(keySignatureChanges, {}, &KeySignatureChange::tick);
}

void MidiSequence::setTempoChanges(std::vector<TempoChange> changes)
{
    tempoChanges = std::move(changes);
}

void MidiSequence::setTimeSignatureChanges(std::vector<TimeSignatureChange> changes)
{
    timeSignatureChanges = std::move(changes);
}

void MidiSequence::setKeySignatureChanges(std::vector<KeySignatureChange> changes)
{
    keySignatureChanges = std::move(changes);
}

void MidiSequence::setChordChanges(std::vector<ChordChange> changes)
{
    chordChanges = std::move(changes);
}

std::vector<KeySignatureChange>
MidiSequence::buildKeySignatureChangesAfterMove(const std::vector<KeySignatureChange>& before,
                                                const std::vector<int>& movedIndices, int anchorIndex,
                                                int targetTick) const
{
    const int count = static_cast<int>(before.size());
    if (anchorIndex < 0 || anchorIndex >= count)
        return before;

    std::vector<bool> isMoving(before.size(), false);
    for (int i : movedIndices)
        if (i >= 0 && i < count)
            isMoving[static_cast<size_t>(i)] = true;
    if (!isMoving[static_cast<size_t>(anchorIndex)])
        return before;

    std::vector<int> bars(before.size());
    for (size_t i = 0; i < before.size(); ++i)
        bars[i] = tickToBarBeatTick(before[i].tick).bar;

    int prevMovingBar = -1;
    for (int i = 0; i < count; ++i)
    {
        if (!isMoving[static_cast<size_t>(i)])
            continue;
        if (bars[static_cast<size_t>(i)] == prevMovingBar)
            return before;
        prevMovingBar = bars[static_cast<size_t>(i)];
    }

    int clampedTarget = std::max(0, targetTick);
    int targetBar = tickToBarBeatTick(clampedTarget).bar;
    int targetBarStart = barStartToTick(targetBar);
    if (clampedTarget - targetBarStart >= barStartToTick(targetBar + 1) - clampedTarget)
        ++targetBar;
    int delta = targetBar - bars[static_cast<size_t>(anchorIndex)];

    int deltaLo = std::numeric_limits<int>::min();
    int deltaHi = std::numeric_limits<int>::max();
    for (int i = 0; i < count; ++i)
    {
        if (!isMoving[static_cast<size_t>(i)])
            continue;
        if (i == 0)
            deltaLo = std::max(deltaLo, 1 - bars[0]);
        else if (!isMoving[static_cast<size_t>(i - 1)])
            deltaLo = std::max(deltaLo, bars[static_cast<size_t>(i - 1)] + 1 - bars[static_cast<size_t>(i)]);
        if (i + 1 < count && !isMoving[static_cast<size_t>(i + 1)])
        {
            int nextTick = before[static_cast<size_t>(i + 1)].tick;
            int nextBar = bars[static_cast<size_t>(i + 1)];
            int limit = barStartToTick(nextBar) == nextTick ? nextBar - 1 : nextBar;
            deltaHi = std::min(deltaHi, limit - bars[static_cast<size_t>(i)]);
        }
    }
    if (deltaLo > deltaHi)
        return before;
    delta = std::clamp(delta, deltaLo, deltaHi);

    auto result = before;
    for (int i = 0; i < count; ++i)
        if (isMoving[static_cast<size_t>(i)])
            result[static_cast<size_t>(i)].tick = barStartToTick(bars[static_cast<size_t>(i)] + delta);
    return result;
}

void MidiSequence::addChordChange(int tick, int chordRoot, int chordType, int bassRoot, int bassType)
{
    for (auto& cc : chordChanges)
    {
        if (cc.tick == tick)
        {
            cc.chordRoot = chordRoot;
            cc.chordType = chordType;
            cc.bassRoot = bassRoot;
            cc.bassType = bassType;
            return;
        }
    }
    chordChanges.push_back({tick, chordRoot, chordType, bassRoot, bassType});
    std::ranges::sort(chordChanges, {}, &ChordChange::tick);
}

std::pair<int, int> MidiSequence::chordAddSpanAt(int tick) const
{
    if (tick < 0)
        return {0, 0};

    int governing = -1;
    for (int i = 0; i < static_cast<int>(chordChanges.size()); ++i)
    {
        if (chordChanges[static_cast<size_t>(i)].tick > tick)
            break;
        governing = i;
    }

    if (governing >= 0 && !chordToString(chordChanges[static_cast<size_t>(governing)]).empty())
        return {0, 0};

    const int bar = tickToBarBeatTick(tick).bar;
    const int barStart = barStartToTick(bar);
    const int nextBarStart = barStartToTick(bar + 1);

    int start = barStart;
    if (governing >= 0)
        start = std::max(chordChanges[static_cast<size_t>(governing)].tick, barStart);

    int end = nextBarStart;
    for (const auto& cc : chordChanges)
    {
        if (cc.tick > start)
        {
            end = std::min(end, cc.tick);
            break;
        }
    }

    return {start, end};
}

std::vector<ChordChange> MidiSequence::buildChordChangesAfterAdd(const std::vector<ChordChange>& before, int startTick,
                                                                 int endTick, int chordRoot, int chordType,
                                                                 int bassRoot, int bassType)
{
    if (endTick <= startTick)
        return before;

    auto changes = before;

    auto upsert = [&changes](const ChordChange& entry)
    {
        for (auto& cc : changes)
        {
            if (cc.tick == entry.tick)
            {
                cc = entry;
                return;
            }
        }
        changes.push_back(entry);
        std::ranges::sort(changes, {}, &ChordChange::tick);
    };

    upsert({startTick, chordRoot, chordType, bassRoot, bassType});

    auto next = std::ranges::find_if(changes, [startTick](const ChordChange& cc) { return cc.tick > startTick; });
    if (next == changes.end() || next->tick > endTick)
        upsert({endTick, chordNone, chordTypeCount, chordNone, chordNone});

    return changes;
}

std::string MidiSequence::chordRootToString(int root)
{
    int noteIndex = root & 0x0F;
    int accIndex = (root >> 4) & 0x07;
    if (root == chordNone || noteIndex < 1 || noteIndex > 7 || accIndex > 6)
        return {};

    std::string result = chordNoteNames[noteIndex];
    result += chordAccidentals[accIndex];
    return result;
}

bool MidiSequence::chordRootFromString(const std::string& text, int& root)
{
    std::string s;
    for (char c : text)
        if (!std::isspace(static_cast<unsigned char>(c)))
            s += c;

    if (s.empty())
        return false;

    s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    for (size_t i = 1; i < s.size(); ++i)
        s[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));

    int noteIndex = 0;
    for (int i = 1; i <= 7; ++i)
        if (chordNoteNames[i][0] == s[0])
            noteIndex = i;
    if (noteIndex == 0)
        return false;

    int sharps = 0;
    int flats = 0;
    for (size_t i = 1; i < s.size(); ++i)
    {
        if (s[i] == '#')
            ++sharps;
        else if (s[i] == 'b')
            ++flats;
        else
            return false;
    }
    if ((sharps > 0 && flats > 0) || sharps > 3 || flats > 3)
        return false;

    root = ((3 + sharps - flats) << 4) | noteIndex;
    return true;
}

std::string MidiSequence::chordTypeToString(int type)
{
    if (type < 0 || type >= chordTypeCount)
        return {};

    return chordTypeNames[type];
}

bool MidiSequence::chordTypeFromString(const std::string& text, int& type)
{
    std::string s;
    for (char c : text)
        if (!std::isspace(static_cast<unsigned char>(c)))
            s += c;

    for (int i = 0; i < chordTypeCount; ++i)
    {
        if (s == chordTypeNames[i])
        {
            type = i;
            return true;
        }
    }
    return false;
}

int MidiSequence::chordRootToSemitone(int root)
{
    int noteIndex = root & 0x0F;
    int accIndex = (root >> 4) & 0x07;
    if (root == chordNone || noteIndex < 1 || noteIndex > 7 || accIndex > 6)
        return -1;

    int semitone = (chordNoteSemitones[noteIndex] + accIndex - 3) % 12;
    return semitone < 0 ? semitone + 12 : semitone;
}

int MidiSequence::semitoneToChordRoot(int semitone, ChordSpelling spelling)
{
    semitone = ((semitone % 12) + 12) % 12;
    if (spelling == ChordSpelling::Sharp)
        return sharpRoots[semitone];
    if (spelling == ChordSpelling::Flat)
        return flatRoots[semitone];
    return mixedRoots[semitone];
}

ChordSpelling MidiSequence::chordSpellingForKeySignature(int sharpsOrFlats)
{
    if (sharpsOrFlats > 0)
        return ChordSpelling::Sharp;
    if (sharpsOrFlats < 0)
        return ChordSpelling::Flat;
    return ChordSpelling::Mixed;
}

int MidiSequence::normalizeChordRoot(int root)
{
    return chordRootToString(root).empty() ? 0x31 : root;
}

int MidiSequence::normalizeChordType(int type)
{
    return (type < 0 || type >= chordTypeCount) ? 0 : type;
}

int MidiSequence::normalizeChordBassRoot(int bassRoot)
{
    return chordRootToString(bassRoot).empty() ? chordNone : bassRoot;
}

std::string MidiSequence::chordToString(const ChordChange& chord)
{
    int noteIndex = chord.chordRoot & 0x0F;

    // No Chord: root=0x7F or noteIndex=0 or chordType=34(cc)
    if (chord.chordRoot == chordNone || noteIndex == 0 || chord.chordType == chordTypeCount)
        return {};

    std::string result = chordRootToString(chord.chordRoot);
    if (result.empty())
        return "--";

    result += chordTypeToString(chord.chordType);

    std::string bassText = chordRootToString(chord.bassRoot);
    if (!bassText.empty())
    {
        result += "/";
        result += bassText;
    }

    return result;
}

std::string MidiSequence::keySignatureToString(int sharpsOrFlats, bool isMinor)
{
    int index = sharpsOrFlats + 7;
    if (index < 0 || index > 14)
        return "--";

    return isMinor ? minorKeys[index] : majorKeys[index];
}

bool MidiSequence::keySignatureFromString(const std::string& text, int& sharpsOrFlats, bool& isMinor)
{
    std::string s;
    for (char c : text)
        if (!std::isspace(static_cast<unsigned char>(c)))
            s += c;

    if (s.empty())
        return false;

    s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    for (size_t i = 1; i < s.size(); ++i)
        s[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));

    for (int i = 0; i < 15; ++i)
    {
        if (s == majorKeys[i])
        {
            sharpsOrFlats = i - 7;
            isMinor = false;
            return true;
        }
        if (s == minorKeys[i])
        {
            sharpsOrFlats = i - 7;
            isMinor = true;
            return true;
        }
    }

    struct Alias
    {
        const char* name;
        int sharpsOrFlats;
        bool isMinor;
    };
    static const Alias aliases[] = {
        {"D#", -3, false}, // → Eb
        {"G#", -4, false}, // → Ab
        {"A#", -2, false}, // → Bb
        {"Dbm", 4, true},  // → C#m
        {"Gbm", 3, true},  // → F#m
    };

    for (const auto& a : aliases)
    {
        if (s == a.name)
        {
            sharpsOrFlats = a.sharpsOrFlats;
            isMinor = a.isMinor;
            return true;
        }
    }

    return false;
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

int MidiSequence::barBeatTickToTick(int bar, int beat, int tickInBeat) const
{
    bar = std::max(1, bar);

    int barStart = barStartToTick(bar);

    auto ts = getTimeSignatureAt(barStart);
    int ticksPerBeat = ticksPerQuarterNote * 4 / ts.denominator;

    return std::max(0, barStart + (beat - 1) * ticksPerBeat + tickInBeat);
}

void MidiSequence::addListener(Listener* listener)
{
    if (listener != nullptr && std::find(listeners.begin(), listeners.end(), listener) == listeners.end())
        listeners.push_back(listener);
}

void MidiSequence::removeListener(Listener* listener)
{
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

void MidiSequence::notifyNotesChanged(int trackIndex)
{
    const auto snapshot = listeners;
    for (auto* l : snapshot)
        l->notesChanged(trackIndex);
}

void MidiSequence::notifyTracksChanged()
{
    const auto snapshot = listeners;
    for (auto* l : snapshot)
        l->tracksChanged();
}

void MidiSequence::notifyTempoChanged()
{
    const auto snapshot = listeners;
    for (auto* l : snapshot)
        l->tempoChanged();
}

void MidiSequence::notifyTimelineMetadataChanged()
{
    const auto snapshot = listeners;
    for (auto* l : snapshot)
        l->timelineMetadataChanged();
}

void MidiSequence::notifySequenceReset()
{
    const auto snapshot = listeners;
    for (auto* l : snapshot)
        l->sequenceReset();
}
