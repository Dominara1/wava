#include <limits.h>
#include <unistd.h>
#include <wayland-client-core.h>

#include "output/shared/graphical.h"

#include "gen/xdg-shell-client-protocol.h"

#include "xdg.h"
#include "main.h"
#ifdef EGL
    #include "egl.h"
#endif
#ifdef SHM
    #include "shm.h"
#endif
#ifdef CAIRO
    #include "cairo.h"
#endif

static struct xdg_surface *wavaXDGSurface;
static struct xdg_toplevel *wavaXDGToplevel;

struct xdg_wm_base *wavaXDGWMBase;

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base,
        uint32_t serial) {
    UNUSED(data);
    xdg_wm_base_pong(xdg_wm_base, serial);
}

const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void xdg_toplevel_handle_configure(void *data,
        struct xdg_toplevel *xdg_toplevel, int32_t w, int32_t h,
        struct wl_array *states) {
    UNUSED(xdg_toplevel);
    UNUSED(states);

    struct waydata       *wd   = data;
    WAVA   *wava = wd->hand;
    WAVA_CONFIG *conf = &wava->conf;

    if(w == 0 && h == 0) return;

    if(conf->w != (uint32_t)w && conf->h != (uint32_t)h) {
        // fixme when i get proper monitor support on XDG
        calculate_win_geo(wava, w, h);

        #ifdef EGL
            waylandEGLWindowResize(wd, w, h);
        #endif

        #ifdef SHM
            reallocSHM(wd);
        #endif

        #ifdef CAIRO
            wava_output_wayland_cairo_resize(wd);
        #endif

        pushWAVAEventStack(wd->events, WAVA_REDRAW);
        pushWAVAEventStack(wd->events, WAVA_RESIZE);
    }

    #ifdef SHM
        update_frame(wd);
    #endif
}

static void xdg_toplevel_handle_close(void *data,
        struct xdg_toplevel *xdg_toplevel) {
    struct waydata           *wd   = data;

    UNUSED(xdg_toplevel);

    pushWAVAEventStack(wd->events, WAVA_QUIT);
}

struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_handle_configure,
    .close = xdg_toplevel_handle_close,
};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
        uint32_t serial) {
    UNUSED(data);

    // confirm that you exist to the compositor
    xdg_surface_ack_configure(xdg_surface, serial);
}

const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

void xdg_init(struct waydata *wd) {
    // create window, or "surface" in waland terms
    wavaXDGSurface = xdg_wm_base_get_xdg_surface(wavaXDGWMBase, wd->surface);

    // for those unaware, the compositor baby sits everything that you
    // make, thus it needs a function through which the compositor
    // will manage your application
    xdg_surface_add_listener(wavaXDGSurface, &xdg_surface_listener, wd);

    wavaXDGToplevel = xdg_surface_get_toplevel(wavaXDGSurface);
    xdg_toplevel_set_title(wavaXDGToplevel, "WAVA");
    xdg_toplevel_add_listener(wavaXDGToplevel, &xdg_toplevel_listener, wd);
}

void xdg_cleanup() {
    xdg_toplevel_destroy(wavaXDGToplevel);
    xdg_surface_destroy(wavaXDGSurface);
}
