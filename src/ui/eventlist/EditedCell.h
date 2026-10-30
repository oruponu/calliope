#pragma once

#include "ui/eventlist/EditedColumn.h"

struct EditedCell
{
    int row = 0;
    EditedColumn column = EditedColumn::Position;

    bool operator==(const EditedCell&) const = default;
};
