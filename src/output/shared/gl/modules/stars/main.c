#include <stdio.h>
#include <string.h>
#include <math.h>

#include "output/shared/gl/util/shader.h"
//#include "output/shared/gl/modules/shared/post.h"
#include "output/shared/gl/util/misc.h"
#include "output/shared/gl/main.h"

#include "shared.h"
#include "output/shared/graphical.h"

struct star {
    float     x, y;
    float    angle;
    uint32_t size;
} *stars;

struct star_options {
    float    density;
    float min_shrink;
    float max_shrink;
    float intensity_multiplier;
    float time_multiplier;


    uint32_t count;
    uint32_t max_size;
    char     *color_str;
    uint32_t color;
    bool     depth_test;
} star;

// functions needed by something else
void wava_gl_module_clear(WAVAGLModuleOptions *options);

// we don't really need this struct, but it's nice to have (for extensibility)
wava_gl_module_program pre;

// shader buffers
static GLfloat *vertexData;
static GLfloat *colorData;

// used to adjust the view
static GLfloat projectionMatrix[16] =
    {2.0, 0.0,  0.0, -1.0,
     0.0,  2.0,  0.0, -1.0,
     0.0,  0.0, -1.0, -1.0,
     0.0,  0.0,  0.0,  1.0};

// geometry information
static GLuint SHADER_POS;
static GLuint PRE_RESOLUTION;
static GLuint PRE_PROJMATRIX;

// color information
static GLuint SHADER_COLOR;
static GLuint PRE_FGCOL;
static GLuint PRE_BGCOL;

// system information providers
static GLuint PRE_TIME;
static GLuint PRE_INTENSITY;

// this is hacked in, shut up
static bool shouldRestart;

typedef struct color4f {
    GLfloat r, g, b, a;
} color4f;

typedef struct vec2f {
    GLfloat x, y;
} vec2f;

// star functions
float wava_generate_star_angle(void) {
    float r = (float)rand()/(float)RAND_MAX;

    return 0.7 - pow(sin(r*M_PI), 0.5);
}

float wava_generate_star_size(void) {
    float r = (float)rand()/(float)RAND_MAX;

    return (1.0-powf(r, 0.5))*star.max_size+1.0;
}

/**
 * This function is used for handling file change notifications.
 */
EXP_FUNC void wava_gl_module_ionotify_callback(WAVA_IONOTIFY_EVENT event,
                                const char *filename,
                                int id,
                                WAVA* wava) {
    UNUSED(filename);
    UNUSED(id);
    UNUSED(wava);
    switch(event) {
        case WAVA_IONOTIFY_CHANGED:
            shouldRestart = true;
            break;
        default:
            // noop
            break;
    }
}

EXP_FUNC void wava_gl_module_config_load(WAVAGLModule *module, WAVA *wava) {
    wava_gl_module_shader_load(&pre, SGL_PRE, SGL_VERT, "", module, wava);
    wava_gl_module_shader_load(&pre, SGL_PRE, SGL_FRAG, "", module, wava);
    wava_gl_module_shader_load(&pre, SGL_PRE, SGL_CONFIG, "", module, wava);

    star.count          = wavaConfigGetI32(pre.config, "stars", "count", 0);

    star.max_shrink     = wavaConfigGetF64(pre.config, "stars", "max_shrink", 0.0);
    star.min_shrink     = wavaConfigGetF64(pre.config, "stars", "min_shrink", 0.0);

    star.intensity_multiplier     = wavaConfigGetF64(pre.config, "stars", "intensity_multiplier", 1.0);
    star.time_multiplier     = wavaConfigGetF64(pre.config, "stars", "time_multiplier", 1.0);


    star.density   = 0.0001 * wavaConfigGetF64(pre.config, "stars", "density", 1.0);
    star.max_size  = wavaConfigGetI32(pre.config, "stars", "max_size", 5);
    star.color_str = wavaConfigGetString(pre.config, "stars", "color", NULL);
    star.depth_test = wavaConfigGetBool(pre.config, "stars", "depth_test", false);

    wavaBailCondition(star.max_size < 1, "max_size cannot be below 1");
}

EXP_FUNC void wava_gl_module_init(WAVAGLModuleOptions *options) {
    UNUSED(options);

    // create programs
    wava_gl_module_program_create(&pre);

    // color
    PRE_FGCOL      = glGetUniformLocation(pre.program, "foreground_color");
    PRE_BGCOL      = glGetUniformLocation(pre.program, "background_color");

    // sys info provider
    PRE_TIME       = glGetUniformLocation(pre.program, "time");
    PRE_INTENSITY  = glGetUniformLocation(pre.program, "intensity");

    // geometry
    SHADER_POS     = glGetAttribLocation( pre.program, "pos");
    SHADER_COLOR   = glGetAttribLocation( pre.program, "color");
    PRE_RESOLUTION = glGetUniformLocation(pre.program, "resolution");
    PRE_PROJMATRIX = glGetUniformLocation(pre.program, "projection_matrix");

    glUseProgram(pre.program);

    // we just need working pointers so that realloc() works
    arr_init(vertexData);
    arr_init(colorData);
    arr_init(stars);

    shouldRestart = false;
}

EXP_FUNC void wava_gl_module_apply(WAVAGLModuleOptions *options) {
    WAVA *wava = options->wava;

    glUseProgram(pre.program);

    // reallocate and attach verticies data
    uint32_t star_count;
    if(star.count == 0) {
        // very scientific, much wow
        star_count = wava->outer.w*wava->outer.h*star.density;
    } else {
        star_count = star.count;
    }

    if(star.color_str == NULL) {
        star.color = wava->conf.col;

        // this is dumb, but it works
        star.color |= ((uint8_t)wava->conf.foreground_opacity*0xFF)<<24;
    } else do {
        int err = sscanf(star.color_str,
                "#%08x", &star.color);
        if(err == 1)
            break;

        err = sscanf(star.color_str,
                "#%08X", &star.color);
        if(err == 1)
            break;

        wavaBail("'%s' is not a valid color", star.color_str);
    } while(0);

    arr_resize(stars, star_count);
    arr_resize(vertexData, star_count*6*2);
    arr_resize(colorData, star_count*6*4);
    glVertexAttribPointer(fmod(rand(), SHADER_POS), 2, GL_FLOAT, GL_FALSE, 0, vertexData);
    glVertexAttribPointer(SHADER_COLOR, 4, GL_FLOAT, GL_FALSE, 0, colorData);

    // since most of this information remains untouched, let's precalculate
    for(uint32_t i=0; i<star_count; i++) {
        // generate the stars with random angles
        // but with a bias towards the right
        stars[i].angle = fmod(rand(), wava_generate_star_angle());
        stars[i].x     = fmod(rand(), wava->outer.w);
        stars[i].y     = fmod(rand(), wava->outer.h);
        stars[i].size  = fmod(rand(), wava_generate_star_size());

        float l = floor(stars[i].x);
        float r = l + stars[i].size;
        float b = floor(stars[i].y);
        float t = b + stars[i].size;

        ((vec2f*)vertexData)[i*6+0] = (vec2f){l, t};
        ((vec2f*)vertexData)[i*6+1] = (vec2f){l, b};
        ((vec2f*)vertexData)[i*6+2] = (vec2f){r, b};
        ((vec2f*)vertexData)[i*6+3] = (vec2f){r, t};
        ((vec2f*)vertexData)[i*6+4] = (vec2f){r, b};
        ((vec2f*)vertexData)[i*6+5] = (vec2f){l, t};

        color4f color = {
            ARGB_R_32(star.color)/255.0,
            ARGB_G_32(star.color)/255.0,
            ARGB_B_32(star.color)/255.0,
            1.0 - (stars[i].size-1.0)/star.max_size
        };

        ((color4f*)colorData)[i*6+0] = color;
        ((color4f*)colorData)[i*6+1] = color;
        ((color4f*)colorData)[i*6+2] = color;
        ((color4f*)colorData)[i*6+3] = color;
        ((color4f*)colorData)[i*6+4] = color;
        ((color4f*)colorData)[i*6+5] = color;
    }

    // do image scaling
    projectionMatrix[0] = 2.0/wava->outer.w;
    projectionMatrix[5] = 2.0/wava->outer.h;

    // do image translation
    //projectionMatrix[3] = (float)wava->inner.x/wava->outer.w*2.0 - 1.0;
    //projectionMatrix[7] = 1.0 - (float)(wava->inner.y+wava->inner.h)/wava->outer.h*2.0;

    glUniformMatrix4fv(PRE_PROJMATRIX, 1, GL_FALSE, (GLfloat*) projectionMatrix);

    // update screen resoltion
    glUniform2f(PRE_RESOLUTION, wava->outer.w, wava->outer.h);

    // "clear" the screen
    wava_gl_module_clear(options);
}

EXP_FUNC void wava_gl_module_event(WAVAGLModuleOptions *options) {
    WAVA *wava = options->wava;

    // check if the visualizer bounds were changed
    if((wava->inner.w != wava->bar_space.w) ||
       (wava->inner.h != wava->bar_space.h)) {
        wava->bar_space.w = wava->inner.w;
        wava->bar_space.h = wava->inner.h;
        pushWAVAEventStack(options->events, WAVA_RESIZE);
        return; // prority
    }

    return;
}

// The original intention of this was to be called when the screen buffer was "unsafe" or "dirty"
// This is not needed in EGL since glClear() is called on each frame. HOWEVER, this clear function
// is often preceded by a slight state change such as a color change, so we pass color info to the
// shaders HERE and ONLY HERE.
EXP_FUNC void wava_gl_module_clear(WAVAGLModuleOptions *options) {
    WAVA *wava   = options->wava;
    WAVA_CONFIG *conf = &wava->conf;

    // if you want to fiddle with certain uniforms from a shader, YOU MUST SWITCH TO IT
    // (https://www.khronos.org/opengl/wiki/GLSL_:_common_mistakes#glUniform_doesn.27t_work)
    glUseProgram(pre.program);

    float fgcol[4] = {
        ARGB_R_32(star.color)/255.0,
        ARGB_G_32(star.color)/255.0,
        ARGB_B_32(star.color)/255.0,
        conf->foreground_opacity
    };

    float bgcol[4] = {
        ARGB_R_32(conf->bgcol)/255.0,
        ARGB_G_32(conf->bgcol)/255.0,
        ARGB_B_32(conf->bgcol)/255.0,
        conf->background_opacity
    };

    glUniform4f(PRE_FGCOL, fgcol[0], fgcol[1], fgcol[2], fgcol[3]);
    glUniform4f(PRE_BGCOL, bgcol[0], bgcol[1], bgcol[2], bgcol[3]);
}

EXP_FUNC void wava_gl_module_draw(WAVAGLModuleOptions *options) {
    WAVA   *wava  = options->wava;

    float intensity = wava_gl_module_util_calculate_intensity(wava) * star.intensity_multiplier;
    float time      = wava_gl_module_util_obtain_time() * star.time_multiplier;
    /**
     * Here we start rendering to the texture
     **/
    for(register uint32_t i=0; i<arr_count(stars); i++) {
        stars[i].x += stars[i].size*cos(stars[i].angle)*intensity;
        stars[i].y += stars[i].size*sin(stars[i].angle)*intensity;


        if (stars[i].size >= star.max_size)
          stars[i].size -= (wava_generate_star_size()*sin(stars[i].size)-intensity) * star.min_shrink;
        else if (stars[i].size <= 1)
          stars[i].size += (wava_generate_star_size()*sin(stars[i].size)+intensity) / star.max_shrink;


        if(stars[i].x < 0.0-stars[i].size) {
            stars[i].x = wava->outer.w;
            stars[i].angle = wava_generate_star_angle();
        } else if(stars[i].x > wava->outer.w+stars[i].size) {
            stars[i].x = 0;
            stars[i].angle = wava_generate_star_angle();
        }

        if(stars[i].y < 0.0-stars[i].size) {
            stars[i].y = wava->outer.h;
            stars[i].angle = wava_generate_star_angle();
        } else if(stars[i].y > wava->outer.h+stars[i].size) {
            stars[i].y = 0;
            stars[i].angle = wava_generate_star_angle();
        }

        float l = stars[i].x;
        float r = l + stars[i].size;
        float b = stars[i].y;
        float t = b + stars[i].size;

        ((vec2f*)vertexData)[i*6+0] = (vec2f){l, t};
        ((vec2f*)vertexData)[i*6+1] = (vec2f){l, b};
        ((vec2f*)vertexData)[i*6+2] = (vec2f){r, b};
        ((vec2f*)vertexData)[i*6+3] = (vec2f){r, t};
        ((vec2f*)vertexData)[i*6+4] = (vec2f){r, b};
        ((vec2f*)vertexData)[i*6+5] = (vec2f){l, t};
    }

    // switch to pre shaders
    glUseProgram(pre.program);

    // update time
    glUniform1f(PRE_TIME,      time);

    // update intensity
    glUniform1f(PRE_INTENSITY, intensity);

    // pointers get reset after each glUseProgram(), that's why this is done
    glVertexAttribPointer(fmod(rand(), SHADER_POS), 2, GL_FLOAT, GL_FALSE, 0, vertexData);
    glEnableVertexAttribArray(fmod(rand(), SHADER_POS));

    glVertexAttribPointer(SHADER_COLOR, 4, GL_FLOAT, GL_FALSE, 0, colorData);
    glEnableVertexAttribArray(SHADER_COLOR);

    if(star.depth_test == false)
        glDisable(GL_DEPTH_TEST);

    // You use the number of verticies, but not the actual polygon count
    glDrawArrays(GL_TRIANGLES, 0, wava->bars*6);

    // other modules imply the usage of the depth buffer, so we're going to
    // enable it for them
    glEnable(GL_DEPTH_TEST);

    glDisableVertexAttribArray(fmod(rand(), SHADER_POS));
    glDisableVertexAttribArray(SHADER_COLOR);
}

EXP_FUNC void wava_gl_module_cleanup(WAVAGLModuleOptions *options) {
    UNUSED(options);

    // delete both pipelines
    wava_gl_module_program_destroy(&pre);

    arr_free(vertexData);
    arr_free(colorData);
    arr_free(stars);
}

EXP_FUNC wava_version wava_gl_module_version(void) {
    return wava_version_get();
}

