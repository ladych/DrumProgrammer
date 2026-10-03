# Testable logic (E-02): everything in this library is measured by the coverage gate.
add_library(drumprog_core STATIC
    engine/TestToneSource.cpp
    io/DeviceSettingsStore.cpp
    io/StdFileSystem.cpp
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
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:DrumProgrammer,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:DrumProgrammer,JUCE_VERSION>"
)

# JUCE itself comes compiled from drumprog_juce through drumprog_core (cmake/DrumProgrammerJuce.cmake).
target_link_libraries(DrumProgrammer PRIVATE drumprog_core)
