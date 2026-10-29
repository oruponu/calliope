#include "model/harmony/ChordChangeList.h"
#include <algorithm>

void ChordChangeList::put(std::vector<ChordChange>& changes, int tick, int chordRoot, int chordType, int bassRoot,
                          int bassType)
{
    for (auto& cc : changes)
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
    changes.push_back({tick, chordRoot, chordType, bassRoot, bassType});
    std::ranges::sort(changes, {}, &ChordChange::tick);
}
