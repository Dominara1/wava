#include <math.h>
#include <cairo.h>

#include "output/shared/cairo/modules/shared/config.h"

#include "output/shared/cairo/util/module.h"
#include "output/shared/graphical.h"
#include "shared.h"

struct options {
    // noop
} options;

wava_config_source *config_file;
WAVAIONOTIFY       file_notifications;

// ionotify fun
EXP_FUNC void wava_cairo_module_ionotify_callback
                (WAVA_IONOTIFY_EVENT event,
                const char* filename,
                int id,
                WAVA* wava) {
    UNUSED(filename);
    UNUSED(id);
    wava_cairo_module_handle *handle = (wava_cairo_module_handle*)wava;
    switch(event) {
        case WAVA_IONOTIFY_CHANGED:
            // trigger restart
            pushWAVAEventStack(handle->events, WAVA_RELOAD);
            break;
        default:
            // noop
            break;
    }
}

// report version
EXP_FUNC wava_version wava_cairo_module_version(void) {
    return wava_version_host_get();
}

// load all the necessary config data and report supported drawing modes
EXP_FUNC WAVA_CAIRO_FEATURE wava_cairo_module_config_load(wava_cairo_module_handle* handle) {
    WAVA *wava = handle->wava;
    WAVA_CONFIG *conf = &wava->conf;

    char config_file_path[MAX_PATH];
    config_file = wava_cairo_module_file_load(
            WAVA_CAIRO_FILE_CONFIG, handle, "config.ini", config_file_path);

    //options.option = wavaConfigGetBool(*config_file, "oscilloscope", "option", false);

    // setup file notifications
    file_notifications = wavaIONotifySetup();

    WAVAIONOTIFYWATCHSETUP setup;
    MALLOC_SELF(setup, 1);
    setup->filename           = config_file_path;
    setup->id                 = 1;
    setup->wava_ionotify_func = wava_cairo_module_ionotify_callback;
    setup->wava               = (WAVA*) handle;
    setup->ionotify           = file_notifications;
    wavaIONotifyAddWatch(setup);

    wavaIONotifyStart(file_notifications);

    free(setup);

    // input MUST be stereo
    conf->stereo = true;
    conf->flag.skipFilter = true;

    return WAVA_CAIRO_FEATURE_FULL_DRAW;
}

EXP_FUNC void               wava_cairo_module_init(wava_cairo_module_handle* handle) {
    UNUSED(handle);
    //WAVA *wava = handle->wava;
    //WAVA_CONFIG *conf = &wava->conf;

    // potentially set if no other modules require it
    //conf->skipFilterF = true;

    wavaWarn("Just a heads up: THIS IS GOING TO LAG! Blame cairo.");
}

EXP_FUNC void               wava_cairo_module_apply(wava_cairo_module_handle* handle) {
    UNUSED(handle);
}

// report drawn regions
EXP_FUNC wava_cairo_region* wava_cairo_module_regions(wava_cairo_module_handle* handle) {
    WAVA *wava = handle->wava;

    wava_cairo_region *regions;
    arr_init(regions);

    wava_cairo_region region;
    region.x = wava->inner.x;
    region.y = wava->inner.y;
    region.w = wava->inner.w;
    region.h = wava->inner.h;
    arr_add(regions, region);

    return regions;
}

// event handler
EXP_FUNC void               wava_cairo_module_event      (wava_cairo_module_handle* handle) {
    UNUSED(handle);
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
    WAVA_AUDIO *audio = &wava->audio;

    // setting new settings because default are WAY too slow
    cairo_antialias_t old_aa        = cairo_get_antialias(handle->cr);
    double            old_tolerance = cairo_get_tolerance(handle->cr);
    cairo_operator_t  old_operator  = cairo_get_operator(handle->cr);

    cairo_set_antialias(handle->cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_tolerance(handle->cr, 1.0);
    cairo_set_operator(handle->cr, CAIRO_OPERATOR_SOURCE);

    cairo_set_source_rgba(handle->cr,
            ARGB_R_32(conf->col)/255.0,
            ARGB_G_32(conf->col)/255.0,
            ARGB_B_32(conf->col)/255.0,
            conf->foreground_opacity);

    cairo_set_line_width(handle->cr, 1.0);

    // optimization strats
    float scale_x = (float)wava->inner.w / INT16_MAX;
    float scale_y = (float)wava->inner.h / INT16_MIN;
    float trans_x = wava->inner.x + wava->inner.w/2.0;
    float trans_y = wava->inner.y + wava->inner.h/2.0;

    // optimized to shit loop
    double x1, y1, x2, y2;
    x1 = scale_x * audio->audio_out_l[0] + trans_x;
    y1 = scale_y * audio->audio_out_r[0] + trans_y;

    for(size_t i = 1; i < (size_t)audio->inputsize; i++) {
        x2 = scale_x * audio->audio_out_l[i] + trans_x;
        y2 = scale_y * audio->audio_out_r[i] + trans_y;

        if(i == 0)
            cairo_move_to(handle->cr, x1, y1);

        cairo_line_to(handle->cr, x2, y2);

        // swap
        x1 = x2;
        y1 = y2;
    }

    cairo_stroke(handle->cr);

    cairo_set_antialias(handle->cr, old_aa);
    cairo_set_tolerance(handle->cr, old_tolerance);
    cairo_set_operator(handle->cr, old_operator);
}

EXP_FUNC void               wava_cairo_module_cleanup    (wava_cairo_module_handle* handle) {
    UNUSED(handle);
    wavaIONotifyKill(file_notifications);

    wavaConfigClose(*config_file);
    free(config_file); // a fun hacky quirk because bad design
}

