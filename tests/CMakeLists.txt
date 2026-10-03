add_executable(drumprog_tests
    engine/EngineKitTest.cpp
    engine/KitBuilderTest.cpp
    engine/KitDescriptionTest.cpp
    engine/SampleBufferTest.cpp
    engine/SampleEngineTest.cpp
    engine/SpscQueueTest.cpp
    engine/TestToneSourceTest.cpp
    engine/TriggerIndicatorsTest.cpp
    engine/VelocityCurveTest.cpp
    engine/VoicePoolTest.cpp
    engine/VoiceTest.cpp
    io/DeviceSettingsStoreTest.cpp
    io/StdFileSystemTest.cpp
    ui/KitPresenterTest.cpp
)

# Q-04: replaces the global operator new to count allocations, which clashes with the
# sanitizers' own allocator. The regular, coverage and Windows builds run it.
if(NOT DRUMPROG_SANITIZERS)
    target_sources(drumprog_tests PRIVATE engine/RealtimeAllocationTest.cpp)
endif()

target_link_libraries(drumprog_tests PRIVATE drumprog_core GTest::gmock_main)
target_include_directories(drumprog_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
drumprog_enable_warnings(drumprog_tests)

include(GoogleTest)
gtest_discover_tests(drumprog_tests DISCOVERY_MODE PRE_TEST)
