add_executable(drumprog_tests
    audio/DeviceRecoveryTest.cpp
    audio/DriverChoiceTest.cpp
    engine/BackingTrackPlayerTest.cpp
    engine/ClockTest.cpp
    engine/DownmixTest.cpp
    engine/EngineKitTest.cpp
    engine/KitBuilderTest.cpp
    engine/KitDescriptionTest.cpp
    engine/KitPublisherTest.cpp
    engine/LongRunTest.cpp
    engine/MetronomeTest.cpp
    engine/PlaybackRendererTest.cpp
    engine/SampleBufferTest.cpp
    engine/SampleEngineTest.cpp
    engine/SequencerTest.cpp
    engine/SnapshotExchangeTest.cpp
    engine/SpscQueueTest.cpp
    engine/TempoMathTest.cpp
    engine/TestToneSourceTest.cpp
    engine/TriggerIndicatorsTest.cpp
    engine/VelocityCurveTest.cpp
    engine/VoicePoolTest.cpp
    engine/VoiceTest.cpp
    i18n/GermanTestLanguage.cpp
    i18n/LanguageCatalogTest.cpp
    i18n/TranslateTest.cpp
    input/EngineNoteSinksTest.cpp
    input/InputActivityTest.cpp
    input/KeyboardInputTest.cpp
    input/KeyNamesTest.cpp
    input/KeymapTest.cpp
    input/MidiInputHandlerTest.cpp
    input/ScancodeTest.cpp
    io/DefaultKitTest.cpp
    io/GlobalKitTest.cpp
    io/MidiFileReaderTest.cpp
    io/MidiFileWriterTest.cpp
    io/MidiImportTest.cpp
    io/MidiRoundTripTest.cpp
    io/PatternMidiExportTest.cpp
    io/ProjectRepositoryTest.cpp
    io/ProjectSerializerTest.cpp
    io/SettingsStoreTest.cpp
    io/SongMidiExportTest.cpp
    io/StdFileSystemTest.cpp
    model/ProjectFactoryTest.cpp
    model/ProjectTest.cpp
    model/SnapshotBuilderTest.cpp
    model/SnapshotPublisherTest.cpp
    model/SongLayoutTest.cpp
    model/SongTakeRecorderTest.cpp
    model/TakeRecorderTest.cpp
    model/TreeChangeListenerTest.cpp
    model/UuidGeneratorTest.cpp
    ui/AboutInfoTest.cpp
    ui/ActivePatternTest.cpp
    ui/ActivityLedTest.cpp
    ui/AudioStatusTest.cpp
    ui/AutosaveTest.cpp
    ui/BackingTrackPresenterTest.cpp
    ui/DocumentControllerTest.cpp
    ui/InputLedPresenterTest.cpp
    ui/KeymapPresenterTest.cpp
    ui/KeyRouterTest.cpp
    ui/KitPresenterTest.cpp
    ui/MidiExportControllerTest.cpp
    ui/MidiImportControllerTest.cpp
    ui/MixPresenterTest.cpp
    ui/PaneSizeTest.cpp
    ui/MusicalTimeTest.cpp
    ui/PatternListPresenterTest.cpp
    ui/PianoRollGeometryTest.cpp
    ui/PianoRollPresenterTest.cpp
    ui/SnapGridTest.cpp
    ui/SongTimelineGeometryTest.cpp
    ui/SongTimelinePresenterTest.cpp
    ui/TempoPresenterTest.cpp
    ui/ThemeSettingTest.cpp
    ui/TransportPresenterTest.cpp
)

# Q-04: replaces the global operator new to count allocations, which clashes with the
# sanitizers' own allocator. The regular, coverage and Windows builds run it.
if(NOT DRUMPROG_SANITIZERS)
    target_sources(drumprog_tests PRIVATE engine/RealtimeAllocationTest.cpp)
endif()

target_link_libraries(drumprog_tests PRIVATE drumprog_core GTest::gmock_main)
target_include_directories(drumprog_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# The tests run in German (i18n/GermanTestLanguage.cpp), so they also check the German translations.
target_compile_definitions(drumprog_tests PRIVATE
    DRUMPROG_LANGUAGES_DIR="${PROJECT_SOURCE_DIR}/resources/Languages")
drumprog_enable_warnings(drumprog_tests)

include(GoogleTest)
gtest_discover_tests(drumprog_tests DISCOVERY_MODE PRE_TEST)

# AP15: every language file translates every text of the program.
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    add_test(NAME TranslationsComplete
             COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/check-translations.py ${PROJECT_SOURCE_DIR})
endif()

# Performance measurements (Q-05, Q-07): timings only mean something in an optimised build without instrumentation,
# so they are not built for the coverage and sanitizer builds and skip themselves in a debug build.
if(NOT DRUMPROG_COVERAGE AND NOT DRUMPROG_SANITIZERS)
    add_executable(drumprog_benchmarks benchmarks/RenderBenchmark.cpp)
    target_link_libraries(drumprog_benchmarks PRIVATE drumprog_core GTest::gtest_main)
    target_include_directories(drumprog_benchmarks PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    drumprog_enable_warnings(drumprog_benchmarks)
    gtest_discover_tests(drumprog_benchmarks DISCOVERY_MODE PRE_TEST)

    # Q-07 paints the program's own piano roll into an image; it needs no window or display.
    if(DRUMPROG_BUILD_APP)
        add_executable(drumprog_gui_benchmarks benchmarks/PianoRollBenchmark.cpp)
        target_link_libraries(drumprog_gui_benchmarks PRIVATE drumprog_app GTest::gtest_main)
        target_include_directories(drumprog_gui_benchmarks PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        drumprog_enable_warnings(drumprog_gui_benchmarks)
        gtest_discover_tests(drumprog_gui_benchmarks DISCOVERY_MODE PRE_TEST)
    endif()
endif()
