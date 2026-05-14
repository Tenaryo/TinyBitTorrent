file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach (TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(
        ${TEST_NAME}
        ${TEST_FILE}
        ${CMAKE_SOURCE_DIR}/src/bencode/decoder.cpp
        ${CMAKE_SOURCE_DIR}/src/bencode/encoder.cpp
        ${CMAKE_SOURCE_DIR}/src/torrent/metainfo.cpp
        ${CMAKE_SOURCE_DIR}/src/output/output.cpp
        ${CMAKE_SOURCE_DIR}/src/util/sha1.cpp
        ${CMAKE_SOURCE_DIR}/src/util/url_encode.cpp
        ${CMAKE_SOURCE_DIR}/src/http/http_client.cpp
        ${CMAKE_SOURCE_DIR}/src/tracker/tracker.cpp
    )
    target_link_libraries(
        ${TEST_NAME} PRIVATE TinyBitTorrent_core GTest::gtest_main
    )
    if (${TEST_NAME} STREQUAL "test_e2e")
        target_compile_definitions(
            ${TEST_NAME}
            PRIVATE SAMPLE_TORRENT="${CMAKE_SOURCE_DIR}/sample.torrent"
        )
    endif ()
    gtest_discover_tests(${TEST_NAME})
endforeach ()
