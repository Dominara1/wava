#ifndef __GL_H
#define __GL_H

#include "main.h"

#include "shared.h"


void            GLConfigLoad(WAVA *wava);
void            GLInit(WAVA *wava);
void            GLClear(WAVA *wava);
void            GLApply(WAVA *wava);
XG_EVENT_STACK *GLEvent(WAVA *wava);
void            GLDraw(WAVA *wava);
void            GLCleanup(WAVA *wava);

#endif

