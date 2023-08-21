# Default filter, is well..... default
option(FILTER_DEFAULT "FILTER_DEFAULT" ON)

# fftw3
pkg_check_modules(FFTW3 REQUIRED fftw3f)
list(APPEND INCLUDE_DIRS "${FFTW3_INCLUDE_DIRS}")
list(APPEND LINK_DIRS "${FFTW3_LIBRARY_DIRS}")

if(FILTER_DEFAULT)
    message(STATUS "Default filter enabled!")
    add_library(filter_default SHARED "${WAVA_MODULE_DIR}/main.c"
                                        "${GLOBAL_FUNCTION_SOURCES}")
    target_link_libraries(filter_default wava-shared "${FFTW3_LIBRARIES}")
    target_link_directories(filter_default PRIVATE "${FFTW3_LIBRARY_DIRS}")
    set_target_properties(filter_default PROPERTIES PREFIX "")
    target_include_directories(filter_default PRIVATE ${FFTW3_INCLUDE_DIRS})
    install(TARGETS filter_default DESTINATION lib/wava)

    find_and_copy_dlls(filter_default)

    # Add legal disclaimer
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_fftw.txt"
        "FFTW License can be obtained at: http://fftw.org/doc/License-and-Copyright.html\n")
endif()

