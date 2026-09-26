add_executable(CalliopeTests
    ${PROJECT_SOURCE_DIR}/src/edit/ChordTrackEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/edit/KeySignatureEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/edit/TempoEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/edit/TimeSignatureEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/PlaybackProcessor.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/PlaybackSnapshot.cpp
    ${PROJECT_SOURCE_DIR}/src/model/MidiSequence.cpp
    ${PROJECT_SOURCE_DIR}/src/model/MidiTrack.cpp
    ${PROJECT_SOURCE_DIR}/src/model/TimelineMap.cpp
    ${PROJECT_SOURCE_DIR}/src/notation/ChordSymbol.cpp
    ${PROJECT_SOURCE_DIR}/src/notation/KeySignatureName.cpp
)

target_sources(CalliopeTests PRIVATE
    edit/ChordAddChangeTests.cpp
    edit/ChordAddSpanTests.cpp
    edit/ChordAddTests.cpp
    edit/ChordDeleteTests.cpp
    edit/ChordMoveTests.cpp
    edit/ChordPasteTests.cpp
    edit/ChordResizeTests.cpp
    edit/ChordStartResizeTests.cpp
    edit/KeySignatureAddChangeTests.cpp
    edit/KeySignatureMoveTests.cpp
    edit/TempoAddChangeTests.cpp
    edit/TimeSignatureAddChangeTests.cpp
    edit/TimeSignatureDeleteTests.cpp
    edit/TimeSignatureMoveTests.cpp
    edit/TimeSignaturePasteTests.cpp
    engine/PlaybackProcessorTests.cpp
    engine/PlaybackSnapshotTests.cpp
    model/BarBeatTickTests.cpp
    model/ChordChangeTests.cpp
    model/DefaultTimelineTests.cpp
    model/RouteTargetTests.cpp
    model/TickSecondsTests.cpp
    model/TimelineLookupTests.cpp
    model/TrackIdTests.cpp
    notation/ChordSymbolTests.cpp
    notation/KeySignatureNameTests.cpp
    ui/IndexSelectionTests.cpp
    ui/RangeSelectGestureTests.cpp
)

target_include_directories(CalliopeTests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(CalliopeTests PRIVATE cxx_std_20)

target_link_libraries(CalliopeTests PRIVATE Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(CalliopeTests)
