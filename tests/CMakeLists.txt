juce_add_console_app(CalliopeTests)

target_sources(CalliopeTests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/audio/ClickVoice.cpp
    ${PROJECT_SOURCE_DIR}/src/edit/controller/ControllerPointEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/edit/controller/ProgramChangeEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/edit/event/NoteEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/edit/event/NoteQuantize.cpp
    ${PROJECT_SOURCE_DIR}/src/edit/harmony/ChordTrackEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/edit/harmony/KeySignatureEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/edit/timeline/TempoEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/edit/timeline/TimeSignatureEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/edit/track/ExclusiveSolo.cpp
    ${PROJECT_SOURCE_DIR}/src/edit/track/TrackDuplication.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/chase/Chase.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/chase/ChaseScan.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/chase/ChaseTracker.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/metronome/CountIn.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/metronome/MetronomeClicks.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/playback/PlaybackProcessor.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/playback/PlaybackSnapshot.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/recording/InputQuantize.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/recording/RecordingInbox.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/recording/TakeBuilder.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/thru/ThruRouter.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/thru/ThruTarget.cpp
    ${PROJECT_SOURCE_DIR}/src/engine/transport/PlaybackClock.cpp
    ${PROJECT_SOURCE_DIR}/src/io/MetaText.cpp
    ${PROJECT_SOURCE_DIR}/src/io/MidiFileIO.cpp
    ${PROJECT_SOURCE_DIR}/src/io/ProjectFileIO.cpp
    ${PROJECT_SOURCE_DIR}/src/io/ProjectXmlReader.cpp
    ${PROJECT_SOURCE_DIR}/src/io/ProjectXmlWriter.cpp
    ${PROJECT_SOURCE_DIR}/src/io/XmlFieldReader.cpp
    ${PROJECT_SOURCE_DIR}/src/io/XmlNumberText.cpp
    ${PROJECT_SOURCE_DIR}/src/model/MetaEventRetention.cpp
    ${PROJECT_SOURCE_DIR}/src/model/MidiSequence.cpp
    ${PROJECT_SOURCE_DIR}/src/model/SequenceContentsRules.cpp
    ${PROJECT_SOURCE_DIR}/src/model/TrackOrder.cpp
    ${PROJECT_SOURCE_DIR}/src/model/controller/ControllerLineValue.cpp
    ${PROJECT_SOURCE_DIR}/src/model/controller/ControllerPointConversion.cpp
    ${PROJECT_SOURCE_DIR}/src/model/controller/ControllerRamps.cpp
    ${PROJECT_SOURCE_DIR}/src/model/grid/TimelineGrid.cpp
    ${PROJECT_SOURCE_DIR}/src/model/harmony/ChordChangeList.cpp
    ${PROJECT_SOURCE_DIR}/src/model/harmony/ChordValue.cpp
    ${PROJECT_SOURCE_DIR}/src/model/harmony/KeySignatureChangeList.cpp
    ${PROJECT_SOURCE_DIR}/src/model/harmony/KeySignatureValue.cpp
    ${PROJECT_SOURCE_DIR}/src/model/notation/ChordSymbol.cpp
    ${PROJECT_SOURCE_DIR}/src/model/notation/GmPrograms.cpp
    ${PROJECT_SOURCE_DIR}/src/model/notation/KeySignatureName.cpp
    ${PROJECT_SOURCE_DIR}/src/model/notation/SysExHex.cpp
    ${PROJECT_SOURCE_DIR}/src/model/timeline/TempoChangeList.cpp
    ${PROJECT_SOURCE_DIR}/src/model/timeline/TimeSignatureChangeList.cpp
    ${PROJECT_SOURCE_DIR}/src/model/timeline/TimelineMap.cpp
    ${PROJECT_SOURCE_DIR}/src/model/track/MidiTrack.cpp
    ${PROJECT_SOURCE_DIR}/src/plugin/PluginSyncPlan.cpp
    ${PROJECT_SOURCE_DIR}/src/session/EditableNotes.cpp
    ${PROJECT_SOURCE_DIR}/src/session/LayeredNoteEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/EventListItems.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/MetaEventDescription.cpp
    ${PROJECT_SOURCE_DIR}/src/undo/UndoHistory.cpp
)

target_sources(CalliopeTests PRIVATE
    audio/ClickVoiceTests.cpp
    audio/MetronomeVolumeTests.cpp
    edit/controller/ControllerPointAddTests.cpp
    edit/controller/ControllerPointClipboardTests.cpp
    edit/controller/ControllerPointDeleteTests.cpp
    edit/controller/ControllerPointMoveTests.cpp
    edit/controller/ControllerPointSelectAllTests.cpp
    edit/controller/ControllerPointValueTests.cpp
    edit/controller/ProgramChangeEditsTests.cpp
    edit/controller/ProgramChangeGroupTests.cpp
    edit/controller/ProgramChangePointTests.cpp
    edit/event/NoteAdjacentLineTests.cpp
    edit/event/NoteClampShiftTests.cpp
    edit/event/NoteEndResizeTests.cpp
    edit/event/NoteFieldEditTests.cpp
    edit/event/NoteQuantizeTests.cpp
    edit/event/NoteShiftTests.cpp
    edit/event/NoteStartResizeTests.cpp
    edit/harmony/ChordAddSpanTests.cpp
    edit/harmony/ChordAddTests.cpp
    edit/harmony/ChordDeleteTests.cpp
    edit/harmony/ChordMoveTests.cpp
    edit/harmony/ChordPasteTests.cpp
    edit/harmony/ChordResizeTests.cpp
    edit/harmony/ChordStartResizeTests.cpp
    edit/harmony/KeySignatureDeleteTests.cpp
    edit/harmony/KeySignatureMoveTests.cpp
    edit/harmony/KeySignaturePasteTests.cpp
    edit/timeline/TempoDeleteTests.cpp
    edit/timeline/TempoMoveTests.cpp
    edit/timeline/TempoPasteTests.cpp
    edit/timeline/TimeSignatureDeleteTests.cpp
    edit/timeline/TimeSignatureMoveTests.cpp
    edit/timeline/TimeSignaturePasteTests.cpp
    edit/track/ExclusiveSoloTests.cpp
    edit/track/NewTrackTests.cpp
    edit/track/TrackDuplicationTests.cpp
    engine/chase/ChaseTests.cpp
    engine/metronome/CountInBarsTests.cpp
    engine/metronome/CountInTests.cpp
    engine/metronome/MetronomeClicksTests.cpp
    engine/playback/PlaybackProcessorTests.cpp
    engine/playback/PlaybackSnapshotTests.cpp
    engine/recording/InputQuantizeTests.cpp
    engine/recording/RecordingInboxTests.cpp
    engine/recording/TakeBuilderTests.cpp
    engine/thru/ThruRouterTests.cpp
    engine/thru/ThruTargetTests.cpp
    engine/transport/PlaybackClockTests.cpp
    io/MidiFileLoadTests.cpp
    io/MidiFileRoundTripTests.cpp
    io/ProjectFileIOTests.cpp
    io/ProjectXmlReadTests.cpp
    io/ProjectXmlRoundTripTests.cpp
    io/XmlNumberTextTests.cpp
    model/DefaultTimelineTests.cpp
    model/MidiSequenceNotificationTests.cpp
    model/RouteTargetTests.cpp
    model/SequenceContentsRulesTests.cpp
    model/TimelineLookupTests.cpp
    model/TrackIdTests.cpp
    model/TrackOrderTests.cpp
    model/controller/ControllerKindTests.cpp
    model/controller/ControllerLineValueTests.cpp
    model/controller/ControllerPointConversionTests.cpp
    model/controller/ControllerRampsTests.cpp
    model/event/ChannelVoiceStatusTests.cpp
    model/grid/GridDivisionTests.cpp
    model/grid/TimelineGridTests.cpp
    model/harmony/ChordChangeListTests.cpp
    model/harmony/ChordChangeTests.cpp
    model/harmony/ChordValueTests.cpp
    model/harmony/KeySignatureChangeListTests.cpp
    model/harmony/KeySignatureValueTests.cpp
    model/notation/ChordSymbolTests.cpp
    model/notation/GmProgramsTests.cpp
    model/notation/KeySignatureNameTests.cpp
    model/notation/SysExHexTests.cpp
    model/timeline/BarBeatTickTests.cpp
    model/timeline/TempoChangeListTests.cpp
    model/timeline/TickSecondsTests.cpp
    model/timeline/TimeSignatureChangeListTests.cpp
    plugin/PluginSyncPlanTests.cpp
    plugin/RetiredStateStoreTests.cpp
    session/EditableNotesTests.cpp
    session/LayeredNoteEditsTests.cpp
    ui/ControllerLaneScaleTests.cpp
    ui/DragValueTipTests.cpp
    ui/EditClipboardTests.cpp
    ui/EventListItemsTests.cpp
    ui/IndexSelectionTests.cpp
    ui/MetaEventDescriptionTests.cpp
    ui/ProgramBrowserStateTests.cpp
    ui/RangeSelectGestureTests.cpp
    ui/RowSelectionTests.cpp
    ui/TimelineGeometryTests.cpp
    ui/VelocityBarGrabTests.cpp
    ui/VelocityBarSelectionTests.cpp
    undo/RecordTakeActionTests.cpp
    undo/ReplaceEventsActionTests.cpp
    undo/SavePointTrackerTests.cpp
    undo/TrackActionTests.cpp
    undo/UndoHistoryTests.cpp
)

target_include_directories(CalliopeTests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(CalliopeTests PRIVATE cxx_std_23)

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

set(INCLUDE_DIRECTIONS_SCRIPT ${PROJECT_SOURCE_DIR}/cmake/CheckIncludeDirections.cmake)
set(INCLUDE_DIRECTIONS_FIXTURES ${CMAKE_CURRENT_SOURCE_DIR}/include-directions)

add_test(NAME include-directions
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${PROJECT_SOURCE_DIR}/src
        -DRULES=${PROJECT_SOURCE_DIR}/cmake/IncludeDirections.cmake -P ${INCLUDE_DIRECTIONS_SCRIPT})
add_test(NAME include-directions-allowed
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${INCLUDE_DIRECTIONS_FIXTURES}/allowed/src
        -DRULES=${INCLUDE_DIRECTIONS_FIXTURES}/rules.cmake -P ${INCLUDE_DIRECTIONS_SCRIPT})
add_test(NAME include-directions-layer-violation
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${INCLUDE_DIRECTIONS_FIXTURES}/layer-violation/src
        -DRULES=${INCLUDE_DIRECTIONS_FIXTURES}/rules.cmake -P ${INCLUDE_DIRECTIONS_SCRIPT})
add_test(NAME include-directions-folder-violation
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${INCLUDE_DIRECTIONS_FIXTURES}/folder-violation/src
        -DRULES=${INCLUDE_DIRECTIONS_FIXTURES}/rules.cmake -P ${INCLUDE_DIRECTIONS_SCRIPT})
set_tests_properties(include-directions-layer-violation PROPERTIES
    PASS_REGULAR_EXPRESSION "low/a/A\\.h includes \"high/H\\.h\" \\(low/a -> high\\)")
set_tests_properties(include-directions-folder-violation PROPERTIES
    PASS_REGULAR_EXPRESSION "low/a/A\\.mm includes \"low/b/B\\.h\" \\(low/a -> low/b\\)")
