#pragma once

#include "io/ProjectXml.h"
#include "model/MidiSequence.h"
#include "model/SequenceContents.h"
#include <juce_core/juce_core.h>
#include <optional>

class ProjectFileIO
{
public:
    static bool save(const MidiSequence& sequence, const ProjectXml::PluginStateSource& stateSource,
                     const juce::File& file);
    static std::optional<SequenceContents> load(const juce::File& file);
};
