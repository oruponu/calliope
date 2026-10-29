#pragma once

// Note off and on, polyphonic pressure, control change, program change, channel pressure and pitch bend.
inline bool isChannelVoiceStatus(int status)
{
    return status >= 0x80 && status <= 0xEF;
}
