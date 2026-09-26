#pragma once

#include "model/PluginAssignment.h"
#include "model/TrackId.h"
#include <cstddef>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

// Plugin states of instances whose track left the sequence. An entry lives as long as some copy
// of the track's assignment does (the model or an undo action), which is what owner observes.
template <class State> class RetiredStateStore
{
public:
    void put(TrackId id, std::weak_ptr<const PluginAssignment> owner, State state)
    {
        entries.insert_or_assign(id, Entry{std::move(owner), std::move(state)});
    }

    std::optional<State> take(TrackId id)
    {
        auto it = entries.find(id);
        if (it == entries.end())
            return std::nullopt;
        std::optional<State> state{std::move(it->second.state)};
        entries.erase(it);
        return state;
    }

    void collectExpired()
    {
        std::erase_if(entries, [](const auto& entry) { return entry.second.owner.expired(); });
    }

    void clear() { entries.clear(); }
    std::size_t size() const { return entries.size(); }

private:
    struct Entry
    {
        std::weak_ptr<const PluginAssignment> owner;
        State state;
    };

    std::unordered_map<TrackId, Entry> entries;
};
