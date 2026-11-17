#include "ui/time/TimeEditErrorText.h"

juce::String timeEditErrorText(TimeEditError error)
{
    switch (error)
    {
    case TimeEditError::EmptyRange:
        return "End must be after Start.";
    case TimeEditError::TickOverflow:
        return "The song would become too long.";
    case TimeEditError::TimeSignatureOffBar:
        return "A time signature change would fall in the middle of a bar.";
    }
    return {};
}
