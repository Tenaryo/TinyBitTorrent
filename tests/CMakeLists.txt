file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach (TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(
        ${TEST_NAME} PRIVATE TinyBitTorrent_core GTest::gtest_main
    )
    gtest_discover_tests(${TEST_NAME})
endforeach ()
