#include "ui/pianoroll/ProgramBrowserState.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("the browser opens on the program's family", "[ui][program-change]")
{
    CHECK(ProgramBrowserState::at(40) == (ProgramBrowserState{5, 40, ProgramBrowserState::Column::Programs}));
    CHECK(ProgramBrowserState::at(0) == (ProgramBrowserState{0, 0, ProgramBrowserState::Column::Programs}));
}

TEST_CASE("up and down move within the family", "[ui][program-change]")
{
    auto state = ProgramBrowserState::at(40);
    state.moveDown();
    CHECK(state.program == 41);

    state = ProgramBrowserState::at(47);
    state.moveDown();
    CHECK(state.program == 47);

    state = ProgramBrowserState::at(40);
    state.moveUp();
    CHECK(state.program == 40);
}

TEST_CASE("up and down in the families column change family and keep the row", "[ui][program-change]")
{
    auto state = ProgramBrowserState::at(42);
    state.moveLeft();
    state.moveDown();
    CHECK(state == (ProgramBrowserState{6, 50, ProgramBrowserState::Column::Families}));

    state = ProgramBrowserState::at(2);
    state.moveLeft();
    state.moveUp();
    CHECK(state == (ProgramBrowserState{0, 2, ProgramBrowserState::Column::Families}));

    state = ProgramBrowserState::at(122);
    state.moveLeft();
    state.moveDown();
    CHECK(state == (ProgramBrowserState{15, 122, ProgramBrowserState::Column::Families}));
}

TEST_CASE("left and right switch columns", "[ui][program-change]")
{
    auto state = ProgramBrowserState::at(40);
    state.moveLeft();
    CHECK(state.column == ProgramBrowserState::Column::Families);
    state.moveRight();
    CHECK(state.column == ProgramBrowserState::Column::Programs);
}

TEST_CASE("choosing a family keeps the row", "[ui][program-change]")
{
    auto state = ProgramBrowserState::at(42);
    state.chooseFamily(0);
    CHECK(state == (ProgramBrowserState{0, 2, ProgramBrowserState::Column::Families}));
}
