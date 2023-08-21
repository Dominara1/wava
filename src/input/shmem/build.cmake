# Project default
option(SHMEM "SHMEM" OFF)

if(SHMEM)
    add_definitions(-DSHMEM)
    add_library(in_shmem SHARED "${WAVA_MODULE_DIR}/main.c"
                                "${GLOBAL_FUNCTION_SOURCES}")
    target_link_libraries(in_shmem wava-shared "-lrt")
    set_target_properties(in_shmem PROPERTIES PREFIX "")
    install(TARGETS in_shmem DESTINATION lib/wava)

    # Maybe license? pls no sue
endif()

