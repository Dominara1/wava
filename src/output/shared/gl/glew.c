#include <math.h>

#ifndef GL
    #define GL
#endif
#include "glew.h"

#include "shared.h"
#include "output/shared/graphical.h"


void GLConfigLoad(WAVA *wava) {
    SGLConfigLoad(wava);
}

void GLInit(WAVA *wava) {
    glewInit();

    SGLInit(wava);
}

void GLApply(WAVA *wava) {
    SGLApply(wava);
}

XG_EVENT_STACK *GLEvent(WAVA *wava) {
    return SGLEvent(wava);
}

void GLClear(WAVA *wava) {
    SGLClear(wava);
}

void GLDraw(WAVA *wava) {
    SGLDraw(wava);
}

void GLCleanup(WAVA *wava) {
    SGLCleanup(wava);
}

