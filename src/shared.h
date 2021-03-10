#ifndef __XAVA_SHARED_H
#define __XAVA_SHARED_H

#include <stdbool.h>

#define EXP_FUNC __attribute__ ((visibility ("default")))

// Audio sensitivity and volume varies greatly between 
// different audio, audio systems and operating systems
// This value is used to properly calibrate the sensitivity
// for a certain platform or setup in the Makefile
#ifndef XAVA_PREDEFINED_SENS_VALUE
	#define XAVA_PREDEFINED_SENS_VALUE 0.0005
#endif

// Some common comparision macros
#ifndef MIN
	#define MIN(x,y) ((x)>(y) ? (y):(x))
#endif
#ifndef MAX
	#define MAX(x,y) ((x)>(y) ? (x):(y))
#endif
#ifndef DIFF
	#define DIFF(x,y) (MAX((x),(y))-MIN((x),(y)))
#endif

#ifdef INIPARSER
	#include "../lib/iniparser/src/iniparser.h"
#else
	#include <iniparser.h>
#endif

#include "module.h"

// XAVA event stuff
typedef enum XAVA_GRAHPICAL_EVENT {
	XAVA_REDRAW, XAVA_IGNORE, XAVA_RESIZE, XAVA_RELOAD,
	XAVA_QUIT
} XG_EVENT;
typedef struct XAVA_GRAHPICAL_EVENT_STACK {
	int pendingEvents;
	XG_EVENT *events;
} XG_EVENT_STACK;
extern void            pushXAVAEventStack    (XG_EVENT_STACK *stack, XG_EVENT event);
extern XG_EVENT        popXAVAEventStack     (XG_EVENT_STACK *stack);
extern XG_EVENT_STACK *newXAVAEventStack     ();
extern void            destroyXAVAEventStack (XG_EVENT_STACK *stack);
extern _Bool           pendingXAVAEventStack (XG_EVENT_STACK *stack);
extern _Bool           isEventPendingXAVA    (XG_EVENT_STACK *stack, XG_EVENT event);

int xavaMkdir(char *dir);
int xavaGetConfigDir(char *configPath);
char *xavaGetInstallDir(void);
unsigned long xavaSleep(unsigned long oldTime, int framerate);

// This funcion is in config.h
// Yes, I commited a sin. Don't shoot, please.
dictionary *get_config_pointer(void);

// Shared audio data sturct
struct audio_data {
	double *audio_out_r;
	double *audio_out_l;
	int format;
	unsigned int rate;
	char *source;				// alsa device, fifo path or pulse source
	int im;						// input mode alsa, fifo or pulse
	int channels;
	int terminate;				// shared variable used to terminate audio thread
	char error_message[1024];
	int inputsize, fftsize;
};

struct config_params {
	char *color, *bcolor, **gradient_colors, *shadow_color;
	double monstercat, integral, gravity, ignore, sens, logScale, logBegin, logEnd,
 eqBalance, foreground_opacity, background_opacity; 
	unsigned int lowcf, highcf, shdw, shdw_col, inputsize, fftsize, gradients, 
 bgcol, col;
	double *smooth;
	int smcount, autobars, stereo, vsync, fixedbars, framerate, bw, bs,
 autosens, overshoot, waves, w, h;
	char *winA;
	int wx, wy;
	_Bool oddoneout, fullF, transF, borderF, bottomF, interactF, taskbarF;
	XAVAMODULE *inputModule, *outputModule;
};

// this is cringe but my linker isn't cooperating,
// so this will have to do
struct function_pointers {
	void            (*pushXAVAEventStack)    (XG_EVENT_STACK *stack, XG_EVENT event);
	XG_EVENT        (*popXAVAEventStack)     (XG_EVENT_STACK *stack);
	XG_EVENT_STACK* (*newXAVAEventStack)     ();
	void            (*destroyXAVAEventStack) (XG_EVENT_STACK *stack);
	_Bool           (*pendingXAVAEventStack) (XG_EVENT_STACK *stack);
	_Bool           (*isEventPendingXAVA)    (XG_EVENT_STACK *stack, XG_EVENT event);
};

struct state_params {
	_Bool pauseRendering;
	struct audio_data audio;
	struct config_params conf;
	struct function_pointers func;
};


#endif
