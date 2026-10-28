#pragma once

#include <set>
#include <vector>

// An order lists, for each position, the index the track there had before reordering.
namespace TrackOrder
{
// The moved tracks keep their relative order and end up together at gap, which counts the boundaries
// between tracks from 0 (before the first) to numTracks (after the last).
std::vector<int> moveTogether(int numTracks, const std::set<int>& moved, int gap);

std::vector<int> inverse(const std::vector<int>& order);
bool isIdentity(const std::vector<int>& order);
} // namespace TrackOrder
