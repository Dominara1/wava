#ifndef __WAYLAND_REGISTRY_H
#define __WAYLAND_REGISTRY_H

#include <wayland-client.h>

#include "shared.h"

extern struct wl_registry *wavaWLRegistry;
extern const struct wl_registry_listener wava_wl_registry_listener;

#endif
