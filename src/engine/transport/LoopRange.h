#pragma once

struct LoopRange
{
    bool enabled = false;
    int start = 0;
    int end = 0;

    bool operator==(const LoopRange&) const = default;
};
