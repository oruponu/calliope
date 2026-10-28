#include "model/TrackOrder.h"
#include <cstddef>

namespace TrackOrder
{
std::vector<int> moveTogether(int numTracks, const std::set<int>& moved, int gap)
{
    std::vector<int> order;
    order.reserve(static_cast<std::size_t>(numTracks));
    for (int i = 0; i < gap; ++i)
        if (!moved.contains(i))
            order.push_back(i);
    order.insert(order.end(), moved.begin(), moved.end());
    for (int i = gap; i < numTracks; ++i)
        if (!moved.contains(i))
            order.push_back(i);
    return order;
}

std::vector<int> inverse(const std::vector<int>& order)
{
    std::vector<int> result(order.size());
    for (std::size_t position = 0; position < order.size(); ++position)
        result[static_cast<std::size_t>(order[position])] = static_cast<int>(position);
    return result;
}

bool isIdentity(const std::vector<int>& order)
{
    for (std::size_t position = 0; position < order.size(); ++position)
        if (order[position] != static_cast<int>(position))
            return false;
    return true;
}
} // namespace TrackOrder
