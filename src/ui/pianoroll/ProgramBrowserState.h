#pragma once

#include "model/GmPrograms.h"
#include <algorithm>

struct ProgramBrowserState
{
    enum class Column
    {
        Families,
        Programs
    };

    int family = 0;
    // The highlighted program; always in family.
    int program = 0;
    Column column = Column::Programs;

    static ProgramBrowserState at(int program)
    {
        program = std::clamp(program, 0, GmPrograms::programCount - 1);
        return {program / GmPrograms::programsPerFamily, program, Column::Programs};
    }

    void moveUp()
    {
        if (column == Column::Families)
            chooseFamily(family - 1);
        else
            program = std::max(family * GmPrograms::programsPerFamily, program - 1);
    }

    void moveDown()
    {
        if (column == Column::Families)
            chooseFamily(family + 1);
        else
            program = std::min((family + 1) * GmPrograms::programsPerFamily - 1, program + 1);
    }

    void moveLeft() { column = Column::Families; }
    void moveRight() { column = Column::Programs; }

    void chooseFamily(int newFamily)
    {
        const int row = program % GmPrograms::programsPerFamily;
        family = std::clamp(newFamily, 0, GmPrograms::familyCount - 1);
        program = family * GmPrograms::programsPerFamily + row;
        column = Column::Families;
    }

    bool operator==(const ProgramBrowserState&) const = default;
};
