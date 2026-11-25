#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

class Document;
class Recorder;
class VstPluginHost;

class FileOperations
{
public:
    // onSongReplaced: called after New, Open, Import MIDI and a dropped file have replaced the song.
    FileOperations(Document& document, Recorder& recorder, VstPluginHost& pluginHost,
                   std::function<void()> onSongReplaced);

    void newFile();
    void open();
    void save();
    void saveAs();
    void importMidi();
    void exportMidi();

    static bool canOpenAny(const juce::StringArray& files);
    void openDropped(const juce::StringArray& files);

    // next runs with the file operation still in progress and must call finish() when it is done, unless it
    // quits the app.
    void saveIfNeededThen(std::function<void()> next);
    void finish();
    bool isInProgress() const;

    // For a project restored other than by open(): calls onSongReplaced and shows the plugins that failed to load.
    void finishOpeningProject();

private:
    void importMidiFile(const juce::File& file);
    void showPluginLoadFailures();

    Document& document;
    Recorder& recorder;
    VstPluginHost& pluginHost;
    std::function<void()> onSongReplaced;
    bool operationInProgress = false;
    std::unique_ptr<juce::FileChooser> midiFileChooser;
    juce::ScopedMessageBox alertBox;

    JUCE_DECLARE_NON_COPYABLE(FileOperations)
};
