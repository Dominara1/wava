# Project default
option(CAIRO_MODULES "CAIRO_MODULES" ON)

if(CAIRO_MODULES)
    pkg_check_modules(CAIRO QUIET cairo libcurl taglib zlib dbus-1)
    if(CAIRO_FOUND)
        add_subdirectory(lib/kiss-mpris-properties)

        add_library(cairo_media_info SHARED "${WAVA_MODULE_DIR}/main.c"
                                    "src/output/shared/util/media/artwork.c"
                                    "src/output/shared/util/media/artwork_taglib.cpp"
                                    "src/output/shared/util/media/media_data.c"
                                    "${GLOBAL_FUNCTION_SOURCES}")
                                #"${WAVA_MODULE_DIR}/../../"

        target_link_directories(cairo_media_info PRIVATE
            "${CAIRO_LIBRARY_DIRS}")
        target_include_directories(cairo_media_info PRIVATE
            "${CAIRO_INCLUDE_DIRS}" lib/kiss-mpris-properties/src
            lib/stb)
        target_link_libraries(cairo_media_info wava-shared kiss-mpris
            "${CAIRO_LIBRARIES}")

        target_compile_definitions(cairo_media_info PUBLIC -DCAIRO)
        set_target_properties(cairo_media_info PROPERTIES PREFIX "")
        set_target_properties(cairo_media_info PROPERTIES IMPORT_PREFIX "")
        configure_file("${WAVA_MODULE_DIR}/config.ini"   cairo/module/media_info/config.ini   COPYONLY)

        # this copies the dlls for mr. windows
        #find_and_copy_dlls(cairo_media_info)

        set_target_properties(cairo_media_info PROPERTIES OUTPUT_NAME "cairo/module/media_info/module")
        install(TARGETS cairo_media_info RENAME module DESTINATION share/wava/cairo/module/media_info/)
    else()
        message(WARNING "CAIRO, TagLib, Zlib, DBus-1 and/or CURL library not found; \"cairo_media_info\" won't build")
    endif()
endif()
