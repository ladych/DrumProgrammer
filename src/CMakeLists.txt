# Testable logic (E-02): everything in this library is measured by the coverage gate.
add_library(drumprog_core STATIC
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
    input/InputActivity.cpp
    input/KeyboardInput.cpp
    input/KeyNames.cpp
    input/Keymap.cpp
    input/MidiInputHandler.cpp
    input/Scancode.cpp
    io/GlobalKit.cpp
    io/MissingSamples.cpp
    io/ProjectRepository.cpp
    io/ProjectSerializer.cpp
    io/SettingsStore.cpp
    io/StdFileSystem.cpp
    model/Project.cpp
    model/ProjectFactory.cpp
    model/SnapshotBuilder.cpp
    model/SnapshotPublisher.cpp
    model/TakeRecorder.cpp
    model/TreeChangeListener.cpp
    model/TreeNode.cpp
    model/UuidGenerator.cpp
    ui/ActivePattern.cpp
    ui/ActivityLed.cpp
    ui/AudioStatus.cpp
    ui/DocumentController.cpp
    ui/InputLedPresenter.cpp
    ui/KeymapPresenter.cpp
    ui/KeyRouter.cpp
    ui/KitPresenter.cpp
    ui/MusicalTime.cpp
    ui/PatternListPresenter.cpp
    ui/PianoRollGeometry.cpp
    ui/PianoRollPresenter.cpp
    ui/SnapGrid.cpp
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
    app/AppComposition.cpp
    app/Dialogs.cpp
    app/JuceDocumentView.cpp
    app/JuceSampleLoader.cpp
    app/KeymapDialog.cpp
    app/KitPanel.cpp
    app/MainComponent.cpp
    app/MainMenu.cpp
    app/NoteInspector.cpp
    app/PatternListPanel.cpp
    app/PianoRollToolbar.cpp
    app/PianoRollView.cpp
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

# JUCE itself comes compiled from drumprog_juce through drumprog_core (cmake/DrumProgrammerJuce.cmake).
target_link_libraries(DrumProgrammer PRIVATE drumprog_core)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(DrumProgrammer PRIVATE X11::X11)
endif()
