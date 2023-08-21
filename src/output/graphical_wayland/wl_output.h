#ifndef __WAYLAND_WL_OUTPUT_H
#define __WAYLAND_WL_OUTPUT_H

#include "gen/xdg-output-unstable-v1-client-protocol.h"
#include "gen/wlr-layer-shell-unstable-v1-client-protocol.h"

#include "main.h"

struct wlOutput {
    struct wl_output      *output;
    uint32_t              scale;

    // im not calling this a "name" because that's **fucking** retarded
    uint32_t              id;

    uint32_t              width;
    uint32_t              height;
    char                  *name;
    uint32_t              num;
    struct zxdg_output_v1 *xdg_output;
    struct wl_list        link;
};

extern struct zxdg_output_manager_v1   *wavaXDGOutputManager;
extern struct zxdg_output_v1_listener  xdg_output_listener;
extern const struct wl_output_listener output_listener;

struct wlOutput *wl_output_get_desired();
extern void wl_output_cleanup(struct waydata *wd);
extern void wl_output_init(struct waydata *wd);

#endif
