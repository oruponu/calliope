#include "ui/file/FileOperations.h"
#include "document/Document.h"
#include "plugin/PluginAssignmentCodec.h"
#include "plugin/VstPluginHost.h"
#include "session/Recorder.h"
#include <utility>

namespace
{
bool isProjectFile(const juce::File& file)
{
    return file.hasFileExtension(".calliope");
}

bool isMidiFile(const juce::File& file)
{
    return file.hasFileExtension(".mid;.midi");
}
} // namespace

FileOperations::FileOperations(Document& documentRef, Recorder& recorderRef, VstPluginHost& pluginHostRef,
                               std::function<void()> onSongReplacedCallback)
    : document(documentRef), recorder(recorderRef), pluginHost(pluginHostRef),
      onSongReplaced(std::move(onSongReplacedCallback))
{
}

void FileOperations::newFile()
{
    saveIfNeededThen(
        [this]
        {
            document.newDocument();
            onSongReplaced();
            finish();
        });
}

void FileOperations::open()
{
    saveIfNeededThen(
        [this]
        {
            document.loadFromUserSpecifiedFileAsync(true,
                                                    [this](juce::Result result)
                                                    {
                                                        if (result.wasOk())
                                                            finishOpeningProject();
                                                        finish();
                                                    });
        });
}

void FileOperations::save()
{
    if (operationInProgress)
        return;
    operationInProgress = true;
    document.saveAsync(true, true, [this](juce::FileBasedDocument::SaveResult) { finish(); });
}

void FileOperations::saveAs()
{
    if (operationInProgress)
        return;
    operationInProgress = true;
    document.saveAsInteractiveAsync(true, [this](juce::FileBasedDocument::SaveResult) { finish(); });
}

void FileOperations::importMidi()
{
    saveIfNeededThen(
        [this]
        {
            midiFileChooser = std::make_unique<juce::FileChooser>("Import MIDI File", juce::File{}, "*.mid;*.midi");
            midiFileChooser->launchAsync(juce::FileBrowserComponent::openMode |
                                             juce::FileBrowserComponent::canSelectFiles,
                                         [this](const juce::FileChooser& chooser)
                                         {
                                             if (const auto file = chooser.getResult(); file != juce::File{})
                                                 importMidiFile(file);
                                             finish();
                                         });
        });
}

void FileOperations::exportMidi()
{
    if (operationInProgress)
        return;
    operationInProgress = true;

    const auto initialFile =
        document.getFile() == juce::File{} ? juce::File{} : document.getFile().withFileExtension(".mid");
    midiFileChooser = std::make_unique<juce::FileChooser>("Export MIDI File", initialFile, "*.mid");
    midiFileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                                     juce::FileBrowserComponent::warnAboutOverwriting,
                                 [this](const juce::FileChooser& chooser)
                                 {
                                     const auto file = chooser.getResult();
                                     if (file != juce::File{} && !document.exportMidi(file))
                                         alertBox = juce::AlertWindow::showScopedAsync(
                                             juce::MessageBoxOptions()
                                                 .withIconType(juce::MessageBoxIconType::WarningIcon)
                                                 .withTitle("Export MIDI")
                                                 .withMessage("The MIDI file could not be written.")
                                                 .withButton("OK", 0),
                                             nullptr);
                                     finish();
                                 });
}

bool FileOperations::canOpenAny(const juce::StringArray& files)
{
    for (auto& path : files)
        if (const juce::File file(path); isProjectFile(file) || isMidiFile(file))
            return true;
    return false;
}

void FileOperations::openDropped(const juce::StringArray& files)
{
    for (auto& path : files)
    {
        const juce::File file(path);
        if (isProjectFile(file))
        {
            saveIfNeededThen(
                [this, file]
                {
                    if (document.loadFrom(file, true).wasOk())
                        finishOpeningProject();
                    finish();
                });
            return;
        }
        if (isMidiFile(file))
        {
            saveIfNeededThen(
                [this, file]
                {
                    importMidiFile(file);
                    finish();
                });
            return;
        }
    }
}

void FileOperations::saveIfNeededThen(std::function<void()> next)
{
    if (operationInProgress)
        return;

    // Commit the recording first so the save prompt covers it.
    recorder.finish();
    operationInProgress = true;
    pluginHost.flushPendingStateChanges();
    document.saveIfNeededAndUserAgreesAsync(
        [this, next = std::move(next)](juce::FileBasedDocument::SaveResult result)
        {
            if (result == juce::FileBasedDocument::savedOk)
                next();
            else
                finish();
        });
}

void FileOperations::finish()
{
    operationInProgress = false;
}

bool FileOperations::isInProgress() const
{
    return operationInProgress;
}

void FileOperations::finishOpeningProject()
{
    onSongReplaced();
    showPluginLoadFailures();
}

void FileOperations::importMidiFile(const juce::File& file)
{
    if (!document.importMidi(file))
    {
        alertBox = juce::AlertWindow::showScopedAsync(juce::MessageBoxOptions()
                                                          .withIconType(juce::MessageBoxIconType::WarningIcon)
                                                          .withTitle("Import MIDI")
                                                          .withMessage("The file could not be read as a MIDI file.")
                                                          .withButton("OK", 0),
                                                      nullptr);
        return;
    }
    onSongReplaced();
}

void FileOperations::showPluginLoadFailures()
{
    const auto& failed = pluginHost.getFailedTracks();
    const auto& sequence = document.getSequence();
    juce::StringArray lines;
    for (int i = 0; i < sequence.getNumTracks(); ++i)
    {
        const auto& track = sequence.getTrack(i);
        const auto& assignment = track.getPluginAssignment();
        if (assignment == nullptr || !failed.contains(track.getId()))
            continue;

        const auto description = PluginAssignmentCodec::fromXml(assignment->descriptionXml);
        const auto trackName =
            track.getName().empty() ? "Track " + juce::String(i + 1) : juce::String::fromUTF8(track.getName().c_str());
        lines.add(trackName + ": " + (description ? description->name : juce::String("Unknown plugin")));
    }
    if (lines.isEmpty())
        return;

    alertBox = juce::AlertWindow::showScopedAsync(
        juce::MessageBoxOptions()
            .withIconType(juce::MessageBoxIconType::WarningIcon)
            .withTitle("Open Project")
            .withMessage("The following plugins could not be loaded:\n" + lines.joinIntoString("\n"))
            .withButton("OK", 0),
        nullptr);
}
