#ifndef __WAVA_OUTPUT_SHARED_CAIRO_H
#define __WAVA_OUTPUT_SHARED_CAIRO_H

#include <cairo.h>
#include "shared.h"
#include "util/feature_compat.h"
#include "util/module.h"

typedef struct wava_cairo_handle {
    WAVA *wava;
    cairo_t            *cr; // name used by a lot of docs, so I'm going with it

    XG_EVENT_STACK     *events;

    wava_cairo_module  *modules;
    WAVA_CAIRO_FEATURE  feature_level;
} wava_cairo_handle;

wava_cairo_handle *__internal_wava_output_cairo_load_config(
                                        WAVA *wava);
void               __internal_wava_output_cairo_init(wava_cairo_handle *handle,
                                                     cairo_t *cr);
void              __internal_wava_output_cairo_apply(wava_cairo_handle *handle);
XG_EVENT_STACK   *__internal_wava_output_cairo_event(wava_cairo_handle *handle);
void              __internal_wava_output_cairo_draw(wava_cairo_handle *handle);
void              __internal_wava_output_cairo_clear(wava_cairo_handle *handle);
void              __internal_wava_output_cairo_cleanup(wava_cairo_handle *handle);

#endif //__WAVA_OUTPUT_SHARED_CAIRO_H

