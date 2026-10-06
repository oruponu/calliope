#include "io/ProjectXml.h"
#include "io/ProjectXmlNames.h"
#include "io/XmlNumberText.h"
#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

namespace
{
namespace names = ProjectXmlNames;

template <typename Enum, std::size_t N>
const char* labelOf(Enum value, const std::array<std::pair<Enum, const char*>, N>& labels)
{
    for (const auto& [candidate, label] : labels)
        if (candidate == value)
            return label;
    jassertfalse;
    return labels.front().second;
}

juce::String utf8(const std::string& text)
{
    return juce::String::fromUTF8(text.data(), static_cast<int>(text.size()));
}

int fileId(TrackId id)
{
    return static_cast<int>(std::to_underlying(id));
}

// Appending walks XmlElement's singly linked list of children, so prepend in reverse to keep writing linear.
template <typename Items, typename Fill>
void prependChildren(juce::XmlElement& parent, const Items& items, const char* tag, Fill fill)
{
    for (auto it = std::rbegin(items); it != std::rend(items); ++it)
    {
        auto child = std::make_unique<juce::XmlElement>(tag);
        fill(*child, *it);
        parent.prependChildElement(child.release());
    }
}

void writeTimeline(juce::XmlElement& root, const TimelineMap& timeline)
{
    auto* element = root.createNewChildElement(names::timelineTag);
    element->setAttribute(names::ppqAttr, timeline.getTicksPerQuarterNote());
    // Time signatures go in first so that the tempos end up before them.
    prependChildren(*element, timeline.getTimeSignatureChanges(), names::timeSignatureTag,
                    [](juce::XmlElement& signature, const TimeSignatureChange& change)
                    {
                        signature.setAttribute(names::tickAttr, change.tick);
                        signature.setAttribute(names::numeratorAttr, change.numerator);
                        signature.setAttribute(names::denominatorAttr, change.denominator);
                    });
    prependChildren(*element, timeline.getTempoChanges(), names::tempoTag,
                    [](juce::XmlElement& tempo, const TempoChange& change)
                    {
                        tempo.setAttribute(names::tickAttr, change.tick);
                        tempo.setAttribute(names::bpmAttr, juce::String(XmlNumberText::formatDouble(change.bpm)));
                    });
}

void writeKeySignatures(juce::XmlElement& root, const std::vector<KeySignatureChange>& changes)
{
    auto* element = root.createNewChildElement(names::keySignaturesTag);
    prependChildren(*element, changes, names::keySignatureTag,
                    [](juce::XmlElement& key, const KeySignatureChange& change)
                    {
                        key.setAttribute(names::tickAttr, change.tick);
                        key.setAttribute(names::sharpsOrFlatsAttr, change.sharpsOrFlats);
                        key.setAttribute(names::minorAttr, change.isMinor ? 1 : 0);
                    });
}

void writeChords(juce::XmlElement& root, const std::vector<ChordChange>& changes)
{
    auto* element = root.createNewChildElement(names::chordsTag);
    prependChildren(*element, changes, names::chordTag,
                    [](juce::XmlElement& chord, const ChordChange& change)
                    {
                        chord.setAttribute(names::tickAttr, change.tick);
                        chord.setAttribute(names::rootAttr, change.chordRoot);
                        chord.setAttribute(names::typeAttr, change.chordType);
                        chord.setAttribute(names::bassRootAttr, change.bassRoot);
                        chord.setAttribute(names::bassTypeAttr, change.bassType);
                    });
}

void writePlugin(juce::XmlElement& trackElement, const PluginAssignment& assignment,
                 const std::optional<std::vector<std::byte>>& liveState)
{
    auto description = juce::parseXML(utf8(assignment.descriptionXml));
    if (description == nullptr)
    {
        // Assignments come from PluginDescription::createXml or from a project file, so this cannot happen.
        jassertfalse;
        return;
    }

    auto* plugin = trackElement.createNewChildElement(names::pluginTag);
    plugin->addChildElement(description.release());
    const auto& state = liveState ? *liveState : assignment.state;
    if (!state.empty())
        plugin->createNewChildElement(names::stateTag)
            ->addTextElement(juce::Base64::toBase64(state.data(), state.size()));
}

void writeTrack(juce::XmlElement& element, const MidiTrack& track, const ProjectXml::PluginStateSource& stateSource)
{
    element.setAttribute(names::idAttr, fileId(track.getId()));
    element.setAttribute(names::nameAttr, utf8(track.getName()));
    element.setAttribute(names::channelAttr, track.getChannel());
    element.setAttribute(names::mutedAttr, track.isMuted() ? 1 : 0);
    element.setAttribute(names::soloAttr, track.isSolo() ? 1 : 0);
    element.setAttribute(names::outputAttr, labelOf(track.getOutputDestination(), names::outputs));
    if (const auto& target = track.getRouteTarget())
        element.setAttribute(names::routeTargetAttr, fileId(*target));
    if (const auto& assignment = track.getPluginAssignment())
        writePlugin(element, *assignment, stateSource ? stateSource(track.getId()) : std::nullopt);

    auto* notes = element.createNewChildElement(names::notesTag);
    prependChildren(*notes, track.getNotes(), names::noteTag,
                    [](juce::XmlElement& noteElement, const MidiNote& note)
                    {
                        noteElement.setAttribute(names::tickAttr, note.startTick);
                        noteElement.setAttribute(names::durationAttr, note.duration);
                        noteElement.setAttribute(names::keyAttr, note.noteNumber);
                        noteElement.setAttribute(names::velocityAttr, note.velocity);
                    });

    auto* events = element.createNewChildElement(names::eventsTag);
    prependChildren(*events, track.getEvents(), names::eventTag,
                    [](juce::XmlElement& eventElement, const MidiEvent& event)
                    {
                        eventElement.setAttribute(names::typeAttr, labelOf(event.type, names::eventTypes));
                        eventElement.setAttribute(names::tickAttr, event.tick);
                        eventElement.setAttribute(names::data1Attr, event.data1);
                        eventElement.setAttribute(names::data2Attr, event.data2);
                    });
}

void writeTracks(juce::XmlElement& root, const MidiSequence& sequence, const ProjectXml::PluginStateSource& stateSource)
{
    auto* element = root.createNewChildElement(names::tracksTag);
    for (int i = sequence.getNumTracks() - 1; i >= 0; --i)
    {
        auto track = std::make_unique<juce::XmlElement>(names::trackTag);
        writeTrack(*track, sequence.getTrack(i), stateSource);
        element->prependChildElement(track.release());
    }
}
} // namespace

namespace ProjectXml
{
std::unique_ptr<juce::XmlElement> write(const MidiSequence& sequence, const PluginStateSource& stateSource)
{
    auto root = std::make_unique<juce::XmlElement>(names::projectTag);
    root->setAttribute(names::versionAttr, names::currentVersion);
    writeTimeline(*root, sequence.getTimeline());
    writeKeySignatures(*root, sequence.getKeySignatureChanges());
    writeChords(*root, sequence.getChordChanges());
    writeTracks(*root, sequence, stateSource);
    return root;
}
} // namespace ProjectXml
