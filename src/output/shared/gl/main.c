#include <stdio.h>
#include <string.h>
#include <math.h>

#include "shared.h"

#include "post.h"
#include "main.h"

// eat dick, compiler
#include "post.c"

static WAVAGLHostOptions host;

#define LOAD_FUNC_POINTER(name) \
    module->func.name = wava_module_symbol_address_get(module->handle, "wava_gl_module_" #name); \
    wavaBailCondition(module->func.name == NULL, "wava_gl_module_" #name " not found!");

void SGLConfigLoad(WAVA *wava) {
    wava_config_source config = wava->default_config.config;
    WAVA_CONFIG_GET_F64(config, "gl", "resolution_scale", 1.0f, (&host)->resolution_scale);

    wavaBailCondition(host.resolution_scale <= 0.0f,
        "Resolution scale cannot be under or equal to 0.0");

    arr_init(host.module);

    host.events = newWAVAEventStack();
    host.wava   = wava;

    // loop until all entries have been read
    char key_name[128];
    uint32_t key_number = 1;
    do {
        snprintf(key_name, 128, "module_%u", key_number);
        WAVA_CONFIG_OPTION(char*, module_name);
        WAVA_CONFIG_GET_STRING(config, "gl", key_name, NULL, module_name);

        // module invalid, probably means that all desired modules are loaded
        if(!module_name_is_set_from_file) {
            wavaLog("'%s' has not been set properly!", module_name);
            break;
        }

        // add module name
        WAVAGLModule module;
        module.name = module_name;

        // the part of the function where module path gets figured out
        do {
            char path[MAX_PATH];
            char *returned_path;

            // path gets reset here
            strcpy(path, "gl/module/");
            strcat(path, module_name);

            // we need the path without the extension for the folder that's going
            // to store our shaders
            module.prefix = strdup(path);

            strcat(path, "/module");

            // prefer local version (ie. build-folder)
            module.handle = wava_module_path_load(path);
            if(wava_module_valid(module.handle))
                break;

            wavaLog("Failed to load '%s' because: '%s'",
                    path, wava_module_error_get(module.handle));

            strcat(path, wava_module_extension_get());
            wavaBailCondition(wavaFindAndCheckFile(WAVA_FILE_TYPE_PACKAGE, path,
                &returned_path) == false, "Failed to open gl module '%s'", path);

            // remove the file extension because of my bad design
            size_t offset=strlen(returned_path)-strlen(wava_module_extension_get());
            returned_path[offset] = '\0';

            module.handle = wava_module_path_load(returned_path);

            wavaBailCondition(!wava_module_valid(module.handle),
                    "Failed to load gl module '%s' because: '%s'",
                    returned_path, wava_module_error_get(module.handle));
            free(returned_path);
        } while(0);

        // append loaded module to cairo handle
        arr_add(host.module, module);

        // the part of the function where function pointers get loaded per module
        {
            WAVAGLModule *module =
                &host.module[arr_count(host.module)-1];

            LOAD_FUNC_POINTER(version);
            LOAD_FUNC_POINTER(config_load);
            LOAD_FUNC_POINTER(ionotify_callback);
            LOAD_FUNC_POINTER(init);
            LOAD_FUNC_POINTER(apply);
            LOAD_FUNC_POINTER(event);
            LOAD_FUNC_POINTER(draw);
            LOAD_FUNC_POINTER(clear);
            LOAD_FUNC_POINTER(cleanup);

            module->options.wava             = wava;
            module->options.resolution_scale = host.resolution_scale;
            module->options.prefix           = module->prefix;
            module->options.events           = host.events;

            // remember to increment the key number
            key_number++;

            // check if the module is of a appropriate version
            wava_version_verify(module->func.version());

            // load it's config
            module->func.config_load(module, wava);
        }
    } while(1);

    wava_gl_module_post_config_load(&host);
}

void SGLInit(WAVA *wava) {
    for(uint32_t i = 0; i < arr_count(host.module); i++) {
        WAVAGLModule *module = &host.module[i];

        module->options.wava = wava;
        module->func.init(&module->options);
    }

    wava_gl_module_post_init(&host);
}

void SGLApply(WAVA *wava){
    for(uint32_t i = 0; i < arr_count(host.module); i++) {
        WAVAGLModule *module = &host.module[i];

        module->options.wava = wava;
        module->func.apply(&module->options);
    }

    wava_gl_module_post_apply(&host);
}

XG_EVENT_STACK *SGLEvent(WAVA *wava) {
    for(uint32_t i = 0; i < arr_count(host.module); i++) {
        WAVAGLModule *module = &host.module[i];

        module->options.wava = wava;
        module->func.event(&module->options);
    }

    return host.events;
}

void SGLClear(WAVA *wava) {
    for(uint32_t i = 0; i < arr_count(host.module); i++) {
        WAVAGLModule *module = &host.module[i];

        module->options.wava = wava;
        module->func.clear(&module->options);
    }
}

void SGLDraw(WAVA *wava) {
    glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);
    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_SRC_ALPHA, GL_DST_ALPHA);

    // bind render target to texture
    wava_gl_module_post_pre_draw_setup(&host);

    // clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    for(uint32_t i = 0; i < arr_count(host.module); i++) {
        WAVAGLModule *module = &host.module[i];

        module->options.wava = wava;
        module->func.draw(&module->options);
    }

    // get out the draw function if post shaders aren't enabled
    wava_gl_module_post_draw(&host);
}

void SGLCleanup(WAVA *wava) {
    for(uint32_t i = 0; i < arr_count(host.module); i++) {
        WAVAGLModule *module = &host.module[i];

        module->options.wava = wava;
        module->func.cleanup(&module->options);

        wava_module_free(module->handle);
        //free(module->name);
        free(module->prefix);
    }

    wava_gl_module_post_cleanup(&host);

    arr_free(host.module);
    destroyWAVAEventStack(host.events);
}

