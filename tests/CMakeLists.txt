if(NOT EXISTS "${CMAKE_SOURCE_DIR}/external/googletest/CMakeLists.txt")
  message(FATAL_ERROR "googletest submodule missing. Run: git submodule update --init --recursive")
endif()

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/googletest ${CMAKE_BINARY_DIR}/external/googletest)

add_executable(test_signer test_signer.cpp)
target_link_libraries(test_signer
    PRIVATE
        signer::signer
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_signer)
