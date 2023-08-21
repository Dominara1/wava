#ifndef __GL_MODULE_SHARED_POST_H
#define __GL_MODULE_SHARED_POST_H

#include "output/shared/gl/main.h"

void wava_gl_module_post_config_load(WAVAGLHostOptions *vars);
void wava_gl_module_post_init(WAVAGLHostOptions *vars);
void wava_gl_module_post_apply(WAVAGLHostOptions *vars);
void wava_gl_module_post_clear(WAVAGLHostOptions *vars);
void wava_gl_module_post_pre_draw_setup(WAVAGLHostOptions *vars);
void wava_gl_module_post_draw(WAVAGLHostOptions *vars);
void wava_gl_module_post_cleanup(WAVAGLHostOptions *vars);
#endif

