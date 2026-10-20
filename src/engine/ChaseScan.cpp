#include "engine/ChaseScan.h"
#include <utility>

namespace
{
constexpr int bankSelectMsb = 0;
constexpr int bankSelectLsb = 32;
constexpr int dataEntryMsb = 6;
constexpr int dataEntryLsb = 38;
constexpr int dataIncrement = 96;
constexpr int dataDecrement = 97;
constexpr int nrpnLsb = 98;
constexpr int nrpnMsb = 99;
constexpr int rpnLsb = 100;
constexpr int rpnMsb = 101;
constexpr int firstChannelModeMessage = 120;
constexpr int resetAllControllers = 121;

MidiEvent controlChange(int tick, int number, int value)
{
    return {.type = MidiEvent::Type::ControlChange, .tick = tick, .data1 = number, .data2 = value};
}

void appendPair(std::vector<MidiEvent>& events, int tick, const ChaseMsbLsb& value, int msbNumber, int lsbNumber)
{
    if (value.msb)
        events.push_back(controlChange(tick, msbNumber, *value.msb));
    if (value.lsb)
        events.push_back(controlChange(tick, lsbNumber, *value.lsb));
}

void appendSelection(std::vector<MidiEvent>& events, int tick, const ChaseSelection& selection)
{
    appendPair(events, tick, {selection.msb, selection.lsb}, selection.nrpn ? nrpnMsb : rpnMsb,
               selection.nrpn ? nrpnLsb : rpnLsb);
}
} // namespace

void ChaseScan::apply(const MidiEvent& event)
{
    switch (event.type)
    {
    case MidiEvent::Type::ControlChange:
        applyControlChange(event.data1, event.data2);
        break;
    case MidiEvent::Type::ProgramChange:
        program = event.data1;
        programBank = bank;
        pendingBank = {};
        break;
    case MidiEvent::Type::PitchBend:
        pitchBend = event.data1;
        break;
    case MidiEvent::Type::ChannelPressure:
        channelPressure = event.data1;
        break;
    case MidiEvent::Type::KeyPressure:
        break;
    }
}

std::vector<ChaseItem> ChaseScan::items(int tick) const
{
    using Kind = ChaseItem::Kind;
    std::vector<ChaseItem> result;
    if (program)
    {
        ChaseItem item{Kind::Program};
        appendPair(item.events, tick, programBank, bankSelectMsb, bankSelectLsb);
        item.events.push_back({.type = MidiEvent::Type::ProgramChange, .tick = tick, .data1 = *program});
        result.push_back(std::move(item));
    }
    if (pendingBank.msb || pendingBank.lsb)
    {
        ChaseItem item{Kind::PendingBank};
        appendPair(item.events, tick, pendingBank, bankSelectMsb, bankSelectLsb);
        result.push_back(std::move(item));
    }
    for (const auto& [number, value] : controllers)
        result.push_back({Kind::Controller, number, {controlChange(tick, number, value)}});
    for (const auto& [number, entry] : parameters)
    {
        ChaseItem item{Kind::Parameter, number};
        appendSelection(item.events, tick, {(number >> 14) != 0, (number >> 7) & 0x7f, number & 0x7f});
        appendPair(item.events, tick, entry, dataEntryMsb, dataEntryLsb);
        result.push_back(std::move(item));
    }
    if (selection)
    {
        ChaseItem item{Kind::Selection};
        appendSelection(item.events, tick, *selection);
        result.push_back(std::move(item));
    }
    if (pitchBend)
        result.push_back(
            {Kind::PitchBend, 0, {{.type = MidiEvent::Type::PitchBend, .tick = tick, .data1 = *pitchBend}}});
    if (channelPressure)
        result.push_back({Kind::ChannelPressure,
                          0,
                          {{.type = MidiEvent::Type::ChannelPressure, .tick = tick, .data1 = *channelPressure}}});
    return result;
}

void ChaseScan::applyControlChange(int number, int value)
{
    switch (number)
    {
    case bankSelectMsb:
        bank.msb = value;
        pendingBank.msb = value;
        return;
    case bankSelectLsb:
        bank.lsb = value;
        pendingBank.lsb = value;
        return;
    case rpnMsb:
    case rpnLsb:
    case nrpnMsb:
    case nrpnLsb:
        select(number == nrpnMsb || number == nrpnLsb, number == rpnMsb || number == nrpnMsb, value);
        return;
    case dataEntryMsb:
        if (selection && selection->namesParameter())
            parameters[selection->parameterNumber()].msb = value;
        return;
    case dataEntryLsb:
        if (selection && selection->namesParameter())
            parameters[selection->parameterNumber()].lsb = value;
        return;
    case dataIncrement:
    case dataDecrement:
        return;
    case resetAllControllers:
        resetControllers();
        return;
    default:
        if (number < firstChannelModeMessage)
            controllers[number] = value;
        return;
    }
}

void ChaseScan::select(bool nrpn, bool msbPart, int value)
{
    // Switching between RPN and NRPN leaves the other half of the new kind unknown.
    if (!selection || selection->nrpn != nrpn)
        selection = ChaseSelection{nrpn};
    (msbPart ? selection->msb : selection->lsb) = value;
}

// What GM RP-015 has Reset All Controllers reset; everything else keeps its value.
void ChaseScan::resetControllers()
{
    controllers[1] = 0;
    controllers[11] = 127;
    for (int number = 64; number <= 67; ++number)
        controllers[number] = 0;
    pitchBend = 8192;
    channelPressure = 0;
    selection = ChaseSelection{false, ChaseSelection::nullParameter, ChaseSelection::nullParameter};
}
