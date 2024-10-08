#include <string.h>

#include "output/shared/gl/main.h"
#include "post.h"
#include "output/shared/gl/util/misc.h"
#include "output/shared/graphical.h"
#include "util/shader.c"
#include "util/misc.c"

// postVertices
static const GLfloat post_vertices[16] = {
    -1.0f, -1.0f,  // Position 0
     0.0f,  0.0f,  // TexCoord 0
     1.0f, -1.0f,  // Position 1
     1.0f,  0.0f,  // TexCoord 1
     1.0f,  1.0f,  // Position 2
     1.0f,  1.0f,  // TexCoord 2
    -1.0f,  1.0f,  // Position 3
     0.0f,  1.0f}; // TexCoord 3

void wava_gl_module_post_update_intensity(WAVAGLHostOptions *vars) {
    // update intensity
    float intensity = wava_gl_module_util_calculate_intensity(vars->wava);
    glUniform1f(vars->gl_vars.INTENSITY, intensity);
}

void wava_gl_module_post_update_time(WAVAGLHostOptions *vars) {
    float currentTime = wava_gl_module_util_obtain_time();

    // update time
    glUniform1f(vars->gl_vars.TIME, currentTime);
}

void wava_gl_module_post_update_colors(WAVAGLHostOptions *vars) {
    float fgcol[4] = {
        ARGB_R_32(vars->wava->conf.col)/255.0,
        ARGB_G_32(vars->wava->conf.col)/255.0,
        ARGB_B_32(vars->wava->conf.col)/255.0,
        vars->wava->conf.foreground_opacity
    };

    float bgcol[4] = {
        ARGB_R_32(vars->wava->conf.bgcol)/255.0,
        ARGB_G_32(vars->wava->conf.bgcol)/255.0,
        ARGB_B_32(vars->wava->conf.bgcol)/255.0,
        vars->wava->conf.background_opacity
    };

    // set and attach foreground color
    glUniform4f(vars->gl_vars.FGCOL, fgcol[0], fgcol[1], fgcol[2], fgcol[3]);

    // set background clear color
    glUniform4f(vars->gl_vars.BGCOL, bgcol[0], bgcol[1], bgcol[2], bgcol[3]);
}

void wava_gl_module_post_config_load(WAVAGLHostOptions *vars) {
    WAVA               *wava   = vars->wava;
    wava_config_source  config = wava->default_config.config;

    WAVA_CONFIG_OPTION(char*, shader);

    WAVA_CONFIG_GET_STRING(config, "gl", "post_shader", "default", shader);
    if(strcmp("none", shader)){
      wavaLog("Post shader '%s' enabled", shader);
        // HACK: (ab)using the 1st modules inotify for the post shaders reload function
        wava_gl_module_shader_load(&vars->post, SGL_POST, SGL_VERT, shader,
                &vars->module[0], wava);
        wava_gl_module_shader_load(&vars->post, SGL_POST, SGL_FRAG, shader,
                &vars->module[0], wava);
        wava_gl_module_shader_load(&vars->post, SGL_POST, SGL_CONFIG, shader,
                &vars->module[0], wava);
        vars->post_enabled = true;

        vars->features = 0;
        if(wavaConfigGetBool(vars->post.config, "features", "colors", false)) {
            vars->features |= GL_MODULE_POST_COLORS;
        }
        if(wavaConfigGetBool(vars->post.config, "features", "time", false)) {
            vars->features |= GL_MODULE_POST_TIME;
        }
        if(wavaConfigGetBool(vars->post.config, "features", "intensity", false)) {
            vars->features |= GL_MODULE_POST_INTENSITY;
        }
    } else {
        vars->post_enabled = false;
    }

}

void wava_gl_module_post_init(WAVAGLHostOptions *vars) {
    if(!vars->post_enabled) return;

    struct gl_vars *gl = &vars->gl_vars;
    wava_gl_module_program *post = &vars->post;

    wava_gl_module_program_create(&vars->post);

    gl->POS        = glGetAttribLocation (post->program, "v_texCoord");
    gl->TEXCOORD   = glGetAttribLocation (post->program, "m_texCoord");
    gl->TEXTURE    = glGetUniformLocation(post->program, "color_texture");
    gl->DEPTH      = glGetUniformLocation(post->program, "depth_texture");
    gl->RESOLUTION = glGetUniformLocation(post->program, "resolution");

    gl->DOTS       = glGetAttribLocation (post->program, "v_dots");

    if(vars->features & GL_MODULE_POST_TIME)
        gl->TIME       = glGetUniformLocation(post->program, "time");
    if(vars->features & GL_MODULE_POST_INTENSITY)
        gl->INTENSITY  = glGetUniformLocation(post->program, "intensity");
    if(vars->features & GL_MODULE_POST_COLORS) {
        gl->FGCOL      = glGetUniformLocation(post->program, "foreground_color");
        gl->BGCOL      = glGetUniformLocation(post->program, "background_color");
    }
}

void wava_gl_module_post_apply(WAVAGLHostOptions *vars) {
    if(!vars->post_enabled) return;

    WAVA              *wava = vars->wava;
    struct gl_vars    *gl   = &vars->gl_vars;

    glUseProgram(vars->post.program);

    // update screen resoltion
    glUniform2f(gl->RESOLUTION, wava->outer.w, wava->outer.h);

    // set texture properties
    glGenTextures(1,              &vars->FBO.final_texture);
    glBindTexture(GL_TEXTURE_2D,   vars->FBO.final_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
            wava->outer.w*vars->resolution_scale,
            wava->outer.h*vars->resolution_scale,
            0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // set texture properties
    glGenTextures(1,             &vars->FBO.depth_texture);
    glBindTexture(GL_TEXTURE_2D,  vars->FBO.depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
            wava->outer.w*vars->resolution_scale,
            wava->outer.h*vars->resolution_scale,
            0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // set framebuffer properties
    glGenFramebuffers(1,  &vars->FBO.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, vars->FBO.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, vars->FBO.final_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D, vars->FBO.depth_texture, 0);

    // check if it borked
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    wavaBailCondition(status != GL_FRAMEBUFFER_COMPLETE,
            "Failed to create framebuffer(s)! Error code 0x%X\n",
            status);
}

void wava_gl_module_post_pre_draw_setup(WAVAGLHostOptions *vars) {
    WAVA              *wava = vars->wava;

    if(vars->post_enabled) {
        // bind render target to texture
        glBindFramebuffer(GL_FRAMEBUFFER, vars->FBO.framebuffer);
        glViewport(0, 0,
                wava->outer.w*vars->resolution_scale,
                wava->outer.h*vars->resolution_scale);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, wava->outer.w, wava->outer.h);
    }
}

void wava_gl_module_post_draw(WAVAGLHostOptions *vars) {
    if(!vars->post_enabled) return;

    WAVA              *wava = vars->wava;

    /**
     * Once the texture has been conpleted, we now activate a seperate pipeline
     * which just displays that texture to the actual framebuffer for easier
     * shader writing
     * */
    // Change framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, wava->outer.w, wava->outer.h);

    // Switch to post shaders
    glUseProgram(vars->post.program);

    // update variables
    if(vars->features & GL_MODULE_POST_COLORS)
        wava_gl_module_post_update_colors(vars);
    if(vars->features & GL_MODULE_POST_TIME)
        wava_gl_module_post_update_time(vars);
    if(vars->features & GL_MODULE_POST_INTENSITY)
        wava_gl_module_post_update_intensity(vars);

    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glPointSize(100.0);

    // set attribute array pointers for the texture unit and vertex unit
    glVertexAttribPointer(vars->gl_vars.POS, 2, GL_FLOAT,
        GL_FALSE, 4 * sizeof(GLfloat),  post_vertices);
    glVertexAttribPointer(vars->gl_vars.TEXCOORD, 2, GL_FLOAT,
        GL_FALSE, 4 * sizeof(GLfloat), &post_vertices[2]);

    // enable the use of the following attribute elements
    glEnableVertexAttribArray(vars->gl_vars.POS);
    glEnableVertexAttribArray(vars->gl_vars.TEXCOORD);

    // Bind the textures
    // Render texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, vars->FBO.final_texture);
    glUniform1i(vars->gl_vars.TEXTURE, 0);

    // Depth texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, vars->FBO.depth_texture);
    glUniform1i(vars->gl_vars.DEPTH, 1);

    // draw frame
    GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

    glColor4f(1.0, 1.0, 1.0, 1.0);
    glDrawArrays(GL_POINTS, 0, 6);

    glDisableVertexAttribArray(vars->gl_vars.POS);
    glDisableVertexAttribArray(vars->gl_vars.TEXCOORD);
}

void wava_gl_module_post_cleanup(WAVAGLHostOptions *vars) {
    if(vars->post_enabled) return;

    wava_gl_module_program_destroy(&vars->post);
}

