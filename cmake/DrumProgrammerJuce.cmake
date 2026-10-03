# JUCE as one static library (drumprog_juce) shared by drumprog_core, the tests and the app.
#
# JUCE modules are CMake INTERFACE libraries that compile their sources into every target linking
# them. The model (AP1) uses juce::ValueTree in drumprog_core, and the app links drumprog_core, so
# linking the modules directly would compile JUCE twice into the app. Instead the modules are
# compiled once here, with JUCE's own warning level, and only their headers and compile definitions
# are passed on.
#
# Without the app only the non-GUI modules are built, so the coverage and sanitizer builds need no
# X11, ALSA or JACK headers and skip juceaide.

if(NOT DRUMPROG_BUILD_APP)
    set(JUCE_MODULES_ONLY ON CACHE BOOL "" FORCE)
endif()
add_subdirectory(external/JUCE EXCLUDE_FROM_ALL)

# Every module whose headers our code may include, with all of their dependencies.
set(DRUMPROG_JUCE_MODULES juce_core juce_events juce_data_structures)
if(DRUMPROG_BUILD_APP)
    list(APPEND DRUMPROG_JUCE_MODULES
        juce_graphics juce_gui_basics juce_gui_extra
        juce_audio_basics juce_audio_devices juce_audio_formats juce_audio_processors juce_audio_utils)
endif()

add_library(drumprog_juce STATIC)
target_link_libraries(drumprog_juce
    PRIVATE
        ${DRUMPROG_JUCE_MODULES}
    PUBLIC
        juce::juce_recommended_config_flags)

target_compile_definitions(drumprog_juce PUBLIC
    JUCE_STANDALONE_APPLICATION=1
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    $<IF:$<CONFIG:Debug>,DEBUG=1 _DEBUG=1,NDEBUG=1 _NDEBUG=1>
    $<$<PLATFORM_ID:Linux>:JUCE_JACK=1>
    $<$<PLATFORM_ID:Linux>:JUCE_ALSA=1>)

foreach(module IN LISTS DRUMPROG_JUCE_MODULES)
    target_compile_definitions(drumprog_juce PUBLIC $<TARGET_PROPERTY:${module},INTERFACE_COMPILE_DEFINITIONS>)
    target_include_directories(drumprog_juce SYSTEM PUBLIC $<TARGET_PROPERTY:${module},INTERFACE_INCLUDE_DIRECTORIES>)
endforeach()
