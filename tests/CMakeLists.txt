add_executable(drumprog_tests
    engine/SnapshotExchangeTest.cpp
    engine/TestToneSourceTest.cpp
    io/DeviceSettingsStoreTest.cpp
    io/ProjectRepositoryTest.cpp
    io/ProjectSerializerTest.cpp
    io/StdFileSystemTest.cpp
    model/ProjectFactoryTest.cpp
    model/ProjectTest.cpp
    model/SnapshotBuilderTest.cpp
    model/SnapshotPublisherTest.cpp
    model/TreeChangeListenerTest.cpp
    model/UuidGeneratorTest.cpp
    ui/DocumentControllerTest.cpp
)
target_link_libraries(drumprog_tests PRIVATE drumprog_core GTest::gmock_main)
target_include_directories(drumprog_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
drumprog_enable_warnings(drumprog_tests)

include(GoogleTest)
gtest_discover_tests(drumprog_tests DISCOVERY_MODE PRE_TEST)
