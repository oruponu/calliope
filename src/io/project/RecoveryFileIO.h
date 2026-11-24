#pragma once

#include "io/project/ProjectXml.h"
#include "model/MidiSequence.h"
#include "model/SequenceContents.h"
#include <juce_core/juce_core.h>
#include <optional>

struct RecoveredProject
{
    SequenceContents contents;
    juce::File originalFile; // juce::File{} for a song that was never saved
};

class RecoveryFileIO
{
public:
    static bool save(const MidiSequence& sequence, const ProjectXml::PluginStateSource& stateSource,
                     const juce::File& originalFile, const juce::File& file);
    static std::optional<RecoveredProject> load(const juce::File& file);
};
