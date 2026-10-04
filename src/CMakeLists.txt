# Testable logic (E-02): everything in this library is measured by the coverage gate.
add_library(drumprog_core STATIC
    engine/EngineKit.cpp
    engine/KitBuilder.cpp
    engine/KitDescription.cpp
    engine/Resampler.cpp
    engine/SampleBuffer.cpp
    engine/SampleEngine.cpp
    engine/TestToneSource.cpp
    engine/TriggerIndicators.cpp
    engine/VelocityCurve.cpp
    engine/Voice.cpp
    engine/VoicePool.cpp
    io/DeviceSettingsStore.cpp
    io/ProjectRepository.cpp
    io/ProjectSerializer.cpp
    io/StdFileSystem.cpp
    model/Project.cpp
    model/ProjectFactory.cpp
    model/SnapshotBuilder.cpp
    model/SnapshotPublisher.cpp
    model/TreeChangeListener.cpp
    model/TreeNode.cpp
    model/UuidGenerator.cpp
    ui/DocumentController.cpp
    ui/KitPresenter.cpp
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
    app/JuceDocumentView.cpp
    app/JuceSampleLoader.cpp
    app/KitPanel.cpp
    app/MainComponent.cpp
    app/MainMenu.cpp
)

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
