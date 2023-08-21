#include "shared.h"

#ifndef __WAVA_OUTPUT_GL_MAIN_H
#define __WAVA_OUTPUT_GL_MAIN_H

// for static checker sake
#if !defined(GL) && !defined(EGL)
    #define GL
    #warning "This WILL break your build. Fix it!"
#endif

#if defined(EGL)
    #include <EGL/eglplatform.h>
    #include <EGL/egl.h>
#endif

#ifndef GL_ALREADY_DEFINED
    #include <GL/glew.h>
#endif

typedef struct WAVAGLModuleOptions {
    // actual options used by the module
    GLdouble     resolution_scale;
    WAVA         *wava;
    void         (*ionotify_callback)
                        (WAVA_IONOTIFY_EVENT,
                        const char* filename,
                        int id,
                        WAVA*);

    char           *prefix;
    XG_EVENT_STACK *events;
} WAVAGLModuleOptions;

// working around my badly designed includes
typedef struct WAVAGLModule WAVAGLModule;

// shader stuff
typedef struct wava_gl_module_program {
    struct shader {
        char *path, *text;
        GLuint handle;
    } frag, vert, geo;
    GLuint     program;
    wava_config_source config;
} wava_gl_module_program;

// post render stuff
typedef enum gl_module_post_render_features {
    GL_MODULE_POST_INTENSITY = 1,
    GL_MODULE_POST_COLORS    = 2,
    GL_MODULE_POST_TIME      = 4,
} gl_module_post_render_features;

struct WAVAGLModule {
    char               *name;
    char               *prefix;
    WAVAMODULE         *handle;

    struct functions {
        wava_version (*version)(void);
        void         (*config_load)(WAVAGLModule*,WAVA*);
        void         (*init)(WAVAGLModuleOptions*);
        void         (*apply)(WAVAGLModuleOptions*);
        void         (*event)(WAVAGLModuleOptions*);
        /**
         * events are returned through the options->events thingy
         **/
        void         (*clear)(WAVAGLModuleOptions*);
        void         (*draw)(WAVAGLModuleOptions*);
        void         (*cleanup)(WAVAGLModuleOptions*);
        void         (*ionotify_callback)
                        (WAVA_IONOTIFY_EVENT,
                        const char* filename,
                        int id,
                        WAVA*);
    } func;

    WAVAGLModuleOptions options;
};

typedef struct WAVAGLHostOptions {
    WAVAGLModule         *module;
    WAVA_CONFIG_OPTION(f64, resolution_scale);
    XG_EVENT_STACK       *events;
    WAVA                 *wava;

    struct FBO {
        GLuint framebuffer;
        GLuint final_texture;
        GLuint depth_texture;
    } FBO;

    // post shader variables
    struct gl_vars {
        // geometry info
        GLuint POS;
        GLuint TEXCOORD;
        GLuint RESOLUTION;
        GLuint DOTS;

        // textures
        GLuint TEXTURE;
        GLuint DEPTH;

        // system info
        GLuint TIME;
        GLuint INTENSITY;

        // color information
        GLuint FGCOL;
        GLuint BGCOL;
    } gl_vars;

    wava_gl_module_program post;
    gl_module_post_render_features features;
    bool post_enabled;
} WAVAGLHostOptions;

void           SGLConfigLoad(WAVA *wava);
void           SGLInit(WAVA *wava);
void           SGLApply(WAVA *wava);
XG_EVENT_STACK *SGLEvent(WAVA *wava);
void           SGLClear(WAVA *wava);
void           SGLDraw(WAVA *wava);
void           SGLCleanup(WAVA *wava);

#endif

