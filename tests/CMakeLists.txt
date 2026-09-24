add_executable(CalliopeTests
    ${PROJECT_SOURCE_DIR}/src/engine/PlaybackProcessor.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/PlaybackSnapshot.cpp
    ${PROJECT_SOURCE_DIR}/src/model/MidiSequence.cpp
    ${PROJECT_SOURCE_DIR}/src/model/MidiTrack.cpp
)

target_sources(CalliopeTests PRIVATE
    engine/PlaybackProcessorTests.cpp
    engine/PlaybackSnapshotTests.cpp
)

target_include_directories(CalliopeTests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(CalliopeTests PRIVATE cxx_std_20)

target_link_libraries(CalliopeTests PRIVATE Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(CalliopeTests)
