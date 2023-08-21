#ifndef __WAVA_SHARED_MODULE_H
#define __WAVA_SHARED_MODULE_H
#include <stdbool.h>

// don't leak internal structure as the user of this library
// isn't supposed to mess with it
typedef struct wavamodule WAVAMODULE;

extern char *wava_module_error_get(WAVAMODULE *module);
extern bool wava_module_valid(WAVAMODULE *module);
extern void *wava_module_symbol_address_get(WAVAMODULE *module, char *symbol);
extern WAVAMODULE *wava_module_input_load(char *name);
extern WAVAMODULE *wava_module_output_load(char *name);
extern WAVAMODULE *wava_module_filter_load(char *name);
extern WAVAMODULE *wava_module_path_load(char *path);
extern void wava_module_free(WAVAMODULE *module);

/**
 * name is the user-specified module name
 * prefix is the application-specified module name prefix
 * root_prefix is the directory from where the module is
 * supposed to be loaded (and it must include the last
 * bracket)
 **/
extern WAVAMODULE *wava_module_custom_load(char *name,
        const char *prefix, const char *root_prefix);

// get the path of a loaded module
extern const char *wava_module_path_get(WAVAMODULE *module);

// get the file extension of a module on the current system
extern const char *wava_module_extension_get(void);

// return module generated filename into result
extern void wava_module_generate_filename(char *name,
        const char *prefix, char *result);


#endif

