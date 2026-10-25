#include "ui/pianoroll/ProgramBrowserState.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("the browser opens on the program's family", "[ui][program-change]")
{
    CHECK(ProgramBrowserState::at({{}, 40}) == (ProgramBrowserState{5, 40, ProgramBrowserState::Column::Programs}));
    CHECK(ProgramBrowserState::at({{}, 0}) == (ProgramBrowserState{0, 0, ProgramBrowserState::Column::Programs}));
}

TEST_CASE("the browser opens with the voice's bank", "[ui][program-change]")
{
    const auto state = ProgramBrowserState::at({{8, 2}, 40});
    CHECK(state.bank == (ProgramBank{8, 2}));
    CHECK(state.program == 40);
    CHECK(state.column == ProgramBrowserState::Column::Programs);
}

TEST_CASE("up and down move within the family", "[ui][program-change]")
{
    auto state = ProgramBrowserState::at({{}, 40});
    state.moveDown();
    CHECK(state.program == 41);

    state = ProgramBrowserState::at({{}, 47});
    state.moveDown();
    CHECK(state.program == 47);

    state = ProgramBrowserState::at({{}, 40});
    state.moveUp();
    CHECK(state.program == 40);
}

TEST_CASE("up and down in the families column change family and keep the row", "[ui][program-change]")
{
    auto state = ProgramBrowserState::at({{}, 42});
    state.moveLeft();
    state.moveDown();
    CHECK(state == (ProgramBrowserState{6, 50, ProgramBrowserState::Column::Families}));

    state = ProgramBrowserState::at({{}, 2});
    state.moveLeft();
    state.moveUp();
    CHECK(state == (ProgramBrowserState{0, 2, ProgramBrowserState::Column::Families}));

    state = ProgramBrowserState::at({{}, 122});
    state.moveLeft();
    state.moveDown();
    CHECK(state == (ProgramBrowserState{15, 122, ProgramBrowserState::Column::Families}));
}

TEST_CASE("tab moves through the lists and the bank fields and wraps", "[ui][program-change]")
{
    auto state = ProgramBrowserState::at({{}, 40});
    state.focusNext();
    state.focusNext();
    CHECK(state.column == ProgramBrowserState::Column::BankLsb);
    state.focusNext();
    CHECK(state.column == ProgramBrowserState::Column::Families);
    state.focusPrevious();
    CHECK(state.column == ProgramBrowserState::Column::BankLsb);
}

TEST_CASE("left and right move through the bank fields without wrapping", "[ui][program-change]")
{
    auto state = ProgramBrowserState::at({{}, 40});
    state.moveRight();
    state.moveRight();
    CHECK(state.column == ProgramBrowserState::Column::BankLsb);
    state.moveRight();
    CHECK(state.column == ProgramBrowserState::Column::BankLsb);
    state.moveLeft();
    state.moveLeft();
    state.moveLeft();
    CHECK(state.column == ProgramBrowserState::Column::Families);
}

TEST_CASE("choosing a family keeps the row", "[ui][program-change]")
{
    auto state = ProgramBrowserState::at({{}, 42});
    state.chooseFamily(0);
    CHECK(state == (ProgramBrowserState{0, 2, ProgramBrowserState::Column::Families}));
}

TEST_CASE("a bank value is kept within range", "[ui][program-change]")
{
    auto state = ProgramBrowserState::at({{}, 40});
    state.setBankValue(ProgramBrowserState::Column::BankMsb, 200);
    CHECK(state.bank.msb == 127);
    state.setBankValue(ProgramBrowserState::Column::BankLsb, -3);
    CHECK(state.bank.lsb == 0);
    state.setBankValue(ProgramBrowserState::Column::BankMsb, 8);
    CHECK(state.bank.msb == 8);
}

TEST_CASE("up and down do nothing in a bank column", "[ui][program-change]")
{
    auto state = ProgramBrowserState::at({{8, 0}, 40});
    state.moveRight();
    const auto before = state;
    state.moveUp();
    state.moveDown();
    CHECK(state == before);
}

TEST_CASE("the voice is the program with the bank", "[ui][program-change]")
{
    CHECK(ProgramBrowserState::at({{8, 2}, 40}).voice() == (ProgramVoice{{8, 2}, 40}));
}
