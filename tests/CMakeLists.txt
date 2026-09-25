add_executable(CalliopeTests
    ${PROJECT_SOURCE_DIR}/src/engine/PlaybackProcessor.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/PlaybackSnapshot.cpp
    ${PROJECT_SOURCE_DIR}/src/model/MidiSequence.cpp
    ${PROJECT_SOURCE_DIR}/src/model/MidiTrack.cpp
    ${PROJECT_SOURCE_DIR}/src/model/TimelineMap.cpp
    ${PROJECT_SOURCE_DIR}/src/notation/ChordSymbol.cpp
    ${PROJECT_SOURCE_DIR}/src/notation/KeySignatureName.cpp
)

target_sources(CalliopeTests PRIVATE
    engine/PlaybackProcessorTests.cpp
    engine/PlaybackSnapshotTests.cpp
    model/BarBeatTickTests.cpp
    model/ChordAddChangeTests.cpp
    model/ChordAddSpanTests.cpp
    model/ChordAddTests.cpp
    model/ChordChangeTests.cpp
    model/ChordDeleteTests.cpp
    model/ChordMoveTests.cpp
    model/ChordPasteTests.cpp
    model/ChordResizeTests.cpp
    model/ChordStartResizeTests.cpp
    model/DefaultTimelineTests.cpp
    model/KeySignatureMoveTests.cpp
    model/TempoAddChangeTests.cpp
    model/TickSecondsTests.cpp
    model/TimeSignatureAddChangeTests.cpp
    model/TimeSignatureDeleteTests.cpp
    model/TimeSignatureMoveTests.cpp
    model/TimeSignaturePasteTests.cpp
    model/TimelineLookupTests.cpp
    notation/ChordSymbolTests.cpp
    notation/KeySignatureNameTests.cpp
)

target_include_directories(CalliopeTests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(CalliopeTests PRIVATE cxx_std_20)

target_link_libraries(CalliopeTests PRIVATE Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(CalliopeTests)
