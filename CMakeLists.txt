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


# Add secp256k1 as subdirectory
set(SECP256K1_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_EXHAUSTIVE_TESTS OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SECP256K1_ENABLE_MODULE_RECOVERY ON CACHE BOOL "" FORCE)
add_subdirectory(external/secp256k1)

# Sodium dependency resolution
# - On Apple (macOS/iOS/simulator): use external/sodium.xcframework (build it if missing)
# - Otherwise: find_package(sodium)

set(SODIUM_TARGET "")

if(APPLE)
    set(SODIUM_XC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/sodium.xcframework")
    if(NOT EXISTS "${SODIUM_XC_DIR}")
        message(STATUS "sodium.xcframework not found. Running build-sodium.sh to create it...")
        execute_process(
            COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/build-sodium.sh"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE SODIUM_BUILD_RESULT
        )
        if(NOT SODIUM_BUILD_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to build sodium.xcframework (exit code ${SODIUM_BUILD_RESULT})")
        endif()
    endif()

    # Resolve correct slice from XCFramework
    set(SODIUM_LIB "")
    set(SODIUM_INCLUDE "")

    if(CMAKE_OSX_SYSROOT STREQUAL "iphoneos")
        file(GLOB _SODIUM_IOS_DEVICE_LIBS "${SODIUM_XC_DIR}/ios-*/libsodium.a")
        if(_SODIUM_IOS_DEVICE_LIBS)
            list(FILTER _SODIUM_IOS_DEVICE_LIBS EXCLUDE REGEX "simulator")
            list(GET _SODIUM_IOS_DEVICE_LIBS 0 SODIUM_LIB)
            get_filename_component(_SLICE_DIR "${SODIUM_LIB}" DIRECTORY)
            set(SODIUM_INCLUDE "${_SLICE_DIR}/Headers")
        endif()
    elseif(CMAKE_OSX_SYSROOT STREQUAL "iphonesimulator")
        file(GLOB _SODIUM_IOS_SIM_LIBS "${SODIUM_XC_DIR}/ios-*/libsodium.a")
        if(_SODIUM_IOS_SIM_LIBS)
            list(FILTER _SODIUM_IOS_SIM_LIBS INCLUDE REGEX "simulator")
            list(GET _SODIUM_IOS_SIM_LIBS 0 SODIUM_LIB)
            get_filename_component(_SLICE_DIR "${SODIUM_LIB}" DIRECTORY)
            set(SODIUM_INCLUDE "${_SLICE_DIR}/Headers")
        endif()
    else()
        # macOS
        file(GLOB _SODIUM_MAC_LIBS "${SODIUM_XC_DIR}/macos-*/libsodium.a")
        if(_SODIUM_MAC_LIBS)
            list(GET _SODIUM_MAC_LIBS 0 SODIUM_LIB)
            get_filename_component(_SLICE_DIR "${SODIUM_LIB}" DIRECTORY)
            set(SODIUM_INCLUDE "${_SLICE_DIR}/Headers")
        endif()
    endif()

    if(NOT SODIUM_LIB OR NOT EXISTS "${SODIUM_LIB}")
        message(FATAL_ERROR "Failed to locate libsodium.a within ${SODIUM_XC_DIR} for sysroot ${CMAKE_OSX_SYSROOT}")
    endif()

    add_library(sodium STATIC IMPORTED)
    set_target_properties(sodium PROPERTIES IMPORTED_LOCATION "${SODIUM_LIB}")
    target_include_directories(sodium INTERFACE "${SODIUM_INCLUDE}")
    set(SODIUM_TARGET sodium)
else()
    find_package(sodium REQUIRED)
    # When found via find_package, the imported target may be 'sodium::sodium' or just 'sodium'
    # Prefer the standard names if available
    if(TARGET sodium::sodium)
        set(SODIUM_TARGET sodium::sodium)
    elseif(TARGET sodium)
        set(SODIUM_TARGET sodium)
    else()
        message(FATAL_ERROR "sodium library not found by find_package")
    endif()
endif()

target_link_libraries(signer
    PRIVATE
        secp256k1
        ${SODIUM_TARGET}
)


# Example: dependencies (if any)
# target_link_libraries(mylib PUBLIC fmt::fmt)

add_library(signer::signer ALIAS signer)

# Enable tests except for iOS device/simulator builds or Xcode generator
if((APPLE AND (CMAKE_OSX_SYSROOT STREQUAL "iphoneos" OR CMAKE_OSX_SYSROOT STREQUAL "iphonesimulator"))
   OR (CMAKE_GENERATOR STREQUAL "Xcode"))
    message(STATUS "Skipping tests (sysroot: ${CMAKE_OSX_SYSROOT}, generator: ${CMAKE_GENERATOR})")
else()
    enable_testing()
    add_subdirectory(tests)
endif()
