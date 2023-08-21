#ifndef __WAVA_SHARED_H
#define __WAVA_SHARED_H

#include <stdlib.h>
#include <stdbool.h>

// Audio sensitivity and volume varies greatly between
// different audio, audio systems and operating systems
// This value is used to properly calibrate the sensitivity
// for a certain platform or setup in the Makefile
#ifndef WAVA_PREDEFINED_SENS_VALUE
    #define WAVA_PREDEFINED_SENS_VALUE 0.0005
#endif

// Some common comparision macros
#ifndef MIN
    #define MIN(x,y) ((x)>(y) ? (y):(x))
#endif
#ifndef MAX
    #define MAX(x,y) ((x)>(y) ? (x):(y))
#endif
#ifndef DIFF
    #define DIFF(x,y) (MAX((x),(y))-MIN((x),(y)))
#endif

#define UNUSED(x) (void)(x)

#define CALLOC_SELF(x, y) (x)=calloc((y), sizeof(*x))
#define MALLOC_SELF(x, y)  (x)=malloc(sizeof(*x)*(y))
#define REALLOC_SELF(x, y) { void (*z)=realloc((x), sizeof(*x)*(y)); \
    wavaBailCondition(!(z), "Failed to reallocate memory"); (x)=(z); }

#ifdef __WIN32__
    #define EXP_FUNC __declspec(dllexport) __attribute__ ((visibility ("default")))
#else
    #define EXP_FUNC __attribute__ ((visibility ("default")))
#endif

#include "shared/util/types.h"
// this array thing is so useful, let's just force it upon everything. thank liv
#include "shared/util/array.h"
#include "shared/module/module.h"
#include "shared/log.h"
#include "shared/ionotify.h"
#include "shared/config/config.h"
#include "shared/io/io.h"
#include "shared/util/version.h"


// configuration parameters
typedef struct WAVA_CONFIG {
    // for internal use only
    WAVA_CONFIG_OPTION(f64,  sens);
    WAVA_CONFIG_OPTION(i32,  fixedbars);
    WAVA_CONFIG_OPTION(bool, autobars);
    WAVA_CONFIG_OPTION(bool, stereo);
    WAVA_CONFIG_OPTION(bool, autosens);

    WAVA_CONFIG_OPTION(u32, fftsize);

    // input/output related options

    // 1 - colors
    WAVA_CONFIG_OPTION(char*, color);
    WAVA_CONFIG_OPTION(char*, bcolor);           // pointer to color string
    // col = foreground color, bgcol = background color
    WAVA_CONFIG_OPTION(u32, col);
    WAVA_CONFIG_OPTION(u32, bgcol);                 // ARGB 32-bit value
    WAVA_CONFIG_OPTION(f64, foreground_opacity);
    WAVA_CONFIG_OPTION(f64, background_opacity);    // range 0.0-1.0

    // 2 - gradients
    WAVA_CONFIG_OPTION(char**, gradients); // array of pointers to color string

    // 3 - timing
    WAVA_CONFIG_OPTION(i32, framerate); // limit wava to a specific framerate
                                        // (can be changed mid-run)
    WAVA_CONFIG_OPTION(i32, vsync);     // 0 = disabled, while enabled, WAVA
                                        // will rely upon the timer function
                                        // provided by your Vsync call
                                        // (PLEASE DESIGN YOUR IMPLEMENTATION
                                        // LIKE THIS)
                                        // Positive values = divisors for your
                                        // monitors real refresh-rate
                                        // -1 = Adaptive Vsync

    // 4 - geometry
    WAVA_CONFIG_OPTION(u32, bw);
    WAVA_CONFIG_OPTION(u32, bs);        // bar width and spacing
    WAVA_CONFIG_OPTION(u32, w);
    WAVA_CONFIG_OPTION(u32, h);         // configured window width and height
    WAVA_CONFIG_OPTION(i32, x);
    WAVA_CONFIG_OPTION(i32, y);         // x and y padding

    WAVA_CONFIG_OPTION(char*, winA);    // pointer to a string of alignment

    // 5 - audio
    WAVA_CONFIG_OPTION(u32, inputsize); // size of the input audio buffer
                                                    // must be a power of 2
    WAVA_CONFIG_OPTION(u32, samplerate);    // the rate at which the audio is sampled
    WAVA_CONFIG_OPTION(u32, samplelatency); // input will try to keep to copy chunks of this size

    // 6 - special flags
    struct {
        WAVA_CONFIG_OPTION(bool, fullscreen);
        WAVA_CONFIG_OPTION(bool, transparency);
        WAVA_CONFIG_OPTION(bool, border);
        WAVA_CONFIG_OPTION(bool, beneath);
        WAVA_CONFIG_OPTION(bool, interact);
        WAVA_CONFIG_OPTION(bool, taskbar);
        WAVA_CONFIG_OPTION(bool, holdSize);

        // not real config options, soom to be moved to the WAVA handle
        WAVA_CONFIG_OPTION(bool, skipFilter);
        WAVA_CONFIG_OPTION(bool, ignoreWindowSize);
    } flag;
} WAVA_CONFIG;

typedef struct WAVA_FILTER {
    struct {
        void (*load_config) (WAVA*);
        int  (*init)        (WAVA*);
        int  (*apply)       (WAVA*);
        int  (*loop)        (WAVA*);
        int  (*cleanup)     (WAVA*);
    } func;

    WAVAMODULE *module;
    // i know void pointers are footguns but consider that this
    // pointer type is GURANTEED to be mutable
    void       *data;
} WAVA_FILTER;

typedef struct WAVA_OUTPUT {
    struct {
        void     (*load_config)  (WAVA*);
        int      (*init)         (WAVA*);
        void     (*clear)        (WAVA*);
        int      (*apply)        (WAVA*);
        XG_EVENT (*handle_input) (WAVA*);
        void     (*draw)         (WAVA*);
        void     (*cleanup)      (WAVA*);
    } func;

    WAVAMODULE *module;
    void       *data;
} WAVA_OUTPUT;

// Shared audio data sturct
typedef struct WAVA_AUDIO {
    struct {
        void     (*load_config) (WAVA*);
        void*    (*loop)        (void*);
    } func;

    WAVAMODULE *module;
    void       *data;

    float        *audio_out_r;
    float        *audio_out_l;
    int32_t      format;
    uint32_t     rate;
    char         *source;               // alsa device, fifo path or pulse source
    uint32_t     channels;
    bool         terminate;             // shared variable used to terminate audio thread
    char         error_message[1024];
    uint32_t     inputsize, fftsize;    // inputsize and fftsize
    uint32_t     latency;               // try to keep (this) latency in samples
} WAVA_AUDIO;

// WAVA handle
typedef struct WAVA {
    // variables that WAVA outputs
    uint32_t bars;        // number of output bars
    uint32_t rest;        // number of screen units until first bar
    uint32_t *f, *fl;    // array to bar data (f = current, fl = last frame)

    // signals to the renderer thread to safely stop rendering (if needed)
    bool pauseRendering;

    // handles to both config variables and the audio state
    WAVA_AUDIO  audio;   // TODO: rename to input when brave enough
    WAVA_FILTER filter;
    WAVA_OUTPUT output;

    WAVA_CONFIG conf;

    struct config {
        // handle to the config file itself
        wava_config_source        config;
    } default_config;

    // visualizer size INSIDE of the window
    struct dimensions {
        int32_t  x, y; // display offset in the visualizer window
        uint32_t w, h; // window dimensions
    } inner, outer;

    // since inner screen space may not match with bar space
    // this exists to bridge that gap
    struct bar_space {
        uint32_t w, h;
    } bar_space;

    WAVAIONOTIFY ionotify;
} WAVA;

#endif
