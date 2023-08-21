#ifndef __EGL_H
#define __EGL_H

#ifndef EGL
    #define EGL
#endif
#include "main.h"

struct _escontext {
    // because windowing systems are complicated
    EGLNativeDisplayType native_display;
    EGLNativeWindowType native_window;

    // EGL display
    EGLDisplay  display;
    // EGL context
    EGLContext  context;
    // EGL surface
    EGLSurface  surface;
};

void           EGLConfigLoad(WAVA *wava);
EGLBoolean     EGLCreateContext(WAVA *wava, struct _escontext *ESContext);
void           EGLInit(WAVA *wava);
void           EGLApply(WAVA *wava);
XG_EVENT_STACK *EGLEvent(WAVA *wava);
void           EGLClear(WAVA *wava);
void           EGLDraw(WAVA *wava);
void           EGLCleanup(WAVA *wava, struct _escontext *ESContext);
#endif

