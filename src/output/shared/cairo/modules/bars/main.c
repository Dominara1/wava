#include <math.h>
#include <cairo.h>

#include "output/shared/cairo/modules/shared/config.h"

#include "output/shared/cairo/util/module.h"
#include "output/shared/graphical.h"
#include "shared.h"

struct options {
    bool mirror;
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
    char config_file_path[MAX_PATH];
    config_file = wava_cairo_module_file_load(
            WAVA_CAIRO_FILE_CONFIG, handle, "config.ini", config_file_path);

    options.mirror = wavaConfigGetBool(*config_file, "bars", "mirror", false);

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

    return WAVA_CAIRO_FEATURE_FULL_DRAW |
        WAVA_CAIRO_FEATURE_DRAW_REGION;
}

EXP_FUNC void               wava_cairo_module_init(wava_cairo_module_handle* handle) {
    UNUSED(handle);
}
EXP_FUNC void               wava_cairo_module_apply(wava_cairo_module_handle* handle) {
    UNUSED(handle);
}

// report drawn regions
EXP_FUNC wava_cairo_region* wava_cairo_module_regions(wava_cairo_module_handle* handle) {
    WAVA *wava = handle->wava;
    WAVA_CONFIG *conf = &wava->conf;

    wava_cairo_region *regions;
    arr_init(regions);

    wava_cairo_region region;
    for(size_t i = 0; i < wava->bars; i++) {
        region.x = wava->rest + i*(conf->bs+conf->bw)+wava->inner.x;
        region.y =                                    wava->inner.y;
        region.w = wava->conf.bw;
        region.h = wava->inner.h;
        arr_add(regions, region);
    }

    return regions;
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
    WAVA   *wava = handle->wava;
    WAVA_CONFIG *conf = &wava->conf;

    struct color {
        float r, g, b, a;
    } fg, bg;

    fg.r = ARGB_R_32(conf->col)/255.0;
    fg.g = ARGB_G_32(conf->col)/255.0;
    fg.b = ARGB_B_32(conf->col)/255.0;
    fg.a = conf->foreground_opacity;

    bg.r = ARGB_R_32(conf->bgcol)/255.0;
    bg.g = ARGB_G_32(conf->bgcol)/255.0;
    bg.b = ARGB_B_32(conf->bgcol)/255.0;
    bg.a = conf->background_opacity;

    if(options.mirror) {
        for(size_t i = 0; i < wava->bars; i++) {
            if(wava->f[i] > wava->fl[i]) {
                cairo_set_source_rgba(handle->cr, fg.r, fg.g, fg.b, fg.a);
                cairo_set_operator(handle->cr, CAIRO_OPERATOR_SOURCE);
                int x = wava->rest    + i*(conf->bs+conf->bw) + wava->inner.x;
                int y = ((wava->inner.h - wava->f[i])>>1)     + wava->inner.y;
                int h = wava->f[i]    - wava->fl[i];
                cairo_rectangle(handle->cr, x, y, wava->conf.bw, (h>>1) + 1);

                y = ((wava->inner.h + wava->fl[i])>>1)    + wava->inner.y - 1;
                cairo_rectangle(handle->cr, x, y, wava->conf.bw, (h>>1) + 1);
                cairo_fill(handle->cr);
            } else {
                cairo_set_source_rgba(handle->cr, bg.r, bg.g, bg.b, bg.a);
                cairo_set_operator(handle->cr, CAIRO_OPERATOR_SOURCE);
                int x = wava->rest    + i*(conf->bs+conf->bw) + wava->inner.x;
                int y = ((wava->inner.h - wava->fl[i])>>1)    + wava->inner.y - 1;
                int h = wava->fl[i]   - wava->f[i];
                cairo_rectangle(handle->cr, x, y, wava->conf.bw, (h>>1) + 1);

                y = ((wava->inner.h + wava->f[i])>>1)     + wava->inner.y;
                cairo_rectangle(handle->cr, x, y, wava->conf.bw, (h>>1) + 1);
                cairo_fill(handle->cr);
            }
        }
    } else {
        for(size_t i = 0; i < wava->bars; i++) {
            if(wava->f[i] > wava->fl[i]) {
                cairo_set_source_rgba(handle->cr, fg.r, fg.g, fg.b, fg.a);
                cairo_set_operator(handle->cr, CAIRO_OPERATOR_SOURCE);
                int x = wava->rest    + i*(conf->bs+conf->bw) + wava->inner.x;
                int y = wava->inner.h - wava->f[i]            + wava->inner.y;
                int h = wava->f[i]    - wava->fl[i];
                cairo_rectangle(handle->cr, x, y, wava->conf.bw, h);
                cairo_fill(handle->cr);
            } else {
                cairo_set_source_rgba(handle->cr, bg.r, bg.g, bg.b, bg.a);
                cairo_set_operator(handle->cr, CAIRO_OPERATOR_SOURCE);
                int x = wava->rest    + i*(conf->bs+conf->bw) + wava->inner.x;
                int y = wava->inner.h - wava->fl[i]           + wava->inner.y;
                int h = wava->fl[i]   - wava->f[i];
                cairo_rectangle(handle->cr, x, y, wava->conf.bw, h);
                cairo_fill(handle->cr);
            }
        }
    }

    cairo_save(handle->cr);
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

    if(options.mirror) {
        for(size_t i = 0; i < wava->bars; i++) {
            int x = wava->rest + i*(conf->bs+conf->bw)+wava->inner.x;
            int y = (wava->inner.h - wava->f[i])/2    +wava->inner.y;
            cairo_rectangle(handle->cr, x, y, wava->conf.bw, wava->f[i]);
        }
    } else {
        for(size_t i = 0; i < wava->bars; i++) {
            int x = wava->rest + i*(conf->bs+conf->bw)+wava->inner.x;
            int y = wava->inner.h - wava->f[i]        +wava->inner.y;
            cairo_rectangle(handle->cr, x, y, wava->conf.bw, wava->f[i]);
        }
    }

    cairo_fill(handle->cr);
}

EXP_FUNC void               wava_cairo_module_cleanup    (wava_cairo_module_handle* handle) {
    UNUSED(handle);

    wavaIONotifyKill(file_notifications);

    wavaConfigClose(*config_file);
    free(config_file); // a fun hacky quirk because bad design
}

