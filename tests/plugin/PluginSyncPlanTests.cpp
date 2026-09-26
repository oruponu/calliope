#include "plugin/PluginSyncPlan.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>

namespace
{
std::shared_ptr<const PluginAssignment> makeAssignment()
{
    return std::make_shared<const PluginAssignment>(PluginAssignment{"<PLUGIN/>", {}});
}
} // namespace

TEST_CASE("live instance whose track was removed is retired", "[plugin][sync]")
{
    MidiSequence seq;
    seq.addTrack().setPluginAssignment(makeAssignment());
    seq.addTrack();
    const TrackId removed = seq.getTrack(0).getId();
    seq.removeTrack(0);

    const auto plan = planPluginSync(seq, {removed}, {});
    CHECK(plan.toRetire == std::vector<TrackId>{removed});
    CHECK(plan.toDestroy.empty());
    CHECK(plan.toCreate.empty());
}

TEST_CASE("live instance whose track lost its assignment is destroyed", "[plugin][sync]")
{
    MidiSequence seq;
    seq.addTrack();
    const TrackId id = seq.getTrack(0).getId();

    const auto plan = planPluginSync(seq, {id}, {});
    CHECK(plan.toRetire.empty());
    CHECK(plan.toDestroy == std::vector<TrackId>{id});
    CHECK(plan.toCreate.empty());
}

TEST_CASE("track with an assignment and no instance is created", "[plugin][sync]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack().setPluginAssignment(makeAssignment());
    const TrackId id = seq.getTrack(1).getId();

    const auto plan = planPluginSync(seq, {}, {});
    CHECK(plan.toRetire.empty());
    CHECK(plan.toDestroy.empty());
    CHECK(plan.toCreate == std::vector<TrackId>{id});
}

TEST_CASE("failed track is not created", "[plugin][sync]")
{
    MidiSequence seq;
    seq.addTrack().setPluginAssignment(makeAssignment());
    const TrackId id = seq.getTrack(0).getId();

    const auto plan = planPluginSync(seq, {}, {id});
    CHECK(plan.toCreate.empty());
}

TEST_CASE("track without an assignment and no instance is left alone", "[plugin][sync]")
{
    MidiSequence seq;
    seq.addTrack();

    const auto plan = planPluginSync(seq, {}, {});
    CHECK(plan.toRetire.empty());
    CHECK(plan.toDestroy.empty());
    CHECK(plan.toCreate.empty());
}

TEST_CASE("nothing changes when instances match assignments", "[plugin][sync]")
{
    MidiSequence seq;
    seq.addTrack().setPluginAssignment(makeAssignment());
    seq.addTrack();
    const TrackId id = seq.getTrack(0).getId();

    const auto plan = planPluginSync(seq, {id}, {});
    CHECK(plan.toRetire.empty());
    CHECK(plan.toDestroy.empty());
    CHECK(plan.toCreate.empty());
}
