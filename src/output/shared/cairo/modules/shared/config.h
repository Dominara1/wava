#ifndef __WAVA_OUTPUT_CAIRO_MODULE_UTIL_CONFIG
#define __WAVA_OUTPUT_CAIRO_MODULE_UTIL_CONFIG

#include "output/shared/cairo/util/module.h"

typedef enum wava_cairo_file_type {
    WAVA_CAIRO_FILE_CONFIG,               // for loading wava_config_source-es
    WAVA_CAIRO_FILE_CONFIG_CUSTOM_READ,   // for loading any file found in the config dir
    WAVA_CAIRO_FILE_CONFIG_CUSTOM_WRITE,  // the same as above
    WAVA_CAIRO_FILE_INSTALL_READ,         // for loading any files off of the install
    WAVA_CAIRO_FILE_ERROR,                // placeholder
} wava_cairo_file_type;

/**
 * A config/custom file helper for WAVA_CAIRO modules
 *
 * 1. You need to specify what type the file actually is
 * 2. A reference to the module that called it
 * 3. Local file name within your designated config folder for your module.
 * 4. If non-NULL, will write the full output file path here
 *
 * The return pointer where the resulting FILE* or wava_config_source gets put
 *
 * On fail the function returns a NULL
**/
void *wava_cairo_module_file_load(
        wava_cairo_file_type      type,
        wava_cairo_module_handle *module,
        const char               *file_name,
        char                     *returned_path);

#endif

