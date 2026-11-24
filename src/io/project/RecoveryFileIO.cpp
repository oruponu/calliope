#include "io/project/RecoveryFileIO.h"
#include "io/project/ProjectXmlNames.h"
#include <utility>

bool RecoveryFileIO::save(const MidiSequence& sequence, const ProjectXml::PluginStateSource& stateSource,
                          const juce::File& originalFile, const juce::File& file)
{
    const auto root = ProjectXml::write(sequence, stateSource);
    root->setAttribute(ProjectXmlNames::recoveredFromAttr, originalFile.getFullPathName());
    if (!file.getParentDirectory().createDirectory())
        return false;
    return root->writeTo(file);
}

std::optional<RecoveredProject> RecoveryFileIO::load(const juce::File& file)
{
    const auto root = juce::parseXML(file);
    if (root == nullptr)
        return std::nullopt;
    auto contents = ProjectXml::read(*root);
    if (!contents)
        return std::nullopt;
    const auto path = root->getStringAttribute(ProjectXmlNames::recoveredFromAttr);
    return RecoveredProject{std::move(*contents), path.isEmpty() ? juce::File{} : juce::File(path)};
}
