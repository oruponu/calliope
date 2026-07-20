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
    src/audio/MidiDeviceOutput.cpp
    src/document/Document.cpp
    src/engine/PlaybackEngine.cpp
    src/engine/PlaybackProcessor.cpp
    src/engine/PlaybackSnapshot.cpp
    src/io/MidiFileIO.cpp
    src/model/MidiSequence.cpp
    src/model/MidiTrack.cpp
    src/plugin/VstPluginHost.cpp
    src/ui/EventListComponent.cpp
    src/ui/TrackListComponent.cpp
    src/ui/commands/AppCommands.cpp
    src/ui/menu/MainMenuModel.cpp
    src/ui/pianoroll/ControllerLaneComponent.cpp
    src/ui/pianoroll/PianoRollComponent.cpp
    src/ui/pianoroll/strips/LoopStrip.cpp
    src/ui/pianoroll/strips/RulerStrip.cpp
    src/ui/pianoroll/strips/TempoTrackStrip.cpp
    src/ui/pianoroll/strips/TimeSignatureEditor.cpp
    src/ui/pianoroll/strips/TimeSignatureStrip.cpp
    src/ui/pianoroll/strips/TimelineStrip.cpp
    src/ui/plugin/PluginEditorController.cpp
    src/ui/plugin/PluginManagementController.cpp
    src/ui/theme/LookAndFeel.cpp
    src/ui/transport/TransportBarComponent.cpp
)

target_include_directories(Calliope PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_features(Calliope PRIVATE cxx_std_20)

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
