#include <math.h>
#include <cairo.h>
#include "output/shared/cairo/util/module.h"
#include "output/shared/graphical.h"
#include "shared.h"

// report version
EXP_FUNC wava_version wava_cairo_module_version(void) {
    return wava_version_host_get();
}

// load all the necessary config data and report supported drawing modes
EXP_FUNC WAVA_CAIRO_FEATURE wava_cairo_module_config_load(wava_cairo_module_handle* handle) {
    UNUSED(handle);
    return WAVA_CAIRO_FEATURE_FULL_DRAW;
}

EXP_FUNC void               wava_cairo_module_init(wava_cairo_module_handle* handle) {
    UNUSED(handle);
}

EXP_FUNC void               wava_cairo_module_apply(wava_cairo_module_handle* handle) {
    UNUSED(handle);
}

// report drawn regions
EXP_FUNC wava_cairo_region* wava_cairo_module_regions(wava_cairo_module_handle* handle) {
    UNUSED(handle);
    return NULL;
}

// event handler
EXP_FUNC void               wava_cairo_module_event      (wava_cairo_module_handle* handle) {
    WAVA *wava = handle->wava;

    // check if the visualizer bounds were changed
    if((wava->inner.w != wava->bar_space.w) ||
       (wava->inner.h != wava->bar_space.h)) {
        wava->bar_space.w = wava->inner.w;
        wava->bar_space.h = wava->inner.h;
        pushWAVAEventStack(handle->events, WAVA_RESIZE);
    }
}

// placeholder, as it literally does nothing atm
EXP_FUNC void               wava_cairo_module_clear      (wava_cairo_module_handle* handle) {
    UNUSED(handle);
}

EXP_FUNC void               wava_cairo_module_draw_region(wava_cairo_module_handle* handle) {
    UNUSED(handle);
}

// no matter what condition, this ensures a safe write
EXP_FUNC void               wava_cairo_module_draw_safe  (wava_cairo_module_handle* handle) {
    UNUSED(handle);
}

// assume that the entire screen's being overwritten
EXP_FUNC void               wava_cairo_module_draw_full  (wava_cairo_module_handle* handle) {
    WAVA   *wava = handle->wava;
    WAVA_CONFIG *conf = &wava->conf;

    cairo_set_source_rgba(handle->cr,
            ARGB_R_32(conf->col)/255.0,
            ARGB_G_32(conf->col)/255.0,
            ARGB_B_32(conf->col)/255.0,
            conf->foreground_opacity);

    float intensity = 0.0;

    for(register uint32_t i=0; i<wava->bars; i++) {
        // the not so, speed part
        // intensity has a low-freq bias as they are more "physical"
        float bar_percentage = (float)(wava->f[i]-1)/(float)conf->h;
        if(bar_percentage > 0.0) {
            intensity+=powf(bar_percentage,
                    (float)2.0*(float)i/(float)wava->bars);
        }
    }

    // since im not bothering to do the math, this'll do
    // - used to balance out intensity across various number of bars
    intensity /= wava->bars;

    cairo_set_source_rgba(handle->cr,
            ARGB_R_32(conf->bgcol)/255.0,
            ARGB_G_32(conf->bgcol)/255.0,
            ARGB_B_32(conf->bgcol)/255.0,
            conf->background_opacity*(1.0-intensity));
    cairo_set_operator(handle->cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(handle->cr);
}

EXP_FUNC void               wava_cairo_module_cleanup    (wava_cairo_module_handle* handle) {
    UNUSED(handle);
}

// ionotify fun
EXP_FUNC void         wava_cairo_module_ionotify_callback
                (WAVA_IONOTIFY_EVENT event,
                const char* filename,
                int id,
                WAVA* wava) {
    UNUSED(event);
    UNUSED(filename);
    UNUSED(id);
    UNUSED(wava);
}
