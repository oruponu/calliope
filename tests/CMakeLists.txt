juce_add_console_app(CalliopeTests)

target_sources(CalliopeTests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/audio/ClickVoice.cpp
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
    ${PROJECT_SOURCE_DIR}/src/io/project/ProjectFileIO.cpp
    ${PROJECT_SOURCE_DIR}/src/io/project/ProjectXmlReader.cpp
    ${PROJECT_SOURCE_DIR}/src/io/project/ProjectXmlWriter.cpp
    ${PROJECT_SOURCE_DIR}/src/io/project/XmlFieldReader.cpp
    ${PROJECT_SOURCE_DIR}/src/io/project/XmlNumberText.cpp
    ${PROJECT_SOURCE_DIR}/src/io/smf/MetaText.cpp
    ${PROJECT_SOURCE_DIR}/src/io/smf/MidiFileIO.cpp
    ${PROJECT_SOURCE_DIR}/src/plugin/PluginSyncPlan.cpp
    ${PROJECT_SOURCE_DIR}/src/session/EditableNotes.cpp
    ${PROJECT_SOURCE_DIR}/src/session/LayeredNoteEdits.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/eventlist/EventListItems.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/eventlist/MetaEventDescription.cpp
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
    io/project/ProjectFileIOTests.cpp
    io/project/ProjectXmlReadTests.cpp
    io/project/ProjectXmlRoundTripTests.cpp
    io/project/XmlNumberTextTests.cpp
    io/smf/MidiFileLoadTests.cpp
    io/smf/MidiFileRoundTripTests.cpp
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
    ui/eventlist/EventListItemsTests.cpp
    ui/eventlist/MetaEventDescriptionTests.cpp
    ui/eventlist/RowSelectionTests.cpp
    ui/pianoroll/ControllerLaneScaleTests.cpp
    ui/pianoroll/DragValueTipTests.cpp
    ui/pianoroll/EditClipboardTests.cpp
    ui/pianoroll/ProgramBrowserStateTests.cpp
    ui/pianoroll/TimelineGeometryTests.cpp
    ui/pianoroll/VelocityBarGrabTests.cpp
    ui/pianoroll/VelocityBarSelectionTests.cpp
    ui/pianoroll/strips/IndexSelectionTests.cpp
    ui/pianoroll/strips/RangeSelectGestureTests.cpp
    undo/SavePointTrackerTests.cpp
    undo/UndoHistoryTests.cpp
    undo/event/RecordTakeActionTests.cpp
    undo/event/ReplaceEventsActionTests.cpp
    undo/track/TrackActionTests.cpp
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
    CalliopeEdit
    Catch2::Catch2WithMain
    juce::juce_audio_basics
    juce::juce_data_structures
)

if(NOT WIN32)
    target_link_libraries(CalliopeTests PRIVATE Iconv::Iconv)
endif()

include(Catch)
catch_discover_tests(CalliopeTests)

add_dependencies(CalliopeTests CalliopeIncludeDirections)

set(INCLUDE_DIRECTIONS_SCRIPT ${PROJECT_SOURCE_DIR}/cmake/CheckIncludeDirections.cmake)
set(INCLUDE_DIRECTIONS_FIXTURES ${CMAKE_CURRENT_SOURCE_DIR}/include-directions)

# A fixture passes when the check reports expectedOutput, or succeeds when expectedOutput is empty.
function(add_include_directions_fixture name expectedOutput)
    add_test(NAME include-directions-${name}
        COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${INCLUDE_DIRECTIONS_FIXTURES}/${name}/src
            -DRULES=${INCLUDE_DIRECTIONS_FIXTURES}/rules.cmake -P ${INCLUDE_DIRECTIONS_SCRIPT})
    if(expectedOutput)
        set_tests_properties(include-directions-${name} PROPERTIES PASS_REGULAR_EXPRESSION "${expectedOutput}")
    endif()
endfunction()

add_include_directions_fixture(allowed "")
add_include_directions_fixture(layer-violation "low/a/A\\.h includes \"high/H\\.h\" \\(low/a -> high\\)")
add_include_directions_fixture(folder-violation "low/a/A\\.mm includes \"low/b/B\\.h\" \\(low/a -> low/b\\)")
add_include_directions_fixture(angle-violation "low/a/A\\.h includes \"high/H\\.h\" \\(low/a -> high\\)")
add_include_directions_fixture(relative-violation "low/L\\.cpp includes \"a/A\\.h\" \\(relative to the including file\\)")
add_include_directions_fixture(dot-relative-violation
    "low/b/B\\.h includes \"\\.\\./a/A\\.h\" \\(relative to the including file\\)")
add_include_directions_fixture(strict-root-file "file outside every layer: Root\\.h")
add_include_directions_fixture(strict-unruled-layer "layer without a rule: other")
add_include_directions_fixture(strict-unruled-folder "folder without a rule: low/c")
