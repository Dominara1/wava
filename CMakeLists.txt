project(wava C CXX)
cmake_minimum_required(VERSION 3.13)

# Include common funnctions
include(src/cmake/utilities.cmake)

# Fix/handle common build errors
include(src/cmake/build-fix.cmake)

# Debug is default
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

# Allow for custom link prefixes (in order to optimize CMake builds for packagers)
if(NOT WAVA_LINK_TO_INSTALL_DIR)
	set(WAVA_LINK_TO_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}")
endif()

# Define WAVA version
set(wava_VERSION_MAJOR 0)
set(wava_VERSION_MINOR 7)
set(wava_VERSION_PATCH 1)
set(wava_VERSION_TWEAK 1)
set(wava_VERSION "${wava_VERSION_MAJOR}.${wava_VERSION_MINOR}.${wava_VERSION_PATCH}.${wava_VERSION_TWEAK}" CACHE STRING "WAVA version")

# Definitions
add_definitions(-DPACKAGE="wava" -DVERSION="${wava_VERSION}")
add_definitions(-DWAVA_VERSION_MAJOR"=${wava_VERSION_MAJOR}")
add_definitions(-DWAVA_VERSION_MINOR"=${wava_VERSION_MINOR}")
add_definitions(-DWAVA_VERSION_PATCH"=${wava_VERSION_PATCH}")
add_definitions(-DWAVA_VERSION_TWEAK"=${wava_VERSION_TWEAK}")
add_definitions(-DPREFIX="${WAVA_LINK_TO_INSTALL_DIR}")

# Do OS-specific stuff
include(src/cmake/os-prerequisites.cmake)

# Set appropriate C flags
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-Wall -Wextra ${BUILD_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG ${BUILD_DEBUG_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "-O3 -fvisibility='internal'")
set(CMAKE_CXX_FLAGS "-Wall -Wextra ${BUILD_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG ${BUILD_DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fvisibility='internal' ")

find_package(PkgConfig REQUIRED)

# math lib
find_library(MATH_LIB m HINTS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
if(NOT MATH_LIB)
	message(FATAL_ERROR "math library not found")
endif()

# pthread
find_library(THREAD_LIB pthread HINTS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
if(NOT THREAD_LIB)
	message(FATAL_ERROR "pthread library not found")
endif()

include_directories(src)

# Build shared library
include("src/shared/build.cmake")

# Build modules
include("src/cmake/modules.cmake")

# Build executable
add_executable(
	wava
	${ADDITIONAL_OBJS}
	src/config.c
	src/wava.c
)

# Link executable
target_link_libraries(wava wava-shared m pthread)

include(src/cmake/install_and_configure.cmake)

