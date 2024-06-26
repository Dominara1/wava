#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "util/feature_compat.h"
#include "util/module.h"
#include "util/region.h"
#include "shared.h"
#include "output/shared/graphical.h"

#define LOAD_FUNC_POINTER(name) \
    module->func.name = wava_module_symbol_address_get(module->handle, "wava_cairo_module_" #name); \
    wavaBailCondition(module->func.name == NULL, "wava_cairo_module_" #name " not found!");

// creates flexible instances, which ARE memory managed and must be free-d after use
wava_cairo_handle *__internal_wava_output_cairo_load_config(
        WAVA *wava) {

    wava_config_source config = wava->default_config.config;
    wava_cairo_handle *handle;
    MALLOC_SELF(handle, 1);
    handle->wava = wava;

    arr_init(handle->modules);
    handle->events = newWAVAEventStack();

    // loop until all entries have been read
    char key_name[128];
    uint32_t key_number = 1;
    do {
        snprintf(key_name, 128, "module_%u", key_number);
        char *module_name = wavaConfigGetString(config, "cairo", key_name, NULL);

        // module invalid, probably means that all desired modules are loaded
        if(module_name == NULL)
            break;

        // add module name
        wava_cairo_module module;
        module.name = module_name;
        module.regions = NULL;

        // the part of the function where module path gets figured out
        do {
            char path[MAX_PATH];
            char *returned_path;

            // path gets reset here
            strcpy(path, "cairo/module/");
            strcat(path, module_name);

            // we need the path without the extension for the folder that's going
            // to store our shaders
            module.prefix = strdup(path);

            strcat(path, "/module");

            // prefer local version (ie. build-folder)
            module.handle = wava_module_path_load(path);
            if(wava_module_valid(module.handle))
                break;

            wavaLog("Failed to load '%s' because: '%s'",
                    path, wava_module_error_get(module.handle));

            strcat(path, wava_module_extension_get());
            wavaBailCondition(wavaFindAndCheckFile(WAVA_FILE_TYPE_PACKAGE, path,
                &returned_path) == false, "Failed to open cairo module '%s'", path);

            // remove the file extension because of my bad design
            size_t offset=strlen(returned_path)-strlen(wava_module_extension_get());
            returned_path[offset] = '\0';

            module.handle = wava_module_path_load(returned_path);

            wavaBailCondition(!wava_module_valid(module.handle),
                    "Failed to load cairo module '%s' because: '%s'",
                    returned_path, wava_module_error_get(module.handle));
            free(returned_path);
        } while(0);

        // append loaded module to cairo handle
        arr_add(handle->modules, module);

        // the part of the function where function pointers get loaded per module
        {
            struct wava_cairo_module *module =
                &handle->modules[arr_count(handle->modules)-1];

            LOAD_FUNC_POINTER(version);
            LOAD_FUNC_POINTER(config_load);
            LOAD_FUNC_POINTER(regions);
            LOAD_FUNC_POINTER(init);
            LOAD_FUNC_POINTER(apply);
            LOAD_FUNC_POINTER(event);
            LOAD_FUNC_POINTER(cleanup);
            LOAD_FUNC_POINTER(ionotify_callback);

            module->config.name   = module->name;
            module->config.wava   = wava;
            module->config.prefix = module->prefix;
            module->config.events = handle->events;
            module->features = module->func.config_load(&module->config);

            if(module->features & WAVA_CAIRO_FEATURE_DRAW_REGION) {
                LOAD_FUNC_POINTER(draw_region);
                LOAD_FUNC_POINTER(clear);
            }

            if(module->features & WAVA_CAIRO_FEATURE_DRAW_REGION_SAFE) {
                LOAD_FUNC_POINTER(draw_safe);
            }

            if(module->features & WAVA_CAIRO_FEATURE_FULL_DRAW) {
                LOAD_FUNC_POINTER(draw_full);
            }

            // remember to increment the key number
            key_number++;
        }
    } while(1);

    return handle;
}

void __internal_wava_output_cairo_init(wava_cairo_handle *handle, cairo_t *cr) {
    // add cairo instance to our handle
    handle->cr   = cr;

    for(size_t i = 0; i < arr_count(handle->modules); i++) {
        handle->modules[i].config.cr = cr;
        handle->modules[i].func.init(&handle->modules[i].config);
    }
}

void __internal_wava_output_cairo_apply(wava_cairo_handle *handle) {
    for(size_t i = 0; i < arr_count(handle->modules); i++) {
        wava_cairo_module        *module = &handle->modules[i];
        struct functions         *f      = &module->func;
        wava_cairo_module_handle *config = &module->config;
        if(module->regions != NULL)
            arr_free(module->regions);

        // update cr because wayland's thing requires that the drawing
        // surface be initialized on every resize
        module->config.cr = handle->cr;
        module->regions = f->regions(config);
    }

    WAVA_CAIRO_FEATURE feature_detected =
        wava_cairo_module_check_compatibility(handle->modules);

    if(feature_detected != handle->feature_level) {
        handle->feature_level = feature_detected;
        for(size_t i = 0; i < arr_count(handle->modules); i++) {
            wava_cairo_module *module = &handle->modules[i];

            // do not overwrite as it's used as a reference
            //module->features = handle->feature_level;
            module->config.use_feature = handle->feature_level;
        }
    }

    // run apply functions for the modules
    for(size_t i = 0; i < arr_count(handle->modules); i++) {
        wava_cairo_module *module = &handle->modules[i];
        module->func.apply(&module->config);
    }
}

XG_EVENT_STACK *__internal_wava_output_cairo_event(wava_cairo_handle *handle) {
    // run the module's event handlers
    for(size_t i = 0; i < arr_count(handle->modules); i++) {
        handle->modules[i].func.event(&handle->modules[i].config);
    }

    return handle->events;
}

void __internal_wava_output_cairo_draw(wava_cairo_handle *handle) {
    if(handle->feature_level == WAVA_CAIRO_FEATURE_FULL_DRAW) {
        cairo_set_source_rgba(handle->cr,
                ARGB_R_32(handle->wava->conf.bgcol)/255.0,
                ARGB_G_32(handle->wava->conf.bgcol)/255.0,
                ARGB_B_32(handle->wava->conf.bgcol)/255.0,
                handle->wava->conf.background_opacity);
        cairo_set_operator(handle->cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint(handle->cr);
        cairo_push_group(handle->cr);
    }

    for(size_t i = 0; i < arr_count(handle->modules); i++) {
        struct functions *f = &handle->modules[i].func;
        wava_cairo_module_handle *config = &handle->modules[i].config;
        switch(handle->feature_level) {
            case WAVA_CAIRO_FEATURE_DRAW_REGION:
                f->draw_region(config);
                break;
            case WAVA_CAIRO_FEATURE_DRAW_REGION_SAFE:
                f->draw_safe(config);
                break;
            case WAVA_CAIRO_FEATURE_FULL_DRAW:
                f->draw_full(config);
                break;
            default:
                wavaBail("Something's definitely broken! REPORT THIS.");
                break;
        }
    }

    if(handle->feature_level == WAVA_CAIRO_FEATURE_FULL_DRAW) {
        cairo_pop_group_to_source(handle->cr);
        cairo_fill(handle->cr);
        cairo_paint(handle->cr);
    }
}

void __internal_wava_output_cairo_clear(wava_cairo_handle *handle) {
    // what is even the point of this function when you're already spendin'
    // precious CPU cycles
    if(handle->feature_level == WAVA_CAIRO_FEATURE_FULL_DRAW)
        return;

    // yes we're calling the modules even if the screen gets redrawn anyway
    // because we need to inform them of the redraw
    for(size_t i = 0; i < arr_count(handle->modules); i++) {
        struct wava_cairo_module *module = &handle->modules[i];
        if(WAVA_CAIRO_FEATURE_DRAW_REGION & module->features)
            module->func.clear(&module->config);
    }

    cairo_set_source_rgba(handle->cr,
        ARGB_R_32(handle->wava->conf.bgcol)/255.0,
        ARGB_G_32(handle->wava->conf.bgcol)/255.0,
        ARGB_B_32(handle->wava->conf.bgcol)/255.0,
        handle->wava->conf.background_opacity);
    cairo_set_operator(handle->cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(handle->cr);
}

void __internal_wava_output_cairo_cleanup(wava_cairo_handle *handle) {
    for(size_t i = 0; i < arr_count(handle->modules); i++) {
        handle->modules[i].func.cleanup(&handle->modules[i].config);
        wava_module_free(handle->modules[i].handle);
    }

    arr_free(handle->modules);
    free(handle);
}

