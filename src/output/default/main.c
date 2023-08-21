#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "shared.h"

#ifdef WAYLAND
bool am_i_wayland(void);
#endif

#ifdef X11
bool am_i_x11(void);
#endif

#ifdef WINDOWS
bool am_i_win32(void);
#endif

#ifdef SDL2
bool am_i_sdl2(void);
#endif

typedef struct system {
    bool (*test_func)(void);

    // if these strings are empty, that means that the system is not supported
    char *cairo;
    char *opengl;
} sys;

sys systems[] = {
#ifdef WAYLAND
    { .test_func = am_i_wayland, .cairo = "wayland_cairo", .opengl = "wayland_opengl" },
#endif
#ifdef WINDOWS
    { .test_func = am_i_win32, .cairo = "win_cairo", .opengl = "win_opengl" },
#endif
#ifdef X11
    { .test_func = am_i_x11, .cairo = "x11_cairo", .opengl = "x11_opengl" },
#endif
#ifdef SDL2
    { .test_func = am_i_sdl2, .cairo = NULL, .opengl = "sdl2_opengl" },
#endif
};

struct functions {
    void     (*cleanup)     (WAVA *wava);
    int      (*init)        (WAVA *wava);
    void     (*clear)       (WAVA *wava);
    int      (*apply)       (WAVA *wava);
    XG_EVENT (*handle_input)(WAVA *wava);
    void     (*draw)        (WAVA *wava);
    void     (*load_config) (WAVA *wava);
} functions;

WAVAMODULE *module;

EXP_FUNC void wavaOutputLoadConfig(WAVA *wava) {
    int supported_systems = sizeof(systems)/sizeof(sys);

    char *system = NULL;
    int i = 0;

    wava_output_module_default_find_any_remaining:
    for(; i < supported_systems; i++) {
        if(systems[i].test_func()) {
            #ifdef CAIRO
            system = systems[i].cairo;
            if(system != NULL)
                break;
            #endif
            #ifdef OPENGL
            system = systems[i].opengl;
            if(system != NULL)
                break;
            #endif
        }
    }

    wavaBailCondition(system == NULL,
        "No supported output methods found for '%s'",
    #if defined(CAIRO)
        "cairo"
    #elif defined(OPENGL)
        "opengl"
    #else
        #error "Build broke, pls fix!"
        "wtf"
    #endif
        );

    module = wava_module_output_load(system);
    if(!wava_module_valid(module)) {
        // execution halts here if the condition fails btw
        wavaBailCondition(i == supported_systems-1,
            "wava module failed to load (definitely bug): %s",
            wava_module_error_get(module));

        wavaLog("wava module failed to load (probably bug): %s",
            wava_module_error_get(module));
        goto wava_output_module_default_find_any_remaining;
    }


    functions.cleanup      = wava_module_symbol_address_get(module, "wavaOutputCleanup");
    functions.init         = wava_module_symbol_address_get(module, "wavaInitOutput");
    functions.clear        = wava_module_symbol_address_get(module, "wavaOutputClear");
    functions.apply        = wava_module_symbol_address_get(module, "wavaOutputApply");
    functions.handle_input = wava_module_symbol_address_get(module, "wavaOutputHandleInput");
    functions.draw         = wava_module_symbol_address_get(module, "wavaOutputDraw");
    functions.load_config  = wava_module_symbol_address_get(module, "wavaOutputLoadConfig");

    functions.load_config(wava);
}

EXP_FUNC void wavaOutputCleanup(WAVA *wava) {
    functions.cleanup(wava);

    wava_module_free(module);
}

EXP_FUNC int wavaInitOutput(WAVA *wava) {
    return functions.init(wava);
}

EXP_FUNC void wavaOutputClear(WAVA *wava) {
    functions.clear(wava);
}

EXP_FUNC int wavaOutputApply(WAVA *wava) {
    return functions.apply(wava);
}

EXP_FUNC XG_EVENT wavaOutputHandleInput(WAVA *wava) {
    return functions.handle_input(wava);
}

EXP_FUNC void wavaOutputDraw(WAVA *wava) {
    functions.draw(wava);
}

