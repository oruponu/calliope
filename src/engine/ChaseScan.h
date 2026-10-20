#pragma once

#include "engine/ChaseItem.h"
#include "engine/ChaseMsbLsb.h"
#include "engine/ChaseSelection.h"
#include "model/MidiEvent.h"
#include <map>
#include <optional>
#include <vector>

// The state one channel is left in by the events applied to it, in time order.
class ChaseScan
{
public:
    void apply(const MidiEvent& event);
    // The state as items in the order they are sent, each event carrying tick.
    std::vector<ChaseItem> items(int tick) const;

private:
    void applyControlChange(int number, int value);
    void select(bool nrpn, bool msbPart, int value);
    void resetControllers();

    ChaseMsbLsb bank;
    std::optional<int> program;
    ChaseMsbLsb programBank;
    // Bank select sent after the last program change, waiting for the next one.
    ChaseMsbLsb pendingBank;
    std::map<int, int> controllers;
    std::optional<ChaseSelection> selection;
    // Keyed by ChaseSelection::parameterNumber, so that RPNs come before NRPNs.
    std::map<int, ChaseMsbLsb> parameters;
    std::optional<int> pitchBend;
    std::optional<int> channelPressure;
};
