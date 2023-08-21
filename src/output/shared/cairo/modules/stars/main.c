#include <stdio.h>
#include <limits.h>
#include <math.h>

#include <cairo.h>

#include "output/shared/cairo/modules/shared/config.h"

#include "output/shared/cairo/util/module.h"
#include "output/shared/graphical.h"
#include "shared.h"

struct star {
    float x, y;
    float angle;
    uint32_t size;
} *stars;

struct options {
    struct star_options {
        float    density;
        uint32_t count;
        uint32_t max_size;
        char     *color_str;
        uint32_t color;
    } star;
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

    options.star.count     = wavaConfigGetI32(*config_file, "stars", "count", 0);
    options.star.density   = 0.0001 *
        wavaConfigGetF64(*config_file, "stars", "density", 1.0);
    options.star.max_size  = wavaConfigGetI32(*config_file, "stars", "max_size", 5);
    options.star.color_str = wavaConfigGetString(*config_file, "stars", "color", NULL);

    wavaBailCondition(options.star.max_size < 1, "max_size cannot be below 1");

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

    return WAVA_CAIRO_FEATURE_FULL_DRAW;
}

EXP_FUNC void               wava_cairo_module_init(wava_cairo_module_handle* handle) {
    UNUSED(handle);
    arr_init(stars);
}

float wava_generate_star_angle(void) {
    float r = (float)rand()/(float)RAND_MAX;

    return 0.7 - pow(sin(r*M_PI), 0.5);
}

uint32_t wava_generate_star_size(void) {
    float r = (float)rand()/(float)RAND_MAX;

    return floor((1.0-pow(r, 0.5))*options.star.max_size)+1;
}

EXP_FUNC void               wava_cairo_module_apply(wava_cairo_module_handle* handle) {
    WAVA *wava = handle->wava;

    uint32_t star_count;
    if(options.star.count == 0) {
        // very scientific, much wow
        star_count = wava->outer.w*wava->outer.h*options.star.density;
    } else {
        star_count = options.star.count;
    }

    arr_resize(stars, star_count);

    for(uint32_t i = 0; i < star_count; i++) {
        // generate the stars with random angles
        // but with a bias towards the right
        stars[i].angle = wava_generate_star_angle();
        stars[i].x     = fmod(rand(), wava->outer.w);
        stars[i].y     = fmod(rand(), wava->outer.h);
        stars[i].size  = wava_generate_star_size();
    }

    if(options.star.color_str == NULL) {
        options.star.color = wava->conf.col;

        // this is dumb, but it works
        options.star.color |= ((uint8_t)wava->conf.foreground_opacity*0xFF)<<24;
    } else do {
        int err = sscanf(options.star.color_str,
                "#%08x", &options.star.color);
        if(err == 1)
            break;

        err = sscanf(options.star.color_str,
                "#%08X", &options.star.color);
        if(err == 1)
            break;

        wavaBail("'%s' is not a valid color", options.star.color_str);
    } while(0);
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

    // batching per color because that's fast in cairo
    for(register uint32_t j=1; j<=options.star.max_size; j++) {
        float alpha = (float)(1+options.star.max_size-j)/options.star.max_size;

        cairo_set_source_rgba(handle->cr,
                ARGB_R_32(options.star.color)/255.0,
                ARGB_G_32(options.star.color)/255.0,
                ARGB_B_32(options.star.color)/255.0,
                ARGB_A_32(options.star.color)/255.0*alpha);
        for(register uint32_t i=0; i<arr_count(stars); i++) {
            // we're drawing per star
            if(stars[i].size != j)
                continue;

            stars[i].x += stars[i].size*cos(stars[i].angle)*intensity;
            stars[i].y += stars[i].size*sin(stars[i].angle)*intensity;

            if(stars[i].x < 0.0-stars[i].size) {
                stars[i].x = wava->outer.w;
                stars[i].angle = wava_generate_star_angle();
            } else if(stars[i].x > wava->outer.w+stars[i].size) {
                stars[i].x = 0;
                stars[i].angle = wava_generate_star_angle();
            }

            if(stars[i].y < 0.0-stars[i].size) {
                stars[i].y = wava->outer.h;
                stars[i].angle = wava_generate_star_angle();
            } else if(stars[i].y > wava->outer.h+stars[i].size) {
                stars[i].y = 0;
                stars[i].angle = wava_generate_star_angle();
            }

            cairo_rectangle(handle->cr, stars[i].x, stars[i].y,
                    stars[i].size, stars[i].size);
        }

        // execute batch
        cairo_fill(handle->cr);
    }
}

EXP_FUNC void               wava_cairo_module_cleanup    (wava_cairo_module_handle* handle) {
    UNUSED(handle);
    arr_free(stars);

    wavaIONotifyKill(file_notifications);

    wavaConfigClose(*config_file);
    free(config_file); // a fun hacky quirk because bad design
}

