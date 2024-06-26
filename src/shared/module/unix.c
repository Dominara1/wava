#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>

#include "shared.h"
#include "shared/io/unix.h"

typedef struct wavamodule {
    char *name;
    void **moduleHandle;
    char *path;
} WAVAMODULE;

char *LIBRARY_EXTENSION = ".so";

EXP_FUNC void wava_module_free(WAVAMODULE *module) {
    dlclose(module->moduleHandle);
    module->moduleHandle = 0;
    free(module->name);
    free(module->path);
    free(module);
}

EXP_FUNC WAVAMODULE *wava_module_load(char *name) {
    #ifdef UNIX_INDEPENDENT_PATHS
        char *prefix = find_prefix();
    #else
        char *prefix = PREFIX;
    #endif

    // make this static size :)
    char new_name[MAX_PATH];

    // Security check
    for(size_t i=0; i<strlen(name); i++) {
        // Disallow directory injections
        if(name[i] == '/') return NULL;
    }

    // regardless, try the local module first, as it has priority
    // over normal ones
    snprintf(new_name, MAX_PATH, "./%s%s", name, LIBRARY_EXTENSION);

    // check if the thing even exists
    FILE *fp = fopen(new_name, "r");
    if(fp == NULL) {
        snprintf(new_name, MAX_PATH, "%s/lib/wava/%s%s",
                prefix, name, LIBRARY_EXTENSION);

        // lower the name, because users
        size_t str_len = strlen(prefix) + strlen("/lib/wava/");
        for(size_t i=str_len; i<strlen(new_name); i++)
            new_name[i] = tolower(new_name[i]);
    } else fclose(fp);

    WAVAMODULE *module = (WAVAMODULE*) malloc(sizeof(WAVAMODULE));
    module->moduleHandle = dlopen(new_name, RTLD_NOW);
    module->name = strdup(name);

    // don't ask, this is unexplainable C garbage at work again
    module->path = strdup(new_name);

    wavaLog("Module loaded '%s' loaded at %p",
        module->name, module->moduleHandle);

    #ifdef UNIX_INDEPENDENT_PATHS
        //free(prefix);
    #endif

    return module;
}

// because of stupidity this is not a entire path instead,
// it's supposed to take in the path without the extension
//
// the extension gets added here, just as a FYI
EXP_FUNC WAVAMODULE *wava_module_path_load(char *path) {
    size_t offset;
    for(offset = strlen(path); offset > 0; offset--) {
        if(path[offset-1] == '/')
            break;
    }

    char *new_name = &path[offset];
    char full_path[strlen(path)+strlen(LIBRARY_EXTENSION)+1];
    strcpy(full_path, path);
    strcat(full_path, LIBRARY_EXTENSION);

    WAVAMODULE *module = (WAVAMODULE*) malloc(sizeof(WAVAMODULE));
    module->moduleHandle = dlopen(full_path, RTLD_NOW);
    module->name = strdup(new_name);
    module->path = strdup(full_path);

    wavaLog("Module loaded '%s' loaded at %p",
        module->name, module->moduleHandle);

    return module;
}

EXP_FUNC char *wava_module_error_get(WAVAMODULE *module) {
    UNUSED(module);
    return dlerror();
}

EXP_FUNC void *wava_module_symbol_address_get(WAVAMODULE *module, char *symbol) {
    void *address = dlsym(module->moduleHandle, symbol);

    // the program would crash with an NULL pointer error anyway
    wavaBailCondition(!address, "Failed to find symbol '%s' in module '%s'",
        symbol, module->name);

    wavaLog("Symbol '%s' of '%s' found at: %p",
            symbol, module->name, address);

    return address;
}

EXP_FUNC bool wava_module_valid(WAVAMODULE *module) {
    if(module->moduleHandle)
        return 1;
    else
        return 0;
}

EXP_FUNC const char *wava_module_path_get(WAVAMODULE *module) {
    // prevent NULL-pointer exception
    if(module == NULL)
        return NULL;

    return module->path;
}

EXP_FUNC const char *wava_module_extension_get(void) {
    return LIBRARY_EXTENSION;
}

EXP_FUNC void wava_module_generate_filename(char *name,
        const char *prefix, char *result) {
    sprintf(result, "%s_%s%s", prefix, name, LIBRARY_EXTENSION);
    return;
}
