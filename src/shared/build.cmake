project(wava-shared C)

# Optional features
set(ADDITIONAL_SHARED_SOURCES "")
set(ADDITIONAL_SHARED_LIBRARIES "")
set(ADDITIONAL_SHARED_DEFINITIONS "")
set(ADDITIONAL_SHARED_INCLUDE_DIRS "")

# Pull git required submodules
execute_process(COMMAND git submodule update --init)

# Pull submodule and install dependency
add_library(iniparser STATIC
        lib/iniparser/src/dictionary.c
        lib/iniparser/src/iniparser.c)
set_target_properties(iniparser PROPERTIES COMPILE_FLAGS "-fPIC")

# Runtime library load
if(UNIX)
    list(APPEND ADDITIONAL_SHARED_LIBRARIES "dl")
    list(APPEND ADDITIONAL_SHARED_SOURCES "src/shared/module/unix.c")
    list(APPEND ADDITIONAL_SHARED_SOURCES "src/shared/io/unix.c")
elseif(MSYS OR MINGW OR MSVC)
    list(APPEND ADDITIONAL_SHARED_SOURCES "src/shared/module/windows.c")
else()
    message(FATAL_ERROR "Your OS/Platform does NOT support modules!")
endif()

# Build WAVA shared library
add_library(wava-shared SHARED
    src/shared/log.c
    src/shared/config/config.c
    src/shared/module/abstractions.c
    src/shared/ionotify.c
    src/shared/io/io.c
    src/shared/util/version.c
    ${ADDITIONAL_SHARED_SOURCES}
)
set_target_properties(wava-shared PROPERTIES COMPILE_FLAGS "-fPIC")
target_link_libraries(wava-shared PRIVATE ${ADDITIONAL_SHARED_LIBRARIES} iniparser pthread)
target_compile_definitions(wava-shared PRIVATE ${ADDITIONAL_SHARED_DEFINITIONS})
target_include_directories(wava-shared PRIVATE "${ADDITIONAL_SHARED_INCLUDE_DIRS}" lib/iniparser/src lib/x-watcher)

install (TARGETS wava-shared DESTINATION lib)

find_and_copy_dlls(wava-shared)

# Add legal disclaimers
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_iniparser.txt"
    "INIparser license can be obtained at: https://raw.githubusercontent.com/ndevilla/iniparser/master/LICENSE\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_xwatcher.txt"
    "xWatcher license can be obtained at: https://raw.githubusercontent.com/nikp123/x-watcher/master/README.md\n")

