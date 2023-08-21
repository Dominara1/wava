#ifndef __WAVA_OUTPUT_SHARED_CAIRO_UTIL_MODULE_H
#define __WAVA_OUTPUT_SHARED_CAIRO_UTIL_MODULE_H

#include <stdbool.h>
#include <cairo.h>
#include "shared.h"
#include "feature_compat.h"
#include "region.h"

typedef struct wava_cairo_module_handle {
    WAVA_CAIRO_FEATURE use_feature;
    char               *name;
    char               *prefix;
    WAVA               *wava;
    cairo_t            *cr;
    XG_EVENT_STACK     *events;
} wava_cairo_module_handle;

typedef struct wava_cairo_module {
    char *name;
    char *prefix;
    WAVA_CAIRO_FEATURE features;
    wava_cairo_region  *regions;

    wava_cairo_module_handle config;

    struct functions {
        // report version
        wava_version       (*version)    (void);

        // load all the necessary config data and report supported drawing modes
        WAVA_CAIRO_FEATURE (*config_load)(wava_cairo_module_handle*);
        void               (*init)       (wava_cairo_module_handle*);
        void               (*apply)      (wava_cairo_module_handle*);

        // report drawn regions
        wava_cairo_region* (*regions)    (wava_cairo_module_handle*);

        // event handler (events are stored in a variable in the handle)
        void               (*event)      (wava_cairo_module_handle*);

        // placeholder
        void               (*clear)      (wava_cairo_module_handle*);

        // only used with draw_region
        void               (*draw_region)(wava_cairo_module_handle*);

        // no matter what condition, this ensures a safe write
        void               (*draw_safe)  (wava_cairo_module_handle*);

        // assume that the entire screen's being overwritten
        void               (*draw_full)  (wava_cairo_module_handle*);

        void               (*cleanup)    (wava_cairo_module_handle*);

        // ionotify fun
        void         (*ionotify_callback)
                        (WAVA_IONOTIFY_EVENT,
                        const char* filename,
                        int id,
                        WAVA*);
    } func;

    // module handle for fun
    WAVAMODULE *handle;
} wava_cairo_module;

WAVA_CAIRO_FEATURE wava_cairo_module_check_compatibility(wava_cairo_module *modules);
#endif // __WAVA_OUTPUT_SHARED_CAIRO_UTIL_MODULE_H

