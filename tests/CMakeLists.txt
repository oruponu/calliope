juce_add_console_app(CalliopeTests)

target_sources(CalliopeTests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/edit/ChordTrackEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/edit/KeySignatureEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/edit/NoteEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/edit/TempoEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/edit/TimeSignatureEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/PlaybackProcessor.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/PlaybackSnapshot.cpp
    ${PROJECT_SOURCE_DIR}/src/io/MidiFileIO.cpp
    ${PROJECT_SOURCE_DIR}/src/io/XmlNumberText.cpp
    ${PROJECT_SOURCE_DIR}/src/model/MidiSequence.cpp
    ${PROJECT_SOURCE_DIR}/src/model/MidiTrack.cpp
    ${PROJECT_SOURCE_DIR}/src/model/SequenceContentsRules.cpp
    ${PROJECT_SOURCE_DIR}/src/model/TimelineMap.cpp
    ${PROJECT_SOURCE_DIR}/src/notation/ChordSymbol.cpp
    ${PROJECT_SOURCE_DIR}/src/notation/KeySignatureName.cpp
    ${PROJECT_SOURCE_DIR}/src/plugin/PluginSyncPlan.cpp
    ${PROJECT_SOURCE_DIR}/src/undo/UndoHistory.cpp
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
    edit/KeySignatureDeleteTests.cpp
    edit/KeySignatureMoveTests.cpp
    edit/KeySignaturePasteTests.cpp
    edit/NoteClampShiftTests.cpp
    edit/NoteEndResizeTests.cpp
    edit/NoteShiftTests.cpp
    edit/NoteStartResizeTests.cpp
    edit/TempoAddChangeTests.cpp
    edit/TempoDeleteTests.cpp
    edit/TempoMoveTests.cpp
    edit/TempoPasteTests.cpp
    edit/TimeSignatureAddChangeTests.cpp
    edit/TimeSignatureDeleteTests.cpp
    edit/TimeSignatureMoveTests.cpp
    edit/TimeSignaturePasteTests.cpp
    engine/PlaybackProcessorTests.cpp
    engine/PlaybackSnapshotTests.cpp
    io/MidiFileLoadTests.cpp
    io/MidiFileRoundTripTests.cpp
    io/XmlNumberTextTests.cpp
    model/BarBeatTickTests.cpp
    model/ChordChangeTests.cpp
    model/DefaultTimelineTests.cpp
    model/MidiSequenceNotificationTests.cpp
    model/RouteTargetTests.cpp
    model/SequenceContentsRulesTests.cpp
    model/TickSecondsTests.cpp
    model/TimelineLookupTests.cpp
    model/TrackIdTests.cpp
    notation/ChordSymbolTests.cpp
    notation/KeySignatureNameTests.cpp
    plugin/PluginSyncPlanTests.cpp
    plugin/RetiredStateStoreTests.cpp
    ui/IndexSelectionTests.cpp
    ui/RangeSelectGestureTests.cpp
    ui/TimelineGeometryTests.cpp
    undo/SavePointTrackerTests.cpp
    undo/TrackActionTests.cpp
    undo/UndoHistoryTests.cpp
)

target_include_directories(CalliopeTests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(CalliopeTests PRIVATE cxx_std_20)

target_compile_definitions(CalliopeTests PRIVATE
    JUCE_USE_CURL=0
)

target_link_libraries(CalliopeTests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_audio_basics
    juce::juce_data_structures
)

if(NOT WIN32)
    target_link_libraries(CalliopeTests PRIVATE Iconv::Iconv)
endif()

include(Catch)
catch_discover_tests(CalliopeTests)
