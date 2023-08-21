#include "feature_compat.h"

// check for logic errors in the feature_compat list
bool wava_cairo_feature_compatibility_assert(WAVA_CAIRO_FEATURE a) {
    if(a&WAVA_CAIRO_FEATURE_DRAW_REGION_SAFE) {
        if(!(a&WAVA_CAIRO_FEATURE_DRAW_REGION)) return false;
        if(!(a&WAVA_CAIRO_FEATURE_FULL_DRAW)) return false;
    }
    if(a == 0) return false;
    return true;
}
