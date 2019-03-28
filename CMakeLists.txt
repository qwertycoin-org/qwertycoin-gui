cmake_minimum_required(VERSION 3.10)

if(POLICY CMP0074)
    # Allow find_package() to use <PackageName>_ROOT variables.
    # This policy was introduced in CMake version 3.12.
    cmake_policy(SET CMP0074 NEW)
endif()

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_LIST_DIR}/cmake/find")

# Configure project build type.
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported build types" FORCE)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
message(STATUS "CMake build type: ${CMAKE_BUILD_TYPE}")

# Configure Hunter (CMake driven cross-platform package manager for C/C++).
# WARNING: Must be included before project(...) function call!
set(HUNTER_CONFIGURATION_TYPES Release CACHE STRING "Hunter configuration types")
set(HUNTER_JOBS_NUMBER 4 CACHE STRING "Hunter jobs number")
include(hunter/HunterGate)
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.23.122.tar.gz"
    SHA1 "d8c252862a9900a703167ba703bfb1b17977948f"
    FILEPATH "${CMAKE_CURRENT_LIST_DIR}/cmake/hunter/HunterConfig.cmake"
)

# Read git revision.
find_package(Git)
message(STATUS "Found Git: ${GIT_FOUND}")
if(GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE PROJECT_GIT_RESULT
        OUTPUT_VARIABLE PROJECT_GIT_COMMIT_ID
        ERROR_VARIABLE PROJECT_GIT_ERROR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE)
    if(NOT PROJECT_GIT_RESULT EQUAL 0)
        set(PROJECT_GIT_COMMIT_ID "unknown")
        message(WARNING "Failed to execute Git: ${PROJECT_GIT_ERROR}")
    else()
        message(STATUS "Git commit ID: ${PROJECT_GIT_COMMIT_ID}")
    endif()
else()
    set(PROJECT_GIT_COMMIT_ID "unknown")
    message(WARNING "Git not found")
endif()

set(PROJECT_DISPLAY_NAME "Qwertycoin")
set(PROJECT_NAME "qwertycoin-gui")
set(PROJECT_DESCRIPTION "Safe. Simple. Seamless. Secure.")
set(PROJECT_VERSION "5.1.4")
set(PROJECT_COPYRIGHT "Copyright 2019 The Qwertycoin Group")
set(PROJECT_VENDOR_NAME "The Qwertycoin Group")
set(PROJECT_VENDOR_URL "https://qwertycoin.org")
set(PROJECT_CURRENCY_DISPLAY_NAME "Qwertycoin")
set(PROJECT_CURRENCY_TICKER "QWC")
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES C CXX)

if(APPLE)
    # Compile src/Platform/OSX/System/asm.s file.
    enable_language(ASM)
endif()

if(WIN32 AND NOT MSVC)
    message(FATAL_ERROR "Only MSVC is supported on this platform!")
endif()

include(project/CompilerSettings)
include(project/GlobalVariables)

# External dependencies
set_directory_properties(PROPERTIES EP_BASE "${CMAKE_CURRENT_BINARY_DIR}/_ExternalProjects")
include(ExternalProject)
include(external/Boost)
include(external/qrencode)
include(external/Qt5)
include(external/Qwertycoin)
include(external/Threads)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/src")

include(project/CPackConfig)
