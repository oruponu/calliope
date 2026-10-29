#include "model/harmony/KeySignatureChangeList.h"
#include "model/harmony/KeySignatureValue.h"
#include <algorithm>

void KeySignatureChangeList::put(std::vector<KeySignatureChange>& changes, int tick, int sharpsOrFlats, bool isMinor)
{
    sharpsOrFlats = KeySignatureValue::normalizeSharpsOrFlats(sharpsOrFlats);

    for (auto& ks : changes)
    {
        if (ks.tick == tick)
        {
            ks.sharpsOrFlats = sharpsOrFlats;
            ks.isMinor = isMinor;
            return;
        }
    }
    changes.push_back({tick, sharpsOrFlats, isMinor});
    std::ranges::sort(changes, {}, &KeySignatureChange::tick);
}
