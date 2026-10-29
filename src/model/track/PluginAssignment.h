#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct PluginAssignment
{
    std::string descriptionXml;
    // Not kept in sync with the running plugin; the host holds the live state.
    std::vector<std::byte> state;
};
