#pragma once

#include "model/MidiSequence.h"
#include "model/SequenceContents.h"
#include "model/TrackId.h"
#include <cstddef>
#include <functional>
#include <juce_core/juce_core.h>
#include <memory>
#include <optional>
#include <vector>

namespace ProjectXml
{
// Returns the live state of the plugin on a track, or nullopt to write the state stored in the model.
using PluginStateSource = std::function<std::optional<std::vector<std::byte>>(TrackId)>;

std::unique_ptr<juce::XmlElement> write(const MidiSequence& sequence, const PluginStateSource& stateSource);
// Track ids in the result are the numbers written in the file; MidiSequence::replaceContents reissues them.
std::optional<SequenceContents> read(const juce::XmlElement& root);
} // namespace ProjectXml
