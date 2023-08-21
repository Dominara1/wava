#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/XKBlib.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrandr.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "output/shared/graphical.h"
#include "shared.h"

#ifdef GL
    #include "output/shared/gl/glew.h"
    #include <X11/extensions/Xrender.h>
    #include <GL/glx.h>
#endif
#ifdef EGL
    #include "output/shared/gl/egl.h"
#endif
#ifdef CAIRO
    #include "output/shared/cairo/main.h"
    #include <cairo.h>
    #include <cairo-xlib.h>
#endif

// random magic values go here
#define SCREEN_CONNECTED 0

static XEvent wavaXEvent;
static Colormap wavaXColormap;
static Display *wavaXDisplay;
static Screen *wavaXScreen;
static Window wavaXWindow, wavaXRoot;
static GC wavaXGraphics;

static XVisualInfo wavaVInfo;
static XSetWindowAttributes wavaAttr;
static Atom wm_delete_window, wmState, fullScreen, mwmHintsProperty, wmStateBelow, taskbar;
static XClassHint wavaXClassHint;
static XWMHints wavaXWMHints;
static XEvent xev;
static Bool wavaSupportsRR;
static int wavaRREventBase;

static XWindowAttributes  windowAttribs;
static XRRScreenResources *wavaXScreenResources;
static XRROutputInfo      *wavaXOutputInfo;
static XRRCrtcInfo        *wavaXCrtcInfo;

static char *monitorName;
static bool overrideRedirectEnabled;
static bool reloadOnDisplayConfigure;

#ifdef GL
    static int VisData[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, True,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 16,
        None
    };

    GLXContext wavaGLXContext;
    GLXFBConfig* wavaFBConfig;

    static XRenderPictFormat *pict_format;

    static GLXFBConfig *fbconfigs, fbconfig;
    static int numfbconfigs;

    void glXSwapIntervalEXT (Display *dpy, GLXDrawable drawable, int interval);
#endif

#ifdef EGL
    static struct _escontext ESContext;
#endif

#ifdef CAIRO
    Drawable wavaXCairoDrawable;
    cairo_surface_t *wavaXCairoSurface;
    //cairo_t *wavaCairoHandle;
    wava_cairo_handle *wavaCairoHandle;
#endif

// mwmHints helps us comunicate with the window manager
struct mwmHints {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
};

static int wavaXScreenNumber;

// Some window manager definitions
enum {
    MWM_HINTS_FUNCTIONS = (1L << 0),
    MWM_HINTS_DECORATIONS =  (1L << 1),

    MWM_FUNC_ALL = (1L << 0),
    MWM_FUNC_RESIZE = (1L << 1),
    MWM_FUNC_MOVE = (1L << 2),
    MWM_FUNC_MINIMIZE = (1L << 3),
    MWM_FUNC_MAXIMIZE = (1L << 4),
    MWM_FUNC_CLOSE = (1L << 5)
};
#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

#ifdef GL
int XGLInit(WAVA_CONFIG *conf) {
    UNUSED(conf);

    // we will use the existing VisualInfo for this, because I'm not messing around with FBConfigs
    wavaGLXContext = glXCreateContext(wavaXDisplay, &wavaVInfo, NULL, 1);
    glXMakeCurrent(wavaXDisplay, wavaXWindow, wavaGLXContext);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    return 0;
}
#endif

// Pull from the terminal colors, and afterwards, do the usual
void snatchColor(char *name, char *colorStr, unsigned int *colorNum, char *databaseName,
        XrmDatabase *wavaXResDB) {
    XrmValue value;
    char *type;
    char strings_are_a_pain_in_c[32];

    sprintf(strings_are_a_pain_in_c, "WAVA.%s", name);

    if(strcmp(colorStr, "default"))
        return; // default not selected

    if(!databaseName)
        return; // invalid database specified

    if(XrmGetResource(*wavaXResDB, strings_are_a_pain_in_c,
                strings_are_a_pain_in_c, &type, &value))
        goto get_color; // XrmGetResource succeeded

    sprintf(strings_are_a_pain_in_c, "*.%s", name);

    if(!XrmGetResource(*wavaXResDB, name, NULL, &type, &value))
        return; // XrmGetResource failed

    get_color:
    sscanf(value.addr, "#%06X", colorNum);
}

void calculateColors(WAVA_CONFIG *conf) {
    XrmInitialize();
    XrmDatabase wavaXResDB = NULL;
    char *databaseName = XResourceManagerString(wavaXDisplay);
    if(databaseName) {
        wavaXResDB = XrmGetStringDatabase(databaseName);
    }
    snatchColor("color5", conf->color, &conf->col, databaseName, &wavaXResDB);
    snatchColor("color4", conf->bcolor, &conf->bgcol, databaseName, &wavaXResDB);
}

EXP_FUNC int wavaInitOutput(WAVA *wava) {
    WAVA_CONFIG *conf = &wava->conf;

    // NVIDIA CPU cap utilization in Vsync fix
    setenv("__GL_YIELD", "USLEEP", 0);

    // workarounds go above if they're required to run before anything else

    // connect to the X server
    wavaXDisplay = XOpenDisplay(NULL);
    wavaBailCondition(!wavaXDisplay, "Could not find X11 display");

    wavaXScreen = DefaultScreenOfDisplay(wavaXDisplay);
    wavaXScreenNumber = DefaultScreen(wavaXDisplay);
    wavaXRoot = RootWindow(wavaXDisplay, wavaXScreenNumber);

    // select appropriate screen
    wavaXScreenResources = XRRGetScreenResources(wavaXDisplay,
        DefaultRootWindow(wavaXDisplay));
    char *screenname = NULL; // potential bugfix if X server has no displays

    wavaSpam("Number of detected screens: %d", wavaXScreenResources->noutput);

    int screenwidth=0, screenheight=0, screenx=0, screeny=0;
    for(int i = 0; i < wavaXScreenResources->noutput; i++) {
        wavaXOutputInfo = XRRGetOutputInfo(wavaXDisplay, wavaXScreenResources,
            wavaXScreenResources->outputs[i]);

        if(wavaXOutputInfo->connection != SCREEN_CONNECTED) // display is not connected
            continue;

        if(wavaXOutputInfo->crtc == 0) // display doesnt have a mode enabled
            continue;

        wavaXCrtcInfo = XRRGetCrtcInfo(wavaXDisplay, wavaXScreenResources,
            wavaXOutputInfo->crtc);

        screenwidth  = wavaXCrtcInfo->width;
        screenheight = wavaXCrtcInfo->height;
        screenx      = wavaXCrtcInfo->x;
        screeny      = wavaXCrtcInfo->y;
        screenname   = strdup(wavaXOutputInfo->name);

        XRRFreeCrtcInfo(wavaXCrtcInfo);
        XRRFreeOutputInfo(wavaXOutputInfo);

        if(!strcmp(screenname, monitorName)) {
            calculate_win_pos(wava, screenwidth, screenheight,
                    conf->w, conf->h);

            // correct window offsets
            wava->outer.x += screenx;
            wava->outer.y += screeny;
            break;
        } else {
            free(screenname);
            // magic value for no screen found
            screenname = NULL;
        }
    }
    XRRFreeScreenResources(wavaXScreenResources);

    // in case that no screen matches, just use default behavior
    if(screenname == NULL) {
        calculate_win_pos(wava,
                wavaXScreen->width, wavaXScreen->height,
                conf->w, conf->h);
        //wavaLog("%d %d %d %d",
        //        wavaXScreen->width,
        //        wavaXScreen->height,
        //        conf->w,
        //        conf->h);
    }

    // 32 bit color means alpha channel support
    #ifdef GL
    if(conf->flag.transparency) {
        fbconfigs = glXChooseFBConfig(wavaXDisplay, wavaXScreenNumber, VisData, &numfbconfigs);
        fbconfig = 0;
        for(int i = 0; i<numfbconfigs; i++) {
            XVisualInfo *visInfo = glXGetVisualFromFBConfig(wavaXDisplay, fbconfigs[i]);
            if(!visInfo) continue;
            else wavaVInfo = *visInfo;

            pict_format = XRenderFindVisualFormat(wavaXDisplay, wavaVInfo.visual);
            if(!pict_format) continue;

            fbconfig = fbconfigs[i];

            if(pict_format->direct.alphaMask > 0) break;
        }
    } else
    #endif
        XMatchVisualInfo(wavaXDisplay, wavaXScreenNumber,
                conf->flag.transparency ? 32 : 24, TrueColor, &wavaVInfo);

    wavaAttr.colormap = XCreateColormap(wavaXDisplay,
            DefaultRootWindow(wavaXDisplay), wavaVInfo.visual, AllocNone);
    wavaXColormap = wavaAttr.colormap;
    calculateColors(conf);
    wavaAttr.background_pixel = 0;
    wavaAttr.border_pixel = 0;

    wavaAttr.backing_store = Always;
    // make it so that the window CANNOT be reordered by the WM
    wavaAttr.override_redirect = overrideRedirectEnabled;

    int wavaWindowFlags = CWOverrideRedirect | CWBackingStore |  CWEventMask |
        CWColormap | CWBorderPixel | CWBackPixel;

    wavaXWindow = XCreateWindow(wavaXDisplay,
        wavaXRoot,
        wava->outer.x, wava->outer.y,
        wava->outer.w, wava->outer.h,
        0,
        wavaVInfo.depth,
        InputOutput,
        wavaVInfo.visual,
        wavaWindowFlags,
        &wavaAttr);

    XStoreName(wavaXDisplay, wavaXWindow, "WAVA");

    // The "X" button is handled by the window manager and not Xorg, so we set up a Atom
    wm_delete_window = XInternAtom (wavaXDisplay, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(wavaXDisplay, wavaXWindow, &wm_delete_window, 1);

    wavaXClassHint.res_name = (char *)"WAVA";
    //wavaXWMHints.flags = InputHint | StateHint;
    //wavaXWMHints.initial_state = NormalState;
    wavaXClassHint.res_class = (char *)"WAVA";
    XmbSetWMProperties(wavaXDisplay, wavaXWindow, NULL, NULL, NULL, 0, NULL,
            &wavaXWMHints, &wavaXClassHint);

    XSelectInput(wavaXDisplay, wavaXWindow, RRScreenChangeNotifyMask |
            VisibilityChangeMask | StructureNotifyMask | ExposureMask |
            KeyPressMask | KeymapNotify);

    #ifdef GL
        wavaBailCondition(XGLInit(conf), "Failed to load GLX extensions");
        GLInit(wava);
    #endif

    #ifdef EGL
        ESContext.native_window = wavaXWindow;
        ESContext.native_display = wavaXDisplay;
        EGLCreateContext(wava, &ESContext);
        EGLInit(wava);
    #endif

    XMapWindow(wavaXDisplay, wavaXWindow);
    wavaXGraphics = XCreateGC(wavaXDisplay, wavaXWindow, 0, 0);

    // Set up atoms
    wmState = XInternAtom(wavaXDisplay, "_NET_WM_STATE", 0);
    taskbar = XInternAtom(wavaXDisplay, "_NET_WM_STATE_SKIP_TASKBAR", 0);
    fullScreen = XInternAtom(wavaXDisplay, "_NET_WM_STATE_FULLSCREEN", 0);
    mwmHintsProperty = XInternAtom(wavaXDisplay, "_MOTIF_WM_HINTS", 0);
    wmStateBelow = XInternAtom(wavaXDisplay, "_NET_WM_STATE_BELOW", 1);

    /**
     * Set up window properties through Atoms,
     *
     * XSendEvent requests the following format:
     *  window  = the respective client window
     *  message_type = _NET_WM_STATE
     *  format = 32
     *  data.l[0] = the action, as listed below
     *  data.l[1] = first property to alter
     *  data.l[2] = second property to alter
     *  data.l[3] = source indication (0-unk,1-normal app,2-pager)
     *  other data.l[] elements = 0
    **/
    xev.xclient.type = ClientMessage;
    xev.xclient.window = wavaXWindow;
    xev.xclient.message_type = wmState;
    xev.xclient.format = 32;
    xev.xclient.data.l[2] = 0;
    xev.xclient.data.l[3] = 0;
    xev.xclient.data.l[4] = 0;

    // keep window in bottom property
    xev.xclient.data.l[0] = conf->flag.beneath ?
        _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
    xev.xclient.data.l[1] = wmStateBelow;
    XSendEvent(wavaXDisplay, wavaXRoot, 0,
            SubstructureRedirectMask | SubstructureNotifyMask, &xev);
    if(conf->flag.beneath) XLowerWindow(wavaXDisplay, wavaXWindow);

    // remove window from taskbar
    xev.xclient.data.l[0] = conf->flag.taskbar ?
        _NET_WM_STATE_REMOVE : _NET_WM_STATE_ADD;
    xev.xclient.data.l[1] = taskbar;
    XSendEvent(wavaXDisplay, wavaXRoot, 0,
            SubstructureRedirectMask | SubstructureNotifyMask, &xev);

    // Setting window options
    struct mwmHints hints;
    hints.flags = (1L << 1);
    hints.decorations = conf->flag.border;
    XChangeProperty(wavaXDisplay, wavaXWindow, mwmHintsProperty,
            mwmHintsProperty, 32, PropModeReplace, (unsigned char *)&hints, 5);

    // move the window in case it didn't by default
    XWindowAttributes xwa;
    XGetWindowAttributes(wavaXDisplay, wavaXWindow, &xwa);
    if(strcmp(conf->winA, "none"))
        XMoveWindow(wavaXDisplay, wavaXWindow, wava->outer.x, wava->outer.y);

    // query for the RR extension in X11
    int error;
    wavaSupportsRR = XRRQueryExtension(wavaXDisplay, &wavaRREventBase, &error);
    if(wavaSupportsRR) {
        int rr_major, rr_minor;

        if(XRRQueryVersion(wavaXDisplay, &rr_major, &rr_minor)) {
            int rr_mask = RRScreenChangeNotifyMask;
            if(rr_major == 1 && rr_minor <= 1) {
                rr_mask &= ~(RRCrtcChangeNotifyMask |
                    RROutputChangeNotifyMask |
                    RROutputPropertyNotifyMask);
            }

            // listen for display configure events only if enabled
            if(reloadOnDisplayConfigure)
                XRRSelectInput(wavaXDisplay, wavaXWindow, rr_mask);
        }
    }

    #if defined(CAIRO)
        wavaXCairoSurface = cairo_xlib_surface_create(wavaXDisplay,
            wavaXWindow, xwa.visual,
            wava->outer.w, wava->outer.h);
        cairo_xlib_surface_set_size(wavaXCairoSurface,
                wava->outer.w, wava->outer.h);
        __internal_wava_output_cairo_init(wavaCairoHandle,
                cairo_create(wavaXCairoSurface));
    #endif

    return 0;
}

EXP_FUNC void wavaOutputClear(WAVA *wava) {
    #if defined(EGL)
        EGLClear(wava);
    #elif defined(GL)
        GLClear(wava);
    #elif defined(CAIRO)
        __internal_wava_output_cairo_clear(wavaCairoHandle);
        UNUSED(wava);
    #else
        UNUSED(wava);
    #endif
}

EXP_FUNC int wavaOutputApply(WAVA *wava) {
    WAVA_CONFIG *conf = &wava->conf;

    calculateColors(conf);

    //Atom xa = XInternAtom(wavaXDisplay, "_NET_WM_WINDOW_TYPE", 0); May be used in the future
    //Atom prop;

    // change window type (this makes sure that compoziting managers don't mess with it)
    //if(xa != NULL)
    //{
    //    prop = XInternAtom(wavaXDisplay, "_NET_WM_WINDOW_TYPE_DESKTOP", 0);
    //    XChangeProperty(wavaXDisplay, wavaXWindow, xa, XA_ATOM, 32, PropModeReplace, (unsigned char *) &prop, 1);
    //}
    // The code above breaks stuff, please don't use it.


    // tell the window manager to switch to a fullscreen state
    xev.xclient.type=ClientMessage;
    xev.xclient.serial = 0;
    xev.xclient.send_event = 1;
    xev.xclient.window = wavaXWindow;
    xev.xclient.message_type = wmState;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = conf->flag.fullscreen ?
        _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
    xev.xclient.data.l[1] = fullScreen;
    xev.xclient.data.l[2] = 0;
    XSendEvent(wavaXDisplay, wavaXRoot, 0,
            SubstructureRedirectMask | SubstructureNotifyMask, &xev);

    wavaOutputClear(wava);

    if(!conf->flag.interact) {
        XRectangle rect;
        XserverRegion region = XFixesCreateRegion(wavaXDisplay, &rect, 1);
        XFixesSetWindowShapeRegion(wavaXDisplay, wavaXWindow, ShapeInput, 0, 0, region);
        XFixesDestroyRegion(wavaXDisplay, region);
    }

    XGetWindowAttributes(wavaXDisplay, wavaXWindow, &windowAttribs);

    #if defined(EGL)
        EGLApply(wava);
    #elif defined(GL)
        GLApply(wava);
    #elif defined(CAIRO)
        __internal_wava_output_cairo_apply(wavaCairoHandle);
        cairo_xlib_surface_set_size(wavaXCairoSurface,
                wava->outer.w, wava->outer.h);
    #endif

    return 0;
}

EXP_FUNC XG_EVENT wavaOutputHandleInput(WAVA *wava) {
    WAVA_CONFIG *conf = &wava->conf;

    // this way we avoid event stacking which requires a full frame to process a single event
    XG_EVENT action = WAVA_IGNORE;

    while(XPending(wavaXDisplay)) {
        XNextEvent(wavaXDisplay, &wavaXEvent);
        switch(wavaXEvent.type) {
            case KeyPress:
            {
                KeySym key_symbol;
                key_symbol = XkbKeycodeToKeysym(wavaXDisplay, (KeyCode)wavaXEvent.xkey.keycode, 0, wavaXEvent.xkey.state & ShiftMask ? 1 : 0);
                switch(key_symbol) {
                    // should_reload = 1
                    // resizeTerminal = 2
                    // bail = -1
                    case XK_a:
                        conf->bs++;
                        return WAVA_RESIZE;
                    case XK_s:
                        if(conf->bs > 0) conf->bs--;
                        return WAVA_RESIZE;
                    case XK_f: // fullscreen
                        conf->flag.fullscreen = !conf->flag.fullscreen;
                        return WAVA_RESIZE;
                    case XK_Up:
                        conf->sens *= 1.05;
                        break;
                    case XK_Down:
                        conf->sens *= 0.95;
                        break;
                    case XK_Left:
                        conf->bw++;
                        return WAVA_RESIZE;
                    case XK_Right:
                        if (conf->bw > 1) conf->bw--;
                        return WAVA_RESIZE;
                    case XK_r: //reload config
                        return WAVA_RELOAD;
                    case XK_q:
                        return WAVA_QUIT;
                    case XK_Escape:
                        return WAVA_QUIT;
                    case XK_b:
                        // WARNING: Assumes that alpha is the
                        // upper 8-bits and that rand is 16-bit
                        conf->bgcol &= 0xff00000;
                        conf->bgcol |= ((rand()<<16)|rand())&0x00ffffff;
                        return WAVA_REDRAW;
                    case XK_c:
                        if(conf->gradients) break;
                        // WARNING: Assumes that alpha is the
                        // upper 8-bits and that rand is 16-bit
                        conf->col &= 0xff00000;
                        conf->col |= ((rand()<<16)|rand())&0x00ffffff;
                        return WAVA_REDRAW;
            }
            break;
        }
        case ConfigureNotify:
        {
            // the window should not be resized when it IS the monitor
            if(overrideRedirectEnabled)
                break;

            // This is needed to track the window size
            XConfigureEvent trackedWavaXWindow;
            trackedWavaXWindow = wavaXEvent.xconfigure;
            if(wava->outer.w != (uint32_t)trackedWavaXWindow.width ||
               wava->outer.h != (uint32_t)trackedWavaXWindow.height) {
                calculate_win_geo(wava,
                        trackedWavaXWindow.width,
                        trackedWavaXWindow.height);
            }
            action = WAVA_RESIZE;
            break;
        }
        case Expose:
            if(action != WAVA_RESIZE)
                action = WAVA_REDRAW;
            break;
        case VisibilityNotify:
            if(wavaXEvent.xvisibility.state == VisibilityUnobscured)
                action = WAVA_RESIZE;
            break;
        case ClientMessage:
            if((Atom)wavaXEvent.xclient.data.l[0] == wm_delete_window)
                return WAVA_QUIT;
            break;
        default:
            if(wavaXEvent.type == wavaRREventBase + RRScreenChangeNotify) {
                wavaLog("Display change detected - Restarting...\n");
                return WAVA_RELOAD;
            }
        }
    }

    // yes this is violent C macro (ab)-use, live with it
    XG_EVENT_STACK *eventStack = 
    #if defined(CAIRO)
        __internal_wava_output_cairo_event(wavaCairoHandle);
    #elif defined(EGL)
        EGLEvent(wava);
    #elif defined(GL)
        GLEvent(wava);
    #endif

    while(pendingWAVAEventStack(eventStack)) {
        XG_EVENT event = popWAVAEventStack(eventStack);
        if(event != WAVA_IGNORE)
            return event;
    }

    return action;
}

EXP_FUNC void wavaOutputDraw(WAVA *wava) {
    #if defined(EGL)
        EGLDraw(wava);
        eglSwapBuffers(ESContext.display, ESContext.surface);
    #elif defined(GL)
        GLDraw(wava);
        glXSwapBuffers(wavaXDisplay, wavaXWindow);
        glXWaitGL();
    #elif defined(CAIRO)
        __internal_wava_output_cairo_draw(wavaCairoHandle);
        UNUSED(wava);
    #else
        UNUSED(wava);
    #endif

    return;
}

EXP_FUNC void wavaOutputCleanup(WAVA *wava) {
    // Root mode leaves artifacts on screen even though the window is dead
    XClearWindow(wavaXDisplay, wavaXWindow);

    // make sure that all events are dead by this point
    XSync(wavaXDisplay, 1);

    #if defined(EGL)
        EGLCleanup(wava, &ESContext);
    #elif defined(GL)
        glXMakeCurrent(wavaXDisplay, 0, 0);
        glXDestroyContext(wavaXDisplay, wavaGLXContext);
        GLCleanup(wava);
    #elif defined(CAIRO)
        __internal_wava_output_cairo_cleanup(wavaCairoHandle);
        UNUSED(wava);
    #else
        UNUSED(wava);
    #endif

    XFreeGC(wavaXDisplay, wavaXGraphics);
    XDestroyWindow(wavaXDisplay, wavaXWindow);
    XFreeColormap(wavaXDisplay, wavaXColormap);
    XCloseDisplay(wavaXDisplay);
    free(monitorName);
    return;
}

EXP_FUNC void wavaOutputLoadConfig(WAVA *wava) {
    wava_config_source config = wava->default_config.config;
    WAVA_CONFIG *conf = &wava->conf;

    UNUSED(conf);

    reloadOnDisplayConfigure = wavaConfigGetBool
        (config, "x11", "reload_on_display_configure", 0);
    overrideRedirectEnabled = wavaConfigGetBool
        (config, "x11", "override_redirect", 0);
    monitorName = strdup(wavaConfigGetString
        (config, "x11", "monitor_name", "none"));

    // Xquartz doesnt support ARGB windows
    // Therefore transparency is impossible on macOS
    #ifdef __APPLE__
        conf->transF = 0;
    #endif

    #ifdef CAIRO
        conf->vsync = 0;
        wavaCairoHandle = __internal_wava_output_cairo_load_config(wava);
    #endif

    #if defined(EGL)
        EGLConfigLoad(wava);
    #elif defined(GL)
        GLConfigLoad(wava);
    #endif
}

