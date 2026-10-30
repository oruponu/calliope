cmake_minimum_required(VERSION 3.22)
project(Calliope VERSION 0.1.0)

if(MSVC)
    add_compile_options(/utf-8)
endif()

add_subdirectory(JUCE)

add_library(CalliopeModel STATIC
    src/model/MetaEventRetention.cpp
    src/model/MidiSequence.cpp
    src/model/SequenceContentsRules.cpp
    src/model/TrackOrder.cpp
    src/model/controller/ControllerLineValue.cpp
    src/model/controller/ControllerPointConversion.cpp
    src/model/controller/ControllerRamps.cpp
    src/model/grid/TimelineGrid.cpp
    src/model/harmony/ChordChangeList.cpp
    src/model/harmony/ChordValue.cpp
    src/model/harmony/KeySignatureChangeList.cpp
    src/model/harmony/KeySignatureValue.cpp
    src/model/notation/ChordSymbol.cpp
    src/model/notation/GmPrograms.cpp
    src/model/notation/KeySignatureName.cpp
    src/model/notation/NoteName.cpp
    src/model/notation/SysExHex.cpp
    src/model/timeline/TempoChangeList.cpp
    src/model/timeline/TimeSignatureChangeList.cpp
    src/model/timeline/TimelineMap.cpp
    src/model/track/MidiTrack.cpp
)
target_include_directories(CalliopeModel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(CalliopeModel PUBLIC cxx_std_23)
target_link_libraries(CalliopeModel PRIVATE juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

add_library(CalliopeEdit STATIC
    src/edit/controller/ChannelEventEdits.cpp
    src/edit/controller/ControllerPointEdits.cpp
    src/edit/controller/ProgramChangeEdits.cpp
    src/edit/event/NoteEdits.cpp
    src/edit/event/NoteQuantize.cpp
    src/edit/harmony/ChordTrackEdits.cpp
    src/edit/harmony/KeySignatureEdits.cpp
    src/edit/timeline/TempoEdits.cpp
    src/edit/timeline/TimeSignatureEdits.cpp
    src/edit/track/ExclusiveSolo.cpp
    src/edit/track/TrackDuplication.cpp
)
target_link_libraries(CalliopeEdit
    PUBLIC CalliopeModel
    PRIVATE juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

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
    src/app/Main.cpp
    src/app/MainComponent.cpp
    src/audio/ClickVoice.cpp
    src/audio/Metronome.cpp
    src/audio/MetronomeProcessor.cpp
    src/device/MidiDeviceOutput.cpp
    src/device/MidiInputReconnector.cpp
    src/device/MidiThru.cpp
    src/device/UserDisabledMidiInputs.cpp
    src/document/Document.cpp
    src/engine/PlaybackEngine.cpp
    src/engine/chase/Chase.cpp
    src/engine/chase/ChaseScan.cpp
    src/engine/chase/ChaseTracker.cpp
    src/engine/metronome/CountIn.cpp
    src/engine/metronome/MetronomeClicks.cpp
    src/engine/playback/PlaybackProcessor.cpp
    src/engine/playback/PlaybackSnapshot.cpp
    src/engine/recording/InputQuantize.cpp
    src/engine/recording/RecordingInbox.cpp
    src/engine/recording/TakeBuilder.cpp
    src/engine/thru/ThruRouter.cpp
    src/engine/thru/ThruTarget.cpp
    src/engine/transport/PlaybackClock.cpp
    src/io/project/ProjectFileIO.cpp
    src/io/project/ProjectXmlReader.cpp
    src/io/project/ProjectXmlWriter.cpp
    src/io/project/XmlFieldReader.cpp
    src/io/project/XmlNumberText.cpp
    src/io/smf/MetaText.cpp
    src/io/smf/MidiFileIO.cpp
    src/plugin/PluginAssignmentCodec.cpp
    src/plugin/PluginStateChangeWatcher.cpp
    src/plugin/PluginSyncPlan.cpp
    src/plugin/VstPluginHost.cpp
    src/session/EditableNotes.cpp
    src/session/LayeredNoteEdits.cpp
    src/session/MidiThruSync.cpp
    src/session/PlaybackSync.cpp
    src/session/Recorder.cpp
    src/ui/commands/AppCommands.cpp
    src/ui/eventlist/EditableRows.cpp
    src/ui/eventlist/EventListComponent.cpp
    src/ui/eventlist/EventListItems.cpp
    src/ui/eventlist/EventListPosition.cpp
    src/ui/eventlist/MetaEventDescription.cpp
    src/ui/eventlist/PositionEditor.cpp
    src/ui/menu/MainMenuModel.cpp
    src/ui/pianoroll/ControllerLaneComponent.cpp
    src/ui/pianoroll/ControllerValueEditor.cpp
    src/ui/pianoroll/DragValueTipPainter.cpp
    src/ui/pianoroll/PianoRollComponent.cpp
    src/ui/pianoroll/ProgramBrowser.cpp
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
    src/ui/tracklist/TrackListComponent.cpp
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
    CalliopeEdit
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

add_custom_target(CalliopeIncludeDirections
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/src
        -DRULES=${CMAKE_CURRENT_SOURCE_DIR}/cmake/IncludeDirections.cmake
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CheckIncludeDirections.cmake
    COMMENT "Checking include directions"
    VERBATIM
)
add_dependencies(CalliopeModel CalliopeIncludeDirections)
add_dependencies(CalliopeEdit CalliopeIncludeDirections)

# Compiles each model and edit header on its own without JUCE, so that a header no library source includes cannot
# reach JUCE either.
file(GLOB_RECURSE calliopeLibraryHeaders CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/edit/*.h
)
set(headerCheckSources "")
foreach(header IN LISTS calliopeLibraryHeaders)
    file(RELATIVE_PATH relativeHeader ${CMAKE_CURRENT_SOURCE_DIR}/src ${header})
    set(headerCheckSource ${CMAKE_CURRENT_BINARY_DIR}/HeaderCheck/${relativeHeader}.cpp)
    file(CONFIGURE OUTPUT ${headerCheckSource} CONTENT "#include \"${relativeHeader}\"\n")
    list(APPEND headerCheckSources ${headerCheckSource})
endforeach()
add_library(CalliopeHeaderCheck OBJECT ${headerCheckSources})
target_link_libraries(CalliopeHeaderCheck PRIVATE
    CalliopeEdit
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_dependencies(Calliope CalliopeIncludeDirections CalliopeHeaderCheck)

option(CALLIOPE_BUILD_TESTS "Build unit tests" ON)
if(CALLIOPE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(external/Catch2)
    target_compile_features(Catch2 PUBLIC cxx_std_23)
    add_subdirectory(tests)
endif()
