cmake_minimum_required(VERSION 4.1.1)
project(signer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(signer
    src/signer.cpp
)

target_include_directories(signer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Example: dependencies (if any)
# target_link_libraries(mylib PUBLIC fmt::fmt)

add_library(signer::signer ALIAS signer)

enable_testing()
add_subdirectory(tests)
