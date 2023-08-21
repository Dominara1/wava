#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shared.h"
#include "shared/module/internal.h"

#if defined(__WIN32__)
    #define DIR_BREAK '\\'
#else
    #define DIR_BREAK '/'
#endif

EXP_FUNC WAVAMODULE *wava_module_output_load(char *name) {
    char *new_name = calloc(strlen(name)+strlen("out_") + 1, sizeof(char));
    sprintf(new_name, "out_%s", name);
    WAVAMODULE *module = wava_module_load(new_name);
    free(new_name);
    return module;
}

EXP_FUNC WAVAMODULE *wava_module_input_load(char *name) {
    char *new_name = calloc(strlen(name)+strlen("in_") + 1, sizeof(char));
    sprintf(new_name, "in_%s", name);
    WAVAMODULE *module = wava_module_load(new_name);
    free(new_name);
    return module;
}

EXP_FUNC WAVAMODULE *wava_module_filter_load(char *name) {
    char *new_name = calloc(strlen(name)+strlen("filter_") + 1, sizeof(char));
    sprintf(new_name, "filter_%s", name);
    WAVAMODULE *module = wava_module_load(new_name);
    free(new_name);
    return module;
}

EXP_FUNC WAVAMODULE *wava_module_custom_load(char *name, const char *prefix,
        const char *root_prefix) {
    char string_size = strlen(root_prefix) + 1 + strlen(prefix) + 1 +
        strlen(name) + 1;
    char path[string_size];

    // a kinda security feature
    for(size_t i=0; i<strlen(name); i++) {
        wavaBailCondition(name[i] == DIRBRK,
            "Directory injections are NOT allowed within the module name!");
    }

    //wavaBailCondition(root_prefix[strlen(root_prefix)-1] != DIRBRK,
    //    "Bug detected: The dev SHOULD'VE included the ending bracket at the "
    //    "root_prefix parameter. Please report this!");

    snprintf(path, string_size-1, "%s%s_%s",
        root_prefix, prefix, name);
    return wava_module_path_load(path);
}

