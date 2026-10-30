#pragma once

#include "edit/controller/CopiedControllerPoint.h"
#include "model/controller/ControllerKind.h"
#include <optional>
#include <vector>

struct CopiedControllerPoints
{
    ControllerKind kind;
    std::vector<CopiedControllerPoint> points;

    bool operator==(const CopiedControllerPoints&) const = default;
};
