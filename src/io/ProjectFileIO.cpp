#include "io/ProjectFileIO.h"

bool ProjectFileIO::save(const MidiSequence& sequence, const ProjectXml::PluginStateSource& stateSource,
                         const juce::File& file)
{
    // XmlElement::writeTo goes through a temporary file, so a failed write leaves the existing file untouched.
    return ProjectXml::write(sequence, stateSource)->writeTo(file);
}

std::optional<SequenceContents> ProjectFileIO::load(const juce::File& file)
{
    const auto root = juce::parseXML(file);
    if (root == nullptr)
        return std::nullopt;
    return ProjectXml::read(*root);
}
