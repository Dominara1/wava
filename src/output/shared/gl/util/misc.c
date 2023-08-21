#include <math.h>

#include "misc.h"

float wava_gl_module_util_calculate_intensity(WAVA *wava) {
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

    return intensity;
}

float wava_gl_module_util_obtain_time(void) {
    return (float)fmodl((long double)wavaGetTime()/(long double)1000.0, 3600.0);
}

