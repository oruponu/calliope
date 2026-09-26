#include "model/MidiSequence.h"
#include "plugin/RetiredStateStore.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <optional>

namespace
{
std::shared_ptr<const PluginAssignment> makeAssignment()
{
    return std::make_shared<const PluginAssignment>(PluginAssignment{"<PLUGIN/>", {}});
}
} // namespace

TEST_CASE("take returns the stored state and removes it", "[plugin][retired]")
{
    RetiredStateStore<int> store;
    const auto owner = makeAssignment();
    store.put(TrackId{1}, owner, 42);
    CHECK(store.take(TrackId{1}) == std::optional<int>{42});
    CHECK_FALSE(store.take(TrackId{1}).has_value());
    CHECK(store.size() == 0);
}

TEST_CASE("take returns nullopt for an unknown id", "[plugin][retired]")
{
    RetiredStateStore<int> store;
    CHECK_FALSE(store.take(TrackId{7}).has_value());
}

TEST_CASE("put replaces an earlier state for the same id", "[plugin][retired]")
{
    RetiredStateStore<int> store;
    const auto owner = makeAssignment();
    store.put(TrackId{1}, owner, 1);
    store.put(TrackId{1}, owner, 2);
    CHECK(store.size() == 1);
    CHECK(store.take(TrackId{1}) == std::optional<int>{2});
}

TEST_CASE("collectExpired keeps states whose owner is alive", "[plugin][retired]")
{
    RetiredStateStore<int> store;
    const auto owner = makeAssignment();
    store.put(TrackId{1}, owner, 42);
    store.collectExpired();
    CHECK(store.size() == 1);
}

TEST_CASE("collectExpired drops states whose owner is gone", "[plugin][retired]")
{
    RetiredStateStore<int> store;
    auto owner = makeAssignment();
    store.put(TrackId{1}, owner, 42);
    owner.reset();
    store.collectExpired();
    CHECK(store.size() == 0);
}

TEST_CASE("clear drops every state", "[plugin][retired]")
{
    RetiredStateStore<int> store;
    const auto owner = makeAssignment();
    store.put(TrackId{1}, owner, 1);
    store.put(TrackId{2}, owner, 2);
    store.clear();
    CHECK(store.size() == 0);
}

TEST_CASE("state survives while a saved copy of the removed track is alive", "[plugin][retired]")
{
    MidiSequence seq;
    seq.addTrack().setPluginAssignment(makeAssignment());
    const TrackId id = seq.getTrack(0).getId();
    std::optional<MidiTrack> saved = seq.getTrack(0);
    seq.removeTrack(0);

    RetiredStateStore<int> store;
    store.put(id, saved->getPluginAssignment(), 7);
    store.collectExpired();
    CHECK(store.size() == 1);

    saved.reset();
    store.collectExpired();
    CHECK(store.size() == 0);
}
