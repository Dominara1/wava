#include "module.h"
#include "region.h"
#include "feature_compat.h"
#include "shared.h"

WAVA_CAIRO_FEATURE wava_cairo_module_check_compatibility(wava_cairo_module *modules) {
    WAVA_CAIRO_FEATURE lowest_common_denominator =
        WAVA_CAIRO_FEATURE_DRAW_REGION_SAFE |
        WAVA_CAIRO_FEATURE_DRAW_REGION |
        WAVA_CAIRO_FEATURE_FULL_DRAW;

    // check whether there are feature misalignments with modules
    for(size_t i = 0; i < arr_count(modules); i++) {
        wavaReturnErrorCondition(!wava_cairo_feature_compatibility_assert(modules[i].features),
                0, "Module sanity check failed for \'%s\'", modules[i].name);
        lowest_common_denominator &= modules[i].features;
    }
    wavaReturnErrorCondition(lowest_common_denominator == 0,
            0, "Modules do not have inter-compatible drawing methods!\n",
            "Please report this to the developer WHILST including your "
            "current configuration.");

    // probably will never happen but who knows ¯\_ (ツ)_/¯
    wavaReturnSpamCondition(lowest_common_denominator & WAVA_CAIRO_FEATURE_DRAW_REGION_SAFE,
            WAVA_CAIRO_FEATURE_DRAW_REGION_SAFE,
            "Skipping region bounds checking as all of them are safe!");

    // if regions arent supported, full draws MUST be
    if((lowest_common_denominator & WAVA_CAIRO_FEATURE_DRAW_REGION) == 0) {
        wavaReturnSpamCondition(lowest_common_denominator & WAVA_CAIRO_FEATURE_FULL_DRAW,
                WAVA_CAIRO_FEATURE_FULL_DRAW,
                "Skipping region bounds checking as all of them are drawing the frame anyway!");

        wavaBail("No common drawing mode exists. Most likely a BUG!");
    }

    bool pass = true;
    for(size_t i = 0; i < arr_count(modules)-1; i++) {
        for(size_t j = i+1; j < arr_count(modules); j++) {
            if(wava_cairo_region_list_check(modules[i].regions,
                        modules[j].regions)) {
                pass = false;
            }
        }
    }

    if(pass == false) {
        wavaBailCondition(
                (lowest_common_denominator & WAVA_CAIRO_FEATURE_FULL_DRAW) == 0,
                "No common drawing mode exists since no regions match. RIP.");
        return WAVA_CAIRO_FEATURE_FULL_DRAW;
    }

    return WAVA_CAIRO_FEATURE_DRAW_REGION;
}
