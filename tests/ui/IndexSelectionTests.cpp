#include "ui/pianoroll/strips/IndexSelection.h"
#include <catch2/catch_test_macros.hpp>
#include <set>
#include <utility>
#include <vector>

namespace
{
struct Item
{
    int tick;
};

IndexSelection selectionOf(std::set<int> indices)
{
    IndexSelection selection;
    selection.assign(std::move(indices));
    return selection;
}
} // namespace

TEST_CASE("a new selection is empty", "[ui][selection]")
{
    IndexSelection selection;
    CHECK(selection.isEmpty());
    CHECK(selection.indices().empty());
}

TEST_CASE("toggling adds an unselected index and removes a selected one", "[ui][selection]")
{
    auto selection = selectionOf({1});
    selection.toggle(3);
    CHECK(selection.indices() == std::set<int>{1, 3});
    selection.toggle(1);
    CHECK(selection.indices() == std::set<int>{3});
}

TEST_CASE("selecting only one index replaces the selection", "[ui][selection]")
{
    auto selection = selectionOf({0, 2, 4});
    selection.selectOnly(5);
    CHECK(selection.indices() == std::set<int>{5});
}

TEST_CASE("adding an index keeps the existing selection", "[ui][selection]")
{
    auto selection = selectionOf({2});
    selection.add(0);
    selection.add(2);
    CHECK(selection.indices() == std::set<int>{0, 2});
}

TEST_CASE("clearing empties the selection", "[ui][selection]")
{
    auto selection = selectionOf({1, 2});
    selection.clear();
    CHECK(selection.isEmpty());
}

TEST_CASE("an index is the sole selection only when nothing else is selected", "[ui][selection]")
{
    CHECK(selectionOf({2}).isSole(2));
    CHECK_FALSE(selectionOf({2}).isSole(3));
    CHECK_FALSE(selectionOf({2, 3}).isSole(2));
    CHECK_FALSE(IndexSelection{}.isSole(2));
}

TEST_CASE("dragging a selected index of a multiple selection drags the whole selection", "[ui][selection]")
{
    CHECK(selectionOf({1, 4, 6}).dragGroup(4) == std::vector<int>{1, 4, 6});
}

TEST_CASE("dragging an unselected index drags only that index", "[ui][selection]")
{
    CHECK(selectionOf({1, 4}).dragGroup(2) == std::vector<int>{2});
}

TEST_CASE("dragging the sole selected index drags only that index", "[ui][selection]")
{
    CHECK(selectionOf({4}).dragGroup(4) == std::vector<int>{4});
}

TEST_CASE("selecting ticks replaces the selection with the items at those ticks", "[ui][selection]")
{
    const std::vector<Item> items{{0}, {480}, {960}, {1920}};
    auto selection = selectionOf({0});
    selection.selectTicks(items, {1920, 480});
    CHECK(selection.indices() == std::set<int>{1, 3});
}

TEST_CASE("selecting ticks ignores ticks without an item", "[ui][selection]")
{
    const std::vector<Item> items{{0}, {480}};
    auto selection = selectionOf({1});
    selection.selectTicks(items, {240, 960});
    CHECK(selection.isEmpty());
}

TEST_CASE("selecting a tick shared by two items selects the first of them", "[ui][selection]")
{
    const std::vector<Item> items{{0}, {480}, {480}};
    IndexSelection selection;
    selection.selectTicks(items, {480});
    CHECK(selection.indices() == std::set<int>{1});
}
