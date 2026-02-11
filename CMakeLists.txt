cmake_minimum_required(VERSION 3.22)
project(Calliope VERSION 0.1.0)

add_subdirectory(JUCE)

juce_add_gui_app(Calliope
    PRODUCT_NAME "Calliope"
)

target_sources(Calliope PRIVATE
    src/Main.cpp
    src/MainComponent.cpp
    src/model/MidiTrack.cpp
    src/model/MidiSequence.cpp
    src/engine/PlaybackEngine.cpp
)

target_compile_definitions(Calliope PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(Calliope PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_utils
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
