cmake_minimum_required(VERSION 3.22)
project(Calliope VERSION 0.1.0)

if(MSVC)
    add_compile_options(/utf-8)
endif()

add_subdirectory(JUCE)

juce_add_gui_app(Calliope
    PRODUCT_NAME "Calliope"
)

juce_add_binary_data(CalliopeData
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/NotoSans-Regular.ttf
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/NotoSans-Bold.ttf
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/Inconsolata-Regular.ttf
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/Inconsolata-Bold.ttf
)

target_sources(Calliope PRIVATE
    src/Main.cpp
    src/MainComponent.cpp
    src/audio/ClickVoice.cpp
    src/audio/Metronome.cpp
    src/audio/MetronomeProcessor.cpp
    src/audio/MidiDeviceOutput.cpp
    src/audio/MidiInputReconnector.cpp
    src/audio/UserDisabledMidiInputs.cpp
    src/audio/MidiThru.cpp
    src/audio/MidiThruSync.cpp
    src/audio/Recorder.cpp
    src/document/Document.cpp
    src/edit/ChordTrackEdits.cpp
    src/edit/ControllerPointEdits.cpp
    src/edit/EditableNotes.cpp
    src/edit/KeySignatureEdits.cpp
    src/edit/LayeredNoteEdits.cpp
    src/edit/NoteEdits.cpp
    src/edit/NoteQuantize.cpp
    src/edit/TempoEdits.cpp
    src/edit/TimeSignatureEdits.cpp
    src/engine/Chase.cpp
    src/engine/ChaseScan.cpp
    src/engine/ChaseTracker.cpp
    src/engine/CountIn.cpp
    src/engine/InputQuantize.cpp
    src/engine/MetronomeClicks.cpp
    src/engine/PlaybackClock.cpp
    src/engine/PlaybackEngine.cpp
    src/engine/PlaybackProcessor.cpp
    src/engine/PlaybackSnapshot.cpp
    src/engine/PlaybackSync.cpp
    src/engine/RecordingInbox.cpp
    src/engine/TakeBuilder.cpp
    src/engine/ThruRouter.cpp
    src/engine/ThruTarget.cpp
    src/io/MidiFileIO.cpp
    src/io/ProjectFileIO.cpp
    src/io/ProjectXmlReader.cpp
    src/io/ProjectXmlWriter.cpp
    src/io/XmlFieldReader.cpp
    src/io/XmlNumberText.cpp
    src/model/ControllerLineValue.cpp
    src/model/ControllerPointConversion.cpp
    src/model/ControllerRamps.cpp
    src/model/MidiSequence.cpp
    src/model/MidiTrack.cpp
    src/model/SequenceContentsRules.cpp
    src/model/TimelineGrid.cpp
    src/model/TimelineMap.cpp
    src/notation/ChordSymbol.cpp
    src/notation/KeySignatureName.cpp
    src/plugin/PluginAssignmentCodec.cpp
    src/plugin/PluginStateChangeWatcher.cpp
    src/plugin/PluginSyncPlan.cpp
    src/plugin/VstPluginHost.cpp
    src/ui/EventListComponent.cpp
    src/ui/EventListItems.cpp
    src/ui/TrackListComponent.cpp
    src/ui/commands/AppCommands.cpp
    src/ui/menu/MainMenuModel.cpp
    src/ui/pianoroll/ControllerLaneComponent.cpp
    src/ui/pianoroll/ControllerValueEditor.cpp
    src/ui/pianoroll/DragValueTipPainter.cpp
    src/ui/pianoroll/PianoRollComponent.cpp
    src/ui/pianoroll/strips/ChordEditor.cpp
    src/ui/pianoroll/strips/ChordStrip.cpp
    src/ui/pianoroll/strips/ChordSymbolPainter.cpp
    src/ui/pianoroll/strips/KeySignatureEditor.cpp
    src/ui/pianoroll/strips/KeySignatureStrip.cpp
    src/ui/pianoroll/strips/LoopStrip.cpp
    src/ui/pianoroll/strips/RangeBandPainter.cpp
    src/ui/pianoroll/strips/RulerStrip.cpp
    src/ui/pianoroll/strips/TempoTrackStrip.cpp
    src/ui/pianoroll/strips/TimeSignatureEditor.cpp
    src/ui/pianoroll/strips/TimeSignatureStrip.cpp
    src/ui/pianoroll/strips/TimelineStrip.cpp
    src/ui/plugin/PluginCatalogController.cpp
    src/ui/plugin/PluginEditorController.cpp
    src/ui/plugin/TrackOutputController.cpp
    src/ui/theme/LookAndFeel.cpp
    src/ui/transport/MetronomeSettingsComponent.cpp
    src/ui/transport/TransportBarComponent.cpp
    src/ui/window/ToolWindow.cpp
    src/undo/UndoHistory.cpp
)

if(WIN32)
    target_sources(Calliope PRIVATE src/ui/window/OwnedWindow_windows.cpp)
elseif(APPLE)
    target_sources(Calliope PRIVATE src/ui/window/OwnedWindow_mac.mm)
else()
    target_sources(Calliope PRIVATE src/ui/window/OwnedWindow_linux.cpp)
    find_package(X11 REQUIRED)
    target_link_libraries(Calliope PRIVATE X11::X11)
endif()

target_include_directories(Calliope PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_features(Calliope PRIVATE cxx_std_23)

target_compile_definitions(Calliope PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1
)

target_link_libraries(Calliope PRIVATE
    CalliopeData
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

if(NOT WIN32)
    find_package(Iconv REQUIRED)
    target_link_libraries(Calliope PRIVATE Iconv::Iconv)
endif()

option(CALLIOPE_BUILD_TESTS "Build unit tests" ON)
if(CALLIOPE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(external/Catch2)
    target_compile_features(Catch2 PUBLIC cxx_std_23)
    add_subdirectory(tests)
endif()
