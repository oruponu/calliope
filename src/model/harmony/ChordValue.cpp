#include "model/harmony/ChordValue.h"
#include "model/harmony/ChordChange.h"

namespace
{
// semitone of each XF note index 1-7 (C, D, E, F, G, A, B)
constexpr int chordNoteSemitones[] = {-1, 0, 2, 4, 5, 7, 9, 11};

// XF root nibbles per semitone
constexpr int sharpRoots[] = {0x31, 0x41, 0x32, 0x42, 0x33, 0x34, 0x44, 0x35, 0x45, 0x36, 0x46, 0x37};
constexpr int flatRoots[] = {0x31, 0x22, 0x32, 0x23, 0x33, 0x34, 0x25, 0x35, 0x26, 0x36, 0x27, 0x37};
constexpr int mixedRoots[] = {0x31, 0x41, 0x32, 0x23, 0x33, 0x34, 0x44, 0x35, 0x26, 0x36, 0x27, 0x37};
} // namespace

bool ChordValue::isValidRoot(int root)
{
    int noteIndex = root & 0x0F;
    int accIndex = (root >> 4) & 0x07;
    return root != ChordChange::none && noteIndex >= 1 && noteIndex <= 7 && accIndex <= 6;
}

int ChordValue::normalizeRoot(int root)
{
    return isValidRoot(root) ? root : 0x31;
}

int ChordValue::normalizeType(int type)
{
    return (type < 0 || type >= ChordChange::typeCount) ? 0 : type;
}

int ChordValue::normalizeBassRoot(int bassRoot)
{
    return isValidRoot(bassRoot) ? bassRoot : ChordChange::none;
}

int ChordValue::rootToSemitone(int root)
{
    if (!isValidRoot(root))
        return -1;

    int noteIndex = root & 0x0F;
    int accIndex = (root >> 4) & 0x07;
    int semitone = (chordNoteSemitones[noteIndex] + accIndex - 3) % 12;
    return semitone < 0 ? semitone + 12 : semitone;
}

int ChordValue::semitoneToRoot(int semitone, ChordSpelling spelling)
{
    semitone = ((semitone % 12) + 12) % 12;
    if (spelling == ChordSpelling::Sharp)
        return sharpRoots[semitone];
    if (spelling == ChordSpelling::Flat)
        return flatRoots[semitone];
    return mixedRoots[semitone];
}

ChordSpelling ChordValue::spellingForKeySignature(int sharpsOrFlats)
{
    if (sharpsOrFlats > 0)
        return ChordSpelling::Sharp;
    if (sharpsOrFlats < 0)
        return ChordSpelling::Flat;
    return ChordSpelling::Mixed;
}
