#include <string.h>
#include <stdlib.h>

#include "gen/xdg-output-unstable-v1-client-protocol.h"
#include "wl_output.h"
#include "main.h"

struct zxdg_output_manager_v1 *wavaXDGOutputManager;

static void output_geometry(void *data, struct wl_output *output, int32_t x,
        int32_t y, int32_t width_mm, int32_t height_mm, int32_t subpixel,
        const char *make, const char *model, int32_t transform) {
    UNUSED(data);
    UNUSED(output);
    UNUSED(x);
    UNUSED(y);
    UNUSED(width_mm);
    UNUSED(height_mm);
    UNUSED(subpixel);
    UNUSED(make);
    UNUSED(model);
    UNUSED(transform);

    // Who cares
}

static void output_mode(void *data, struct wl_output *output, uint32_t flags,
        int32_t width, int32_t height, int32_t refresh) {
    UNUSED(data);
    UNUSED(output);
    UNUSED(flags);
    UNUSED(width);
    UNUSED(height);
    UNUSED(refresh);
    // Who cares
}

static void output_done(void *data, struct wl_output *output) {
    UNUSED(data);
    UNUSED(output);
    // Who cares
}

static void output_scale(void *data, struct wl_output *wl_output,
        int32_t scale) {
    UNUSED(wl_output);

    struct wlOutput *output = data;
    output->scale = scale;
}

const struct wl_output_listener output_listener = {
    .geometry = output_geometry,
    .mode = output_mode,
    .done = output_done,
    .scale = output_scale,
};

void wl_output_cleanup(struct waydata *wd) {
    struct wlOutput *output, *tmp;

    wl_list_for_each_safe(output, tmp, &wd->outputs, link) {
        wavaSpam("Destroying output %s", output->name);
        wl_list_remove(&output->link);
        free(output->name);
        free(output);
    }
}

void wl_output_init(struct waydata *wd) {
    wl_list_init(&wd->outputs);
}

struct wlOutput *wl_output_get_desired(struct waydata *wd) {
    struct wlOutput *output, *tmp, *lastGood;

    lastGood = NULL; // supress warnings by GCC

    wl_list_for_each_safe(output, tmp, &wd->outputs, link) {
        if(!strcmp(output->name, monitorName)) {
            return output;
        } else lastGood = output;
    }

    // in case none are available, just send the last display
    return lastGood;
}


// XDG_OUTPUT_HANDLE

static void xdg_output_handle_logical_position(void *data,
        struct zxdg_output_v1 *xdg_output, int32_t x, int32_t y) {
    UNUSED(data);
    UNUSED(xdg_output);
    UNUSED(x);
    UNUSED(y);
    // Who cares
}

static void xdg_output_handle_logical_size(void *data,
        struct zxdg_output_v1 *xdg_output, int32_t width, int32_t height) {
    UNUSED(xdg_output);

    struct wlOutput *output = data;
    output->width = width;
    output->height = height;
}

static void xdg_output_handle_name(void *data,
        struct zxdg_output_v1 *xdg_output, const char *name) {
    UNUSED(xdg_output);

    struct wlOutput *output = data;
    wavaSpam("Output %s loaded with id %d", name, output->id);
    output->name = strdup(name);
}

static void xdg_output_handle_done(void *data,
        struct zxdg_output_v1 *xdg_output) {
    // Who cares
    UNUSED(data);
    UNUSED(xdg_output);
}

static void xdg_output_handle_description(void *data,
        struct zxdg_output_v1 *xdg_output, const char *description) {
    // Who cares
    UNUSED(data);
    UNUSED(xdg_output);
    UNUSED(description);
}

struct zxdg_output_v1_listener xdg_output_listener = {
    .logical_position = xdg_output_handle_logical_position,
    .logical_size = xdg_output_handle_logical_size,
    .name = xdg_output_handle_name,
    .description = xdg_output_handle_description,
    .done = xdg_output_handle_done,
};

