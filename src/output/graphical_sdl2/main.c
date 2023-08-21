#include <time.h>

#include <SDL.h>
#include <SDL_opengl.h>
#define GL_ALREADY_DEFINED

#include "output/shared/gl/glew.h"

#include "output/shared/graphical.h"
#include "shared.h"

SDL_Window *wavaSDLWindow;
SDL_Surface *wavaSDLWindowSurface;
SDL_Event wavaSDLEvent;
SDL_DisplayMode wavaSDLVInfo;

SDL_GLContext wavaSDLGLContext;

EXP_FUNC void wavaOutputCleanup(WAVA *s)
{
    GLCleanup(s);
    SDL_GL_DeleteContext(wavaSDLGLContext);
    SDL_FreeSurface(wavaSDLWindowSurface);
    SDL_DestroyWindow(wavaSDLWindow);
    SDL_Quit();
}

EXP_FUNC int wavaInitOutput(WAVA *s)
{
    WAVA_CONFIG *p = &s->conf;

    wavaBailCondition(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS),
            "Unable to initailize SDL2: %s", SDL_GetError());

    // calculating window x and y position
    wavaBailCondition(SDL_GetCurrentDisplayMode(0, &wavaSDLVInfo),
            "Error opening display: %s", SDL_GetError());
    calculate_win_pos(s, wavaSDLVInfo.w, wavaSDLVInfo.h,
            p->w, p->h);

    // creating a window
    Uint32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
    if(p->flag.fullscreen) windowFlags |= SDL_WINDOW_FULLSCREEN;
    if(!p->flag.border) windowFlags |= SDL_WINDOW_BORDERLESS;
    if(p->vsync) windowFlags |= SDL_RENDERER_PRESENTVSYNC;
    wavaSDLWindow = SDL_CreateWindow("WAVA", s->outer.x, s->outer.y,
            s->outer.w, s->outer.h, windowFlags);
    wavaBailCondition(!wavaSDLWindow, "SDL window cannot be created: %s", SDL_GetError());
    //SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", "cannot create SDL window", NULL);

    wavaSDLGLContext = SDL_GL_CreateContext(wavaSDLWindow);
    GLInit(s);

    return 0;
}

EXP_FUNC void wavaOutputClear(WAVA *s) {
    GLClear(s);
}

EXP_FUNC int wavaOutputApply(WAVA *s) {
    WAVA_CONFIG *p = &s->conf;

    // toggle fullscreen
    SDL_SetWindowFullscreen(wavaSDLWindow,
            SDL_WINDOW_FULLSCREEN & p->flag.fullscreen);

    wavaSDLWindowSurface = SDL_GetWindowSurface(wavaSDLWindow);
    // Appearently SDL uses multithreading so this avoids invalid access
    // If I had a job, here's what I would be fired for xD
    SDL_Delay(100);
    wavaOutputClear(s);

    GLApply(s);

    // Window size patch, because wava wipes w and h for some reason.
    wavaBailCondition(SDL_GetCurrentDisplayMode(
                SDL_GetWindowDisplayIndex(wavaSDLWindow), &wavaSDLVInfo),
            "Error opening display: %s", SDL_GetError());
    calculate_win_pos(s, wavaSDLVInfo.w, wavaSDLVInfo.h,
            wavaSDLWindowSurface->w, wavaSDLWindowSurface->h);
    return 0;
}

EXP_FUNC XG_EVENT wavaOutputHandleInput(WAVA *s) {
    WAVA_CONFIG *p = &s->conf;

    while(SDL_PollEvent(&wavaSDLEvent) != 0) {
        switch(wavaSDLEvent.type) {
            case SDL_KEYDOWN:
                switch(wavaSDLEvent.key.keysym.sym)
                {
                    // should_reload = 1
                    // resizeTerminal = 2
                    // bail = -1
                    case SDLK_a:
                        p->bs++;
                        return WAVA_RESIZE;
                    case SDLK_s:
                        if(p->bs > 0) p->bs--;
                        return WAVA_RESIZE;
                    case SDLK_ESCAPE:
                        return WAVA_QUIT;
                    case SDLK_f: // fullscreen
                        p->flag.fullscreen = !p->flag.fullscreen;
                        return WAVA_RESIZE;
                    case SDLK_UP: // key up
                        p->sens *= 1.05;
                        break;
                    case SDLK_DOWN: // key down
                        p->sens *= 0.95;
                        break;
                    case SDLK_LEFT: // key left
                        p->bw++;
                        return WAVA_RESIZE;
                    case SDLK_RIGHT: // key right
                        if(p->bw > 1) p->bw--;
                        return WAVA_RESIZE;
                    case SDLK_r: // reload config
                        return WAVA_RELOAD;
                    case SDLK_c: // change foreground color
                        if(p->gradients) break;
                        p->col = rand() % 0x100;
                        p->col = p->col << 16;
                        p->col += (unsigned int)rand();
                        return WAVA_REDRAW;
                    case SDLK_b: // change background color
                        p->bgcol = rand() % 0x100;
                        p->bgcol = p->bgcol << 16;
                        p->bgcol += (unsigned int)rand();
                        return WAVA_REDRAW;
                    case SDLK_q:
                        return WAVA_QUIT;
                }
                break;
            case SDL_WINDOWEVENT:
                if(wavaSDLEvent.window.event == SDL_WINDOWEVENT_CLOSE) return -1;
                else if(wavaSDLEvent.window.event == SDL_WINDOWEVENT_RESIZED){
                    // if the user resized the window
                    wavaBailCondition(SDL_GetCurrentDisplayMode(
                                SDL_GetWindowDisplayIndex(wavaSDLWindow),
                                &wavaSDLVInfo),
                            "Error opening display: %s", SDL_GetError());
                    calculate_win_pos(s, wavaSDLVInfo.w, wavaSDLVInfo.h,
                            wavaSDLEvent.window.data1, wavaSDLEvent.window.data2);
                    return WAVA_RESIZE;
                }
                break;
        }
    }

    XG_EVENT_STACK *glEventStack = GLEvent(s);
    while(pendingWAVAEventStack(glEventStack)) {
        XG_EVENT event = popWAVAEventStack(glEventStack);
        if(event != WAVA_IGNORE)
            return event;
    }

    return WAVA_IGNORE;
}

EXP_FUNC void wavaOutputDraw(WAVA *s) {
    GLDraw(s);
    SDL_GL_SwapWindow(wavaSDLWindow);
    return;
}

EXP_FUNC void wavaOutputLoadConfig(WAVA *s) {
    WAVA_CONFIG *p = &s->conf;
    //struct WAVA_CONFIG config = s->default_config.config;

    // VSync doesnt work on SDL2 :(
    p->vsync = 0;

    GLConfigLoad(s);
}

