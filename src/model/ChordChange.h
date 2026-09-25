#pragma once

struct ChordChange
{
    static constexpr int none = 0x7F;
    // also the chord type of a canonical No Chord
    static constexpr int typeCount = 34;

    int tick;
    int chordRoot; // XF format: upper nibble=accidental(0-6), lower nibble=note(0-7)
    int chordType; // XF format: 0-34
    int bassRoot;  // same as chordRoot, 0x7F=none
    int bassType;  // same as chordType, 0x7F=none

    // No Chord: root=0x7F or noteIndex=0 or chordType=34(cc)
    bool isNoChord() const { return chordRoot == none || (chordRoot & 0x0F) == 0 || chordType == typeCount; }

    static ChordChange noChord(int tick) { return {tick, none, typeCount, none, none}; }

    bool operator==(const ChordChange&) const = default;
};
