#include "document/RecoveryLocation.h"
#include "settings/AppProperties.h"

juce::File getRecoveryFile()
{
    return getAppProperties().getUserSettings()->getFile().getSiblingFile("Recovery").getChildFile("Recovery.calliope");
}
