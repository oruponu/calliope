#pragma once

#include <algorithm>
#include <set>
#include <utility>
#include <vector>

class IndexSelection
{
public:
    bool isEmpty() const { return selected.empty(); }
    bool contains(int index) const { return selected.contains(index); }
    bool isSole(int index) const { return selected.size() == 1 && contains(index); }
    const std::set<int>& indices() const { return selected; }

    void clear() { selected.clear(); }
    void selectOnly(int index) { selected = {index}; }
    void add(int index) { selected.insert(index); }

    void toggle(int index)
    {
        if (selected.erase(index) == 0)
            selected.insert(index);
    }

    void assign(std::set<int> indices) { selected = std::move(indices); }

    std::vector<int> dragGroup(int anchor) const
    {
        if (contains(anchor) && selected.size() > 1)
            return std::vector<int>(selected.begin(), selected.end());
        return {anchor};
    }

    template <class T> void selectTicks(const std::vector<T>& items, const std::vector<int>& ticks)
    {
        selected.clear();
        for (int tick : ticks)
        {
            auto it = std::ranges::find(items, tick, &T::tick);
            if (it != items.end())
                selected.insert(static_cast<int>(it - items.begin()));
        }
    }

private:
    std::set<int> selected;
};
