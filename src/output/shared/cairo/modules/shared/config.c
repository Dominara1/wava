#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cairo.h>

#include "shared.h"

#include "output/shared/cairo/main.h"

#include "output/shared/cairo/util/feature_compat.h"
#include "output/shared/cairo/util/module.h"
#include "output/shared/cairo/util/region.h"

#include "config.h"

void *wava_cairo_module_file_load(
        wava_cairo_file_type      type,
        wava_cairo_module_handle *module,
        const char               *file_name,
        char                     *returned_path) {

    char file_path[MAX_PATH];
    void *pointer = NULL;

    wava_config_source config;

    // "cairo/module/$module_name"
    strcpy(file_path, module->prefix);
    strcat(file_path, "/");
    // add file name
    strcat(file_path, file_name);

    enum WAVA_FILE_TYPE wava_io_file_type;
    switch(type) {
        case WAVA_CAIRO_FILE_INSTALL_READ:
            wava_io_file_type = WAVA_FILE_TYPE_PACKAGE;
            break;
        default:
            wava_io_file_type = WAVA_FILE_TYPE_CONFIG;
            break;
    }

    // get REAL path
    char *actual_path;
    wavaReturnErrorCondition(
        wavaFindAndCheckFile(wava_io_file_type,
            file_path, &actual_path) == false,
        NULL,
        "Failed to find or open '%s'", file_path);

    switch(type) {
        case WAVA_CAIRO_FILE_CONFIG:
            config = wavaConfigOpen(actual_path);
            pointer = malloc(sizeof(wava_config_source));
            wavaBailCondition(pointer == NULL, "Memory error");
            ((wava_config_source*)pointer)[0] = config;
            break;
        case WAVA_CAIRO_FILE_INSTALL_READ:
        case WAVA_CAIRO_FILE_CONFIG_CUSTOM_READ:
            pointer = fopen(file_path, "rb");
            break;
        case WAVA_CAIRO_FILE_CONFIG_CUSTOM_WRITE:
            pointer = fopen(file_path, "wb");
            break;
        default:
            wavaReturnError(NULL, "Invalid file type");
    }

    if(returned_path != NULL) {
        strcpy(returned_path, actual_path);
    }
    free(actual_path);

    return pointer;
}

