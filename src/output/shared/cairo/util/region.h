#ifndef __WAVA_OUTPUT_SHARED_CAIRO_REGION_H
#define __WAVA_OUTPUT_SHARED_CAIRO_REGION_H

#include <stdbool.h>
// keeping bounding boxes constrained to well,... rectangles. we can make this
// system way simpler than do to bitmask checks which is stupidly memory
// inefficient and computationally expensive
typedef struct wava_cairo_region {
    int x; // region starting x coordinate
    int y; // region starting y coordinate
    int w; // region starting w coordinate
    int h; // region starting h coordinate
} wava_cairo_region;

// functions
bool wava_cairo_region_list_check(wava_cairo_region *A,
        wava_cairo_region *B);

#endif

