#include "model/MetaEventRetention.h"

namespace MetaEventRetention
{
bool isRetained(int type, const std::vector<std::uint8_t>& data, Owner owner)
{
    if (type < 0x00 || type > 0x7F)
        return false;
    if (type == 0x2F || type == 0x20 || type == 0x51 || type == 0x58 || type == 0x59)
        return false;
    if (type == 0x03 && owner == Owner::Track)
        return false;
    if (type == 0x7F && data.size() >= 3 && data[0] == 0x43 && data[1] == 0x7B && data[2] == 0x01)
        return false;
    return true;
}
} // namespace MetaEventRetention
