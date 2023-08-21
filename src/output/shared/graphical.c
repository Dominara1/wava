#include <string.h>

#include "graphical.h"

#define SET_INNER_X(val) \
    wava->inner.x = ((wava->outer.w == wava->inner.w)? 0 : (val))
#define SET_INNER_Y(val) \
    wava->inner.y = ((wava->outer.h == wava->inner.h)? 0 : (val))

void __internal_wava_graphical_calculate_win_pos_keep(WAVA *wava,
                                            uint32_t winW, uint32_t winH) {
    WAVA_CONFIG *conf = &wava->conf;

    wava->outer.w = winW;
    wava->outer.h = winH;
    wava->inner.w = conf->w;
    wava->inner.h = conf->h;
    // skip resetting the outer x, as those can be troublesome
    wava->inner.x = 0;
    wava->inner.y = 0;

    if(wava->outer.w <= conf->w)
        wava->inner.w = wava->outer.w;
    if(wava->outer.h <= conf->h)
        wava->inner.h = wava->outer.h;

    if(!strcmp(conf->winA, "top")) {
        SET_INNER_X((winW - conf->w) / 2 + conf->x);
        SET_INNER_Y(0                    + conf->y);
    } else if(!strcmp(conf->winA, "bottom")) {
        SET_INNER_X((winW - conf->w) / 2 + conf->x);
        SET_INNER_Y(winH - conf->h       - conf->y);
    } else if(!strcmp(conf->winA, "top_left")) {
        SET_INNER_X(0                    + conf->x);
        SET_INNER_Y(0                    + conf->y);
    } else if(!strcmp(conf->winA, "top_right")) {
        SET_INNER_X(winW - conf->w       - conf->x);
        SET_INNER_Y(0                    + conf->y);
    } else if(!strcmp(conf->winA, "left")) {
        SET_INNER_X(0                    + conf->x);
        SET_INNER_Y((winH - conf->h) / 2 + conf->y);
    } else if(!strcmp(conf->winA, "right")) {
        SET_INNER_X(winW - conf->w       - conf->x);
        SET_INNER_Y((winH - conf->h) / 2 + conf->y);
    } else if(!strcmp(conf->winA, "bottom_left")) {
        SET_INNER_X(0                    + conf->x);
        SET_INNER_Y(winH - conf->h       - conf->y);
    } else if(!strcmp(conf->winA, "bottom_right")) {
        SET_INNER_X(winW - conf->w       - conf->x);
        SET_INNER_Y(winH - conf->h       - conf->y);
    } else if(!strcmp(conf->winA, "center")) {
        SET_INNER_X((winW - conf->w) / 2 + conf->x);
        SET_INNER_Y((winH - conf->h) / 2 + conf->y);
    }
}

void __internal_wava_graphical_calculate_win_pos_nokeep(WAVA *wava,
                                        uint32_t winW, uint32_t winH) {
    wava->outer.w = winW;
    wava->outer.h = winH;
    wava->inner.w = winW;
    wava->inner.h = winH;
    wava->inner.x = 0;
    wava->inner.y = 0;
}

void calculate_win_geo(WAVA *wava, uint32_t winW, uint32_t winH) {
    if(wava->conf.flag.holdSize) {
        __internal_wava_graphical_calculate_win_pos_keep(wava, winW, winH);
    } else {
        wava->outer.w = winW;
        wava->outer.h = winH;
        wava->inner.w = winW;
        wava->inner.h = winH;
        // skip resetting the outer x, as those can be troublesome
        wava->inner.x = 0;
        wava->inner.y = 0;
    }
}

void calculate_win_pos(WAVA *wava, uint32_t scrW, uint32_t scrH,
                        uint32_t winW, uint32_t winH) {
    WAVA_CONFIG *conf = &wava->conf;

    wava->outer.x = 0;
    wava->outer.y = 0;

    if(wava->conf.flag.holdSize) {
        if(wava->conf.flag.fullscreen) {
            __internal_wava_graphical_calculate_win_pos_keep(wava, scrW, scrH);
        } else {
            __internal_wava_graphical_calculate_win_pos_keep(wava, winW, winH);
        }
    } else {
        if(wava->conf.flag.fullscreen) {
            __internal_wava_graphical_calculate_win_pos_nokeep(wava, scrW, scrH);
        } else {
            __internal_wava_graphical_calculate_win_pos_nokeep(wava, winW, winH);
        }
    }

    if(wava->conf.flag.fullscreen == false) {
        if(!strcmp(conf->winA, "top")) {
            wava->outer.x = (int32_t)(scrW - winW) / 2 + conf->x;
        } else if(!strcmp(conf->winA, "bottom")) {
            wava->outer.x = (int32_t)(scrW - winW) / 2 + conf->x;
            wava->outer.y = (int32_t)(scrH - winH)     - conf->y;
        } else if(!strcmp(conf->winA, "top_left")) {
            // noop
        } else if(!strcmp(conf->winA, "top_right")) {
            wava->outer.x = (int32_t)(scrW - winW)     - conf->x;
        } else if(!strcmp(conf->winA, "left")) {
            wava->outer.y = (int32_t)(scrH - winH) / 2;
        } else if(!strcmp(conf->winA, "right")) {
            wava->outer.x = (int32_t)(scrW - winW)     - conf->x;
            wava->outer.y = (int32_t)(scrH - winH) / 2 + conf->y;
        } else if(!strcmp(conf->winA, "bottom_left")) {
            wava->outer.y = (int32_t)(scrH - winH)     - conf->y;
        } else if(!strcmp(conf->winA, "bottom_right")) {
            wava->outer.x = (int32_t)(scrW - winW)     - conf->x;
            wava->outer.y = (int32_t)(scrH - winH)     - conf->y;
        } else if(!strcmp(conf->winA, "center")) {
            wava->outer.x = (int32_t)(scrW - winW) / 2 + conf->x;
            wava->outer.y = (int32_t)(scrH - winH) / 2 + conf->y;
        }
    }

    // Some error checking
    wavaLogCondition(wava->outer.x > (int)(scrW-winW),
            "Screen out of bounds (X axis) (%d %d %d %d)",
            scrW, scrH, wava->outer.x, winW);
    wavaLogCondition(wava->outer.y > (int)(scrH-winH),
            "Screen out of bounds (Y axis) (%d %d %d %d)",
            scrW, scrH, wava->outer.y, winH);
}

