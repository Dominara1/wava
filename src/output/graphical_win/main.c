#include <assert.h>
#include <stdio.h>
#include <time.h>

#include <tchar.h>
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>

// it must be in this OCD breaking order or it will fail to compile ;(
#ifdef GL
    #include "output/shared/gl/glew.h"
    #include <GL/wglext.h>
#endif

#ifdef CAIRO
    #include <cairo.h>
    #include <cairo-win32.h>
    #include "output/shared/cairo/main.h"
#endif

#include "output/shared/graphical.h"
#include "shared.h"

#include "main.h"

const char szAppName[] = "WAVA";
const char wcWndName[] = "WAVA";

HWND wavaWinWindow;
MSG wavaWinEvent;
HMODULE wavaWinModule;
WNDCLASSEX wavaWinClass;
HDC wavaWinFrame;
TIMECAPS wavaPeriod;

// These hold the size and position of the window if you're switching to fullscreen mode
// because Windows (or rather WIN32) doesn't do it internally
i32 oldX, oldY, oldW, oldH;

#ifdef GL
    BOOL WINAPI wglSwapIntervalEXT (int interval);
    HGLRC wavaWinGLFrame;
#endif

#ifdef CAIRO
    wava_cairo_handle *wavaCairoHandle;
    cairo_surface_t   *wavaCairoSurface;
#endif

// a crappy workaround for a flawed event-loop design
static _Bool resized=FALSE, quit=FALSE;

// Warning: shitty win32 api design below, cover your poor eyes
// Why can't I just pass this shit immediately to my events function
// instead of doing this shit

// Retarded shit, exhibit A:
void *wavaHandleForWindowFuncBecauseWinAPIIsOutdated;
LRESULT CALLBACK WindowFunc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    // this shit is beyond retarded
    WAVA *wava = wavaHandleForWindowFuncBecauseWinAPIIsOutdated;

    // god why
    if(wava == NULL)
        return DefWindowProc(hWnd,msg,wParam,lParam);

    WAVA_CONFIG *conf = &wava->conf;

    switch(msg) {
        case WM_CREATE:
            return WAVA_RESIZE;
        case WM_KEYDOWN:
            switch(wParam) {
                // should_reload = 1
                // resizeTerminal = 2
                // bail = -1
                case 'A':
                    conf->bs++;
                    return WAVA_RESIZE;
                case 'S':
                    if(conf->bs > 0) conf->bs--;
                    return WAVA_RESIZE;
                case 'F': // fullscreen
                    conf->flag.fullscreen = !conf->flag.fullscreen;
                    return WAVA_RESIZE;
                case VK_UP:
                    conf->sens *= 1.05;
                    break;
                case VK_DOWN:
                    conf->sens *= 0.95;
                    break;
                case VK_LEFT:
                    conf->bw++;
                    return WAVA_RESIZE;
                case VK_RIGHT:
                    if (conf->bw > 1) conf->bw--;
                    return WAVA_RESIZE;
                case 'R': //reload config
                    return WAVA_RELOAD;
                case 'Q':
                    return WAVA_QUIT;
                case VK_ESCAPE:
                    return WAVA_QUIT;
                case 'B':
                    conf->bgcol = (rand()<<16)|rand();
                    return WAVA_REDRAW;
                case 'C':
                    if(conf->gradients) break;
                    conf->col = (rand()<<16)|rand();
                    return WAVA_REDRAW;
                default: break;
            }
            break;
        case WM_SIZE:
            calculate_win_geo(wava, LOWORD(lParam), HIWORD(lParam));
            resized=TRUE;
            return WAVA_RELOAD;
        case WM_CLOSE:
            // Perform cleanup tasks.
            PostQuitMessage(0);
            quit=TRUE;
            return WAVA_QUIT;
        case WM_DESTROY:
            quit=TRUE;
            return WAVA_QUIT;
        case WM_QUIT: // lol why is this ignored
            break;
        case WM_NCHITTEST: {
            LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
            if (hit == HTCLIENT) hit = HTCAPTION;
            return hit;
        }
        default:
            return DefWindowProc(hWnd,msg,wParam,lParam);
    }
    return WAVA_IGNORE;
}

EXP_FUNC void wavaOutputClear(WAVA *wava) {
    #ifdef GL
        GLClear(wava);
    #endif
    #ifdef CAIRO
        UNUSED(wava);
        __internal_wava_output_cairo_clear(wavaCairoHandle);
    #endif
}

unsigned char register_window_win(HINSTANCE HIn) {
    wavaWinClass.cbSize=sizeof(WNDCLASSEX);
    wavaWinClass.style=CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wavaWinClass.lpfnWndProc=WindowFunc;
    wavaWinClass.cbClsExtra=0;
    wavaWinClass.cbWndExtra=0;
    wavaWinClass.hInstance=HIn;
    wavaWinClass.hIcon=NULL;
    wavaWinClass.hIconSm=wavaWinClass.hIcon;
    wavaWinClass.hCursor=LoadCursor(NULL,IDC_ARROW);
    wavaWinClass.hbrBackground=(HBRUSH)CreateSolidBrush(0x00000000);
    wavaWinClass.lpszMenuName=NULL;
    wavaWinClass.lpszClassName=szAppName;

    return RegisterClassEx(&wavaWinClass);
}

void GetDesktopResolution(int *horizontal, int *vertical) {
    RECT desktop;

    // Get a handle to the desktop window
    const HWND hDesktop = GetDesktopWindow();
    // Get the size of screen to the variable desktop
    GetWindowRect(hDesktop, &desktop);

    // return dimensions
    (*horizontal) = desktop.right;
    (*vertical) = desktop.bottom;

    return;
}

EXP_FUNC int wavaInitOutput(WAVA *wava) {
    WAVA_CONFIG *conf = &wava->conf;

    // reset event trackers
    resized=FALSE;
    quit=FALSE;

    // never assume that memory is clean
    oldX = 0; oldY = 0; oldW = -1; oldH = -1;

    // get handle
    wavaWinModule = GetModuleHandle(NULL);

    // register window class
    wavaBailCondition(!register_window_win(wavaWinModule), "RegisterClassEx failed");

    // get window size etc..
    int screenWidth, screenHeight;
    GetDesktopResolution(&screenWidth, &screenHeight);

    // adjust window position etc...
    calculate_win_pos(wava, screenWidth, screenHeight,
            conf->w, conf->h);

    // why?
    //if(!conf->transF) conf->interactF=1; // correct practicality error

    // extended and standard window styles
    DWORD dwExStyle=0, dwStyle=0;
    if(conf->flag.transparency) dwExStyle|=WS_EX_TRANSPARENT;
    if(!conf->flag.interact) dwExStyle|=WS_EX_LAYERED;
    if(!conf->flag.taskbar) dwExStyle|=WS_EX_TOOLWINDOW;
    if(conf->flag.border) dwStyle|=WS_CAPTION;

    // create window
    wavaWinWindow = CreateWindowEx(dwExStyle, szAppName, wcWndName,
        WS_POPUP | WS_VISIBLE | dwStyle,
        wava->outer.x, wava->outer.y, wava->outer.w, wava->outer.h,
        NULL, NULL, wavaWinModule, NULL);
    wavaBailCondition(!wavaWinWindow, "CreateWindowEx failed");

    // transparency fix
    if(conf->flag.transparency) SetLayeredWindowAttributes(wavaWinWindow,
            0x00FFFFFF, 255, LWA_ALPHA | LWA_COLORKEY);
    SetWindowPos(wavaWinWindow, conf->flag.beneath ? HWND_BOTTOM : HWND_NOTOPMOST,
            wava->outer.x, wava->outer.y,
            wava->outer.w, wava->outer.h,
            SWP_SHOWWINDOW);

    // we need the desktop window manager to enable transparent background (from Vista ...onward)
    DWM_BLURBEHIND bb = {0};
    HRGN hRgn = CreateRectRgn(0, 0, -1, -1);
    bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.hRgnBlur = hRgn;
    bb.fEnable = conf->flag.transparency;
    bb.fTransitionOnMaximized = true;
    DwmEnableBlurBehindWindow(wavaWinWindow, &bb);

    wavaWinFrame = GetDC(wavaWinWindow);

    #ifdef GL
        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,                                // Version Number
            PFD_DRAW_TO_WINDOW      |         // Format Must Support Window
        #ifdef GL
            PFD_SUPPORT_OPENGL      |         // Format Must Support OpenGL
        #endif
            PFD_SUPPORT_COMPOSITION |         // Format Must Support Composition
            PFD_DOUBLEBUFFER,                 // Must Support Double Buffering
            PFD_TYPE_RGBA,                    // Request An RGBA Format
            32,                               // Select Our Color Depth
            0, 0, 0, 0, 0, 0,                 // Color Bits Ignored
            8,                                // An Alpha Buffer
            0,                                // Shift Bit Ignored
            0,                                // No Accumulation Buffer
            0, 0, 0, 0,                       // Accumulation Bits Ignored
            24,                               // 16Bit Z-Buffer (Depth Buffer)
            8,                                // Some Stencil Buffer
            0,                                // No Auxiliary Buffer
            PFD_MAIN_PLANE,                   // Main Drawing Layer
            0,                                // Reserved
            0, 0, 0                           // Layer Masks Ignored
        };

        int PixelFormat = ChoosePixelFormat(wavaWinFrame, &pfd);
        wavaBailCondition(PixelFormat == 0, "ChoosePixelFormat failed!");

        BOOL bResult = SetPixelFormat(wavaWinFrame, PixelFormat, &pfd);
        wavaBailCondition(bResult == FALSE, "SetPixelFormat failed!");

        wavaWinGLFrame = wglCreateContext(wavaWinFrame);
        wavaBailCondition(wavaWinGLFrame == NULL, "wglCreateContext failed!");
    #endif

    // process colors
    if(!strcmp(conf->color, "default")) {
        // we'll just get the accent color (which is way easier and an better thing to do)
        WINBOOL opaque = 1;
        DWORD fancyVariable;
        HRESULT error = DwmGetColorizationColor(&fancyVariable, &opaque);
        conf->col = fancyVariable;
        wavaWarnCondition(!SUCCEEDED(error), "DwmGetColorizationColor failed");
    } // as for the other case, we don't have to do any more processing

    if(!strcmp(conf->bcolor, "default")) {
        // we'll just get the accent color (which is way easier and a better thing to do)
        WINBOOL opaque = 1;
        DWORD fancyVariable;
        HRESULT error = DwmGetColorizationColor(&fancyVariable, &opaque);
        conf->bgcol = fancyVariable;
        wavaWarnCondition(!SUCCEEDED(error), "DwmGetColorizationColor failed");
    }

    // WGL
    #ifdef GL
        wglMakeCurrent(wavaWinFrame, wavaWinGLFrame);
        GLInit(wava);
    #endif
    #ifdef CAIRO
        wavaCairoSurface = cairo_win32_surface_create(wavaWinFrame);
        __internal_wava_output_cairo_init(wavaCairoHandle,
                cairo_create(wavaCairoSurface));
    #endif

    // set up precise timers (otherwise unstable framerate)
    wavaWarnCondition(timeGetDevCaps(&wavaPeriod, sizeof(TIMECAPS))!=MMSYSERR_NOERROR,
            "Unable to obtain precise system timers! Stability may be of concern!");

    timeEndPeriod(0);
    timeBeginPeriod(wavaPeriod.wPeriodMin);

    return 0;
}

EXP_FUNC int wavaOutputApply(WAVA *wava) {
    WAVA_CONFIG *conf = &wava->conf;

    //ReleaseDC(wavaWinWindow, wavaWinFrame);

    if(conf->flag.fullscreen) {
        POINT Point = {0};
        HMONITOR Monitor = MonitorFromPoint(Point, MONITOR_DEFAULTTONEAREST);
        MONITORINFO MonitorInfo = { 0 };
        if (GetMonitorInfo(Monitor, &MonitorInfo)) {
            DWORD Style = WS_POPUP | WS_VISIBLE;
            SetWindowLongPtr(wavaWinWindow, GWL_STYLE, Style);

            // dont overwrite old size on accident if already fullscreen
            if(oldW == -1 && oldH == -1) {
                oldX = wava->outer.x; oldY = wava->outer.y;
                oldW = wava->outer.w; oldH = wava->outer.h;
            }

            // resizing to full screen
            SetWindowPos(wavaWinWindow, 0, MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                MonitorInfo.rcMonitor.right-MonitorInfo.rcMonitor.left,
                MonitorInfo.rcMonitor.bottom-MonitorInfo.rcMonitor.top,
                SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }
        // check if the window has been already resized
    } else if(oldW != -1 && oldH != -1) {
        wava->outer.x = oldX; wava->outer.y = oldY;
        calculate_win_geo(wava, oldW, oldH);

        oldW = -1;
        oldH = -1;

        // restore window properties
        DWORD Style = WS_POPUP | WS_VISIBLE | (conf->flag.border? WS_CAPTION:0);
        SetWindowLongPtr(wavaWinWindow, GWL_STYLE, Style);

        SetWindowPos(wavaWinWindow, 0,
                wava->outer.w, wava->outer.y,
                wava->outer.w, wava->outer.h,
                SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }

    wavaOutputClear(wava);

    // WGL stuff
    #ifdef GL
        GLApply(wava);
        PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)(void*)wglGetProcAddress("wglSwapIntervalEXT");
        wglSwapIntervalEXT(conf->vsync);
    #endif
    #ifdef CAIRO
        __internal_wava_output_cairo_apply(wavaCairoHandle);
    #endif

    return 0;
}

EXP_FUNC XG_EVENT wavaOutputHandleInput(WAVA *wava) {
    // don't even fucking ask
    wavaHandleForWindowFuncBecauseWinAPIIsOutdated = wava;

    while(PeekMessage(&wavaWinEvent, wavaWinWindow, 0, 0, PM_REMOVE)) {
        TranslateMessage(&wavaWinEvent);

        // you fucking piece of shit why are you predefined type AAAAAAAAAAAAAA
        int r=DispatchMessage(&wavaWinEvent);  // handle return values

        // so you may have wondered why do i do stuff like this
        // it's because non-keyboard/mouse messages DONT pass through return values
        // which, guess what, completely breaks my previous design - thanks micro$oft, really appreciate it

        if(quit) {
            quit=FALSE;
            return WAVA_QUIT;
        }
        if(resized) {
            resized=FALSE;
            return WAVA_RESIZE;
        }

        if(r != WAVA_IGNORE)
            return r;
    }

    XG_EVENT_STACK *eventStack = 
    #if defined(CAIRO)
        __internal_wava_output_cairo_event(wavaCairoHandle);
    #elif defined(GL)
        GLEvent(wava);
    #endif

    while(pendingWAVAEventStack(eventStack)) {
        XG_EVENT event = popWAVAEventStack(eventStack);
        if(event != WAVA_IGNORE)
            return event;
    }

    return WAVA_IGNORE;
}

EXP_FUNC void wavaOutputDraw(WAVA *wava) {
    #ifdef GL
        wglMakeCurrent(wavaWinFrame, wavaWinGLFrame);
        GLDraw(wava);
        SwapBuffers(wavaWinFrame);
    #endif

    #ifdef CAIRO
        UNUSED(wava);
        __internal_wava_output_cairo_draw(wavaCairoHandle);
        SwapBuffers(wavaWinFrame);
    #endif
}

EXP_FUNC void wavaOutputCleanup(WAVA *wava) {
    #ifdef GL
        GLCleanup(wava);
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(wavaWinGLFrame);
    #endif

    #ifdef CAIRO
        UNUSED(wava);
        __internal_wava_output_cairo_cleanup(wavaCairoHandle);
    #endif

    // Normal Win32 stuff
    timeEndPeriod(wavaPeriod.wPeriodMin);
    ReleaseDC(wavaWinWindow, wavaWinFrame);
    DestroyWindow(wavaWinWindow);
    UnregisterClass(szAppName, wavaWinModule);
    //CloseHandle(wavaWinModule);
}

EXP_FUNC void wavaOutputLoadConfig(WAVA *wava) {
    WAVA_CONFIG *conf = &wava->conf;

    #ifdef GL
        // VSync is a must due to shit Windows timers
        conf->vsync = 1;
        GLConfigLoad(wava);
    #endif
    #ifdef CAIRO
        conf->vsync = 0;
        wavaCairoHandle = __internal_wava_output_cairo_load_config(wava);
    #endif
}

