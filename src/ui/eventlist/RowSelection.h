#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace RowSelection
{
// Rows in ascending order. SparseSet::addRange sorts every range on each call, so adding scattered rows one by one is
// quadratic; cutting the gaps out of one spanning range from the front only ever splits the last range.
inline juce::SparseSet<int> fromRows(const std::vector<int>& rows)
{
    juce::SparseSet<int> set;
    if (rows.empty())
        return set;

    set.addRange({rows.front(), rows.back() + 1});
    for (size_t i = 1; i < rows.size(); ++i)
        if (rows[i] > rows[i - 1] + 1)
            set.removeRange({rows[i - 1] + 1, rows[i]});
    return set;
}
} // namespace RowSelection
