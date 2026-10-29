#pragma once

#include "model/controller/ProgramVoice.h"
#include "model/notation/GmPrograms.h"
#include <algorithm>

struct ProgramBrowserState
{
    enum class Column
    {
        Families,
        Programs,
        BankMsb,
        BankLsb
    };

    int family = 0;
    // The highlighted program; always in family.
    int program = 0;
    Column column = Column::Programs;
    ProgramBank bank;

    static ProgramBrowserState at(const ProgramVoice& voice)
    {
        const int program = std::clamp(voice.program, 0, GmPrograms::programCount - 1);
        return {program / GmPrograms::programsPerFamily, program, Column::Programs, voice.bank};
    }

    void moveUp()
    {
        if (column == Column::Families)
            chooseFamily(family - 1);
        else if (column == Column::Programs)
            program = std::max(family * GmPrograms::programsPerFamily, program - 1);
    }

    void moveDown()
    {
        if (column == Column::Families)
            chooseFamily(family + 1);
        else if (column == Column::Programs)
            program = std::min((family + 1) * GmPrograms::programsPerFamily - 1, program + 1);
    }

    void moveLeft()
    {
        setColumn(static_cast<Column>(std::max(static_cast<int>(Column::Families), static_cast<int>(column) - 1)));
    }

    void moveRight()
    {
        setColumn(static_cast<Column>(std::min(static_cast<int>(Column::BankLsb), static_cast<int>(column) + 1)));
    }

    void focusNext()
    {
        setColumn(column == Column::BankLsb ? Column::Families : static_cast<Column>(static_cast<int>(column) + 1));
    }

    void focusPrevious()
    {
        setColumn(column == Column::Families ? Column::BankLsb : static_cast<Column>(static_cast<int>(column) - 1));
    }

    void setBankValue(Column bankColumn, int value)
    {
        if (bankColumn == Column::BankMsb)
            bank.msb = std::clamp(value, 0, 127);
        else if (bankColumn == Column::BankLsb)
            bank.lsb = std::clamp(value, 0, 127);
    }

    ProgramVoice voice() const { return {bank, program}; }

    void chooseFamily(int newFamily)
    {
        const int row = program % GmPrograms::programsPerFamily;
        family = std::clamp(newFamily, 0, GmPrograms::familyCount - 1);
        program = family * GmPrograms::programsPerFamily + row;
        setColumn(Column::Families);
    }

    bool operator==(const ProgramBrowserState&) const = default;

private:
    void setColumn(Column newColumn) { column = newColumn; }
};
