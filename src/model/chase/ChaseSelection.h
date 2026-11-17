#pragma once

#include <optional>

// The RPN or NRPN that data entry applies to. Either half may not have been sent yet.
struct ChaseSelection
{
    static constexpr int nullParameter = 127;

    bool nrpn = false;
    std::optional<int> msb;
    std::optional<int> lsb;

    // Whether data entry would reach a known parameter.
    bool namesParameter() const { return msb && lsb && !(*msb == nullParameter && *lsb == nullParameter); }
    // The ChaseItem number of the parameter; only when namesParameter().
    int parameterNumber() const { return (nrpn ? 1 << 14 : 0) | *msb << 7 | *lsb; }
};
