# Testable logic (E-02): everything in this library is measured by the coverage gate.
add_library(drumprog_core STATIC
    audio/DeviceRecovery.cpp
    audio/DriverChoice.cpp
    engine/BackingTrackPlayer.cpp
    engine/Clock.cpp
    engine/Downmix.cpp
    engine/EngineKit.cpp
    engine/KitBuilder.cpp
    engine/KitDescription.cpp
    engine/KitPublisher.cpp
    engine/Metronome.cpp
    engine/PlaybackRenderer.cpp
    engine/Resampler.cpp
    engine/SampleBuffer.cpp
    engine/SampleEngine.cpp
    engine/Sequencer.cpp
    engine/TempoMath.cpp
    engine/TestToneSource.cpp
    engine/TriggerIndicators.cpp
    engine/VelocityCurve.cpp
    engine/Voice.cpp
    engine/VoicePool.cpp
    i18n/LanguageCatalog.cpp
    i18n/Translate.cpp
    input/InputActivity.cpp
    input/KeyboardInput.cpp
    input/KeyNames.cpp
    input/Keymap.cpp
    input/MidiInputHandler.cpp
    input/Scancode.cpp
    io/DefaultKit.cpp
    io/GlobalKit.cpp
    io/MidiFileReader.cpp
    io/MidiFileWriter.cpp
    io/MidiImport.cpp
    io/MissingSamples.cpp
    io/PatternMidiExport.cpp
    io/ProjectRepository.cpp
    io/ProjectSerializer.cpp
    io/SettingsStore.cpp
    io/SongMidiExport.cpp
    io/StdFileSystem.cpp
    model/Project.cpp
    model/ProjectFactory.cpp
    model/SnapshotBuilder.cpp
    model/SnapshotPublisher.cpp
    model/SongLayout.cpp
    model/SongTakeRecorder.cpp
    model/TakeRecorder.cpp
    model/TreeChangeListener.cpp
    model/TreeNode.cpp
    model/UuidGenerator.cpp
    ui/AboutInfo.cpp
    ui/ActivePattern.cpp
    ui/ActivityLed.cpp
    ui/AudioStatus.cpp
    ui/BackingTrackPresenter.cpp
    ui/DocumentController.cpp
    ui/InputLedPresenter.cpp
    ui/KeymapPresenter.cpp
    ui/KeyRouter.cpp
    ui/KitPresenter.cpp
    ui/MidiExportController.cpp
    ui/MidiImportController.cpp
    ui/MixPresenter.cpp
    ui/MusicalTime.cpp
    ui/PaneSize.cpp
    ui/PatternListPresenter.cpp
    ui/PianoRollGeometry.cpp
    ui/PianoRollPresenter.cpp
    ui/SnapGrid.cpp
    ui/SongTimelineGeometry.cpp
    ui/SongTimelinePresenter.cpp
    ui/TempoPresenter.cpp
    ui/TransportPresenter.cpp
)
target_include_directories(drumprog_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(drumprog_core PUBLIC drumprog_juce)
drumprog_enable_warnings(drumprog_core)

if(NOT DRUMPROG_BUILD_APP)
    return()
endif()

# Humble objects (E-03): JUCE glue without logic, listed in coverage-exclusions.txt.
set(DRUMPROG_APP_SOURCES
    app/Main.cpp
    app/AboutDialog.cpp
    app/AppComposition.cpp
    app/AudioDeviceSupervisor.cpp
    app/Dialogs.cpp
    app/IconButton.cpp
    app/JuceBackingTrackLoader.cpp
    app/JuceDocumentView.cpp
    app/JuceMidiDialogs.cpp
    app/JuceSampleLoader.cpp
    app/KeymapDialog.cpp
    app/KitPanel.cpp
    app/MainComponent.cpp
    app/MainMenu.cpp
    app/MetronomePanel.cpp
    app/MixPanel.cpp
    app/NoteInspector.cpp
    app/PatternListPanel.cpp
    app/PianoRollToolbar.cpp
    app/PianoRollView.cpp
    app/SongTimelineView.cpp
    app/Splitter.cpp
    app/TransportBar.cpp
)

# Physical keyboard access (scancodes) is platform code in separate files (Q-13).
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(X11 REQUIRED)
    list(APPEND DRUMPROG_APP_SOURCES app/NativeKeyboard_linux.cpp)
elseif(WIN32)
    list(APPEND DRUMPROG_APP_SOURCES app/NativeKeyboard_windows.cpp)
else()
    list(APPEND DRUMPROG_APP_SOURCES app/NativeKeyboard_other.cpp)
endif()

juce_add_gui_app(DrumProgrammer
    PRODUCT_NAME "Drum Programmer"
    COMPANY_NAME "ladych"
    VERSION ${PROJECT_VERSION}
)

target_sources(DrumProgrammer PRIVATE ${DRUMPROG_APP_SOURCES})
drumprog_enable_warnings_for_sources(${DRUMPROG_APP_SOURCES})

target_compile_definitions(DrumProgrammer PRIVATE
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:DrumProgrammer,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:DrumProgrammer,JUCE_VERSION>"
)

# Default kit (io::DefaultKit): its samples and license text are built into the program.
file(GLOB DRUMPROG_DEFAULT_KIT_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/resources/DefaultKit/*")
juce_add_binary_data(DrumProgrammerDefaultKit
    HEADER_NAME DefaultKitData.h
    NAMESPACE DefaultKitData
    SOURCES ${DRUMPROG_DEFAULT_KIT_FILES})
# Its generated header is included as a system header, so clang-tidy does not check it.
target_link_libraries(DrumProgrammer PRIVATE $<LINK_ONLY:DrumProgrammerDefaultKit>)
target_include_directories(DrumProgrammer SYSTEM PRIVATE
    $<TARGET_PROPERTY:DrumProgrammerDefaultKit,INTERFACE_INCLUDE_DIRECTORIES>)

# Button icons (app::IconButton): the SVG files of Lucide Icons, see resources/Icons/LIESMICH.txt.
file(GLOB DRUMPROG_ICON_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/resources/Icons/*.svg")
juce_add_binary_data(DrumProgrammerIcons
    HEADER_NAME IconData.h
    NAMESPACE IconData
    SOURCES ${DRUMPROG_ICON_FILES})
target_link_libraries(DrumProgrammer PRIVATE $<LINK_ONLY:DrumProgrammerIcons>)
target_include_directories(DrumProgrammer SYSTEM PRIVATE
    $<TARGET_PROPERTY:DrumProgrammerIcons,INTERFACE_INCLUDE_DIRECTORIES>)

# Translation files (AP15, i18n::LanguageCatalog): one file per language, see resources/Languages/de.txt.
file(GLOB DRUMPROG_LANGUAGE_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/resources/Languages/*.txt")
juce_add_binary_data(DrumProgrammerLanguages
    HEADER_NAME LanguageData.h
    NAMESPACE LanguageData
    SOURCES ${DRUMPROG_LANGUAGE_FILES})
target_link_libraries(DrumProgrammer PRIVATE $<LINK_ONLY:DrumProgrammerLanguages>)
target_include_directories(DrumProgrammer SYSTEM PRIVATE
    $<TARGET_PROPERTY:DrumProgrammerLanguages,INTERFACE_INCLUDE_DIRECTORIES>)

# JUCE itself comes compiled from drumprog_juce through drumprog_core (cmake/DrumProgrammerJuce.cmake).
target_link_libraries(DrumProgrammer PRIVATE drumprog_core)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(DrumProgrammer PRIVATE X11::X11)
endif()
