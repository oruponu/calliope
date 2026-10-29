#pragma once

struct KeySignatureChange
{
    int tick;
    int sharpsOrFlats; // -7..+7 (negative=flats, positive=sharps)
    bool isMinor;
    bool operator==(const KeySignatureChange&) const = default;
};
