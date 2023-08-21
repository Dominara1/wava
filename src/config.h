#ifndef H_CONFIG
#define H_CONFIG
    // This is changed by the CMake build file, don't touch :)
    #ifndef WAVA_DEFAULT_INPUT
        #define WAVA_DEFAULT_INPUT "pulseaudio"
    #endif
    #ifndef WAVA_DEFAULT_OUTPUT
        #define WAVA_DEFAULT_OUTPUT "opengl"
    #endif
    #ifndef WAVA_DEFAULT_FILTER
        #define WAVA_DEFAULT_FILTER "default"
    #endif

    #include "shared.h"

    char *load_config(char *configPath, WAVA*);
    void clean_config();
#endif
