#include "io/ProjectXml.h"
#include "io/ProjectXmlNames.h"
#include "io/XmlFieldReader.h"
#include "io/XmlNumberText.h"
#include "model/SequenceContentsRules.h"
#include <cstdint>
#include <set>
#include <utility>

namespace
{
namespace names = ProjectXmlNames;

const juce::XmlElement* requireChild(const juce::XmlElement& parent, const char* tag, XmlFieldReader& fields)
{
    const auto* child = parent.getChildByName(tag);
    fields.require(child != nullptr);
    return child;
}

TimelineMap readTimeline(const juce::XmlElement& element, XmlFieldReader& fields)
{
    TimelineMap timeline;
    timeline.setTicksPerQuarterNote(fields.integer(element, names::ppqAttr));

    std::vector<TempoChange> tempos;
    for (const auto* tempo : element.getChildWithTagNameIterator(names::tempoTag))
        tempos.push_back({fields.integer(*tempo, names::tickAttr), fields.finiteDouble(*tempo, names::bpmAttr)});

    std::vector<TimeSignatureChange> signatures;
    for (const auto* signature : element.getChildWithTagNameIterator(names::timeSignatureTag))
        signatures.push_back({fields.integer(*signature, names::tickAttr),
                              fields.integer(*signature, names::numeratorAttr),
                              fields.integer(*signature, names::denominatorAttr)});

    timeline.setTempoChanges(std::move(tempos));
    timeline.setTimeSignatureChanges(std::move(signatures));
    return timeline;
}

std::vector<KeySignatureChange> readKeySignatures(const juce::XmlElement& element, XmlFieldReader& fields)
{
    std::vector<KeySignatureChange> changes;
    for (const auto* key : element.getChildWithTagNameIterator(names::keySignatureTag))
        changes.push_back({fields.integer(*key, names::tickAttr), fields.integer(*key, names::sharpsOrFlatsAttr),
                           fields.flag(*key, names::minorAttr)});
    return changes;
}

std::vector<ChordChange> readChords(const juce::XmlElement& element, XmlFieldReader& fields)
{
    std::vector<ChordChange> changes;
    for (const auto* chord : element.getChildWithTagNameIterator(names::chordTag))
        changes.push_back({fields.integer(*chord, names::tickAttr), fields.integer(*chord, names::rootAttr),
                           fields.integer(*chord, names::typeAttr), fields.integer(*chord, names::bassRootAttr),
                           fields.integer(*chord, names::bassTypeAttr)});
    return changes;
}

std::shared_ptr<const PluginAssignment> readPlugin(const juce::XmlElement& element, XmlFieldReader& fields)
{
    PluginAssignment assignment;

    const juce::XmlElement* description = nullptr;
    for (const auto* child : element.getChildIterator())
    {
        if (!child->isTextElement() && !child->hasTagName(names::stateTag))
        {
            description = child;
            break;
        }
    }
    fields.require(description != nullptr);
    // PluginAssignmentCodec::toXml writes descriptions in this same form, so they come back unchanged.
    if (description != nullptr)
        assignment.descriptionXml =
            description->toString(juce::XmlElement::TextFormat().singleLine().withoutHeader()).toStdString();

    if (const auto* state = element.getChildByName(names::stateTag))
    {
        juce::MemoryOutputStream decoded;
        fields.require(juce::Base64::convertFromBase64(decoded, state->getAllSubText().trim()));
        const auto* data = static_cast<const std::byte*>(decoded.getData());
        assignment.state.assign(data, data + decoded.getDataSize());
    }

    return std::make_shared<const PluginAssignment>(std::move(assignment));
}

TrackId readTrackId(const juce::XmlElement& element, const char* name, XmlFieldReader& fields)
{
    const int value = fields.integer(element, name);
    fields.require(value > 0);
    return TrackId{static_cast<std::uint32_t>(value > 0 ? value : 0)};
}

MidiTrack readTrack(const juce::XmlElement& element, XmlFieldReader& fields)
{
    MidiTrack track(readTrackId(element, names::idAttr, fields));
    track.setName(fields.text(element, names::nameAttr));
    track.setChannel(fields.integer(element, names::channelAttr));
    track.setMuted(fields.flag(element, names::mutedAttr));
    track.setSolo(fields.flag(element, names::soloAttr));
    track.setOutputDestination(fields.choice(element, names::outputAttr, names::outputs));
    if (element.hasAttribute(names::routeTargetAttr))
        track.setRouteTarget(readTrackId(element, names::routeTargetAttr, fields));
    if (const auto* plugin = element.getChildByName(names::pluginTag))
        track.setPluginAssignment(readPlugin(*plugin, fields));

    if (const auto* notes = requireChild(element, names::notesTag, fields))
        for (const auto* note : notes->getChildWithTagNameIterator(names::noteTag))
            track.addNote({fields.integer(*note, names::keyAttr), fields.integer(*note, names::velocityAttr),
                           fields.integer(*note, names::tickAttr), fields.integer(*note, names::durationAttr)});

    if (const auto* events = requireChild(element, names::eventsTag, fields))
        for (const auto* event : events->getChildWithTagNameIterator(names::eventTag))
            track.addEvent({fields.choice(*event, names::typeAttr, names::eventTypes),
                            fields.integer(*event, names::tickAttr), fields.integer(*event, names::data1Attr),
                            fields.integer(*event, names::data2Attr)});

    return track;
}

std::vector<MidiTrack> readTracks(const juce::XmlElement& element, XmlFieldReader& fields)
{
    std::vector<MidiTrack> tracks;
    std::set<TrackId> ids;
    for (const auto* track : element.getChildWithTagNameIterator(names::trackTag))
    {
        tracks.push_back(readTrack(*track, fields));
        fields.require(ids.insert(tracks.back().getId()).second);
    }
    return tracks;
}
} // namespace

namespace ProjectXml
{
std::optional<SequenceContents> read(const juce::XmlElement& root)
{
    if (!root.hasTagName(names::projectTag) ||
        XmlNumberText::parseInt(root.getStringAttribute(names::versionAttr).toStdString()) != names::currentVersion)
        return std::nullopt;

    XmlFieldReader fields;
    SequenceContents contents;
    if (const auto* timeline = requireChild(root, names::timelineTag, fields))
        contents.timeline = readTimeline(*timeline, fields);
    if (const auto* keys = requireChild(root, names::keySignaturesTag, fields))
        contents.keySignatureChanges = readKeySignatures(*keys, fields);
    if (const auto* chords = requireChild(root, names::chordsTag, fields))
        contents.chordChanges = readChords(*chords, fields);
    if (const auto* tracks = requireChild(root, names::tracksTag, fields))
        contents.tracks = readTracks(*tracks, fields);

    if (!fields.ok() || !SequenceContentsRules::accepts(contents))
        return std::nullopt;
    return contents;
}
} // namespace ProjectXml
