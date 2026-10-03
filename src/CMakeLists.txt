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
    io/StdFileSystem.cpp
    ui/KitPresenter.cpp
)
target_include_directories(drumprog_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
drumprog_enable_warnings(drumprog_core)

if(NOT DRUMPROG_BUILD_APP)
    return()
endif()

# Humble objects (E-03): JUCE glue without logic, listed in coverage-exclusions.txt.
set(DRUMPROG_APP_SOURCES
    app/Main.cpp
    app/AppComposition.cpp
    app/JuceSampleLoader.cpp
    app/KitPanel.cpp
    app/MainComponent.cpp
)

juce_add_gui_app(DrumProgrammer
    PRODUCT_NAME "Drum Programmer"
    COMPANY_NAME "ladych"
    VERSION ${PROJECT_VERSION}
)

target_sources(DrumProgrammer PRIVATE ${DRUMPROG_APP_SOURCES})
drumprog_enable_warnings_for_sources(${DRUMPROG_APP_SOURCES})

target_compile_definitions(DrumProgrammer PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:DrumProgrammer,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:DrumProgrammer,JUCE_VERSION>"
    $<$<PLATFORM_ID:Linux>:JUCE_JACK=1>
    $<$<PLATFORM_ID:Linux>:JUCE_ALSA=1>
)

target_link_libraries(DrumProgrammer
    PRIVATE
        drumprog_core
        juce::juce_audio_formats
        juce::juce_audio_utils
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
