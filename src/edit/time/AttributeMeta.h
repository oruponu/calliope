#pragma once

#include <cstdint>

// Sequence number, text, copyright, name, instrument, program name, device name, channel prefix, port, SMPTE offset,
// sequencer specific.
constexpr bool isAttributeMeta(std::uint8_t type)
{
    return type <= 0x04 || type == 0x08 || type == 0x09 || type == 0x20 || type == 0x21 || type == 0x54 || type == 0x7F;
}
