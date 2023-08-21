#include "shared/ionotify.h"
#define TRUE 1
#define FALSE 0

#define _XOPEN_SOURCE_EXTENDED
#include <locale.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#else
#include <stdlib.h>
#endif

#include <stdio.h>
#include <stdbool.h>
#if defined(__unix__)||defined(__APPLE__)
    #include <termios.h>
#endif
#include <math.h>
#include <fcntl.h>

#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <dirent.h>
#include <unistd.h>

#ifndef PACKAGE
    #define PACKAGE "INSERT_PROGRAM_NAME"
    #warning "Package name not defined!"
#endif

#ifndef VERSION
    #define VERSION "INSERT_PROGRAM_VERSION"
    #warning "Package version not defined!"
#endif

#include "config.h"
#include "shared.h"

char *configPath;

static _Bool kys = 0, should_reload = 0;

// teh main WAVA handle (made just to not piss off MinGW)
static WAVA wava;

// WAVA magic variables, too many of them indeed
static pthread_t p_thread;

void handle_ionotify_call(WAVA_IONOTIFY_EVENT event, const char *filename,
        int id, WAVA *wava) {
    UNUSED(id);
    UNUSED(wava);
    UNUSED(filename);
    switch(event) {
        case WAVA_IONOTIFY_CHANGED:
        //case WAVA_IONOTIFY_DELETED: // ignore because it is just broken
            should_reload = 1;
            break;
        case WAVA_IONOTIFY_ERROR:
            wavaBail("ionotify event errored out! Bailing...");
            break;
        case WAVA_IONOTIFY_CLOSED:
            wavaLog("ionotify socket closed");
            break;
        default:
            wavaLog("Unrecognized ionotify call occured!");
            break;
    }
    return;
}

// general: cleanup
void cleanup(void) {
    WAVA_CONFIG *p      = &wava.conf;
    WAVA_AUDIO  *audio  = &wava.audio;
    WAVA_FILTER *filter = &wava.filter;
    WAVA_OUTPUT *output = &wava.output;

    wavaIONotifyKill(wava.ionotify);

    // telling audio thread to terminate
    audio->terminate = 1;

    // waiting for all background threads and other stuff to terminate properly
    wavaSleep(100, 0);

    // kill the audio thread
    pthread_join(p_thread, NULL);

    output->func.cleanup(&wava);
    if(!p->flag.skipFilter)
        filter->func.cleanup(&wava);

    // destroy modules
    wava_module_free(audio->module);
    wava_module_free(output->module);
    wava_module_free(filter->module);

    // color information
    arr_free(p->gradients);

    // config file path
    //free(configPath); don't free as the string is ONLY allocated during bootup

    // cleanup remaining FFT buffers (abusing C here)
    if(audio->channels == 2)
        free(audio->audio_out_r);

    free(audio->audio_out_l);

    // clean the config
    wavaConfigClose(wava.default_config.config);
}

#if defined(__unix__)||defined(__APPLE__)
// general: handle signals
void sig_handler(int sig_no) {
    switch(sig_no) {
        case SIGUSR1:
            should_reload = true;
            break;
        case SIGINT:
            wavaLog("CTRL-C pressed -- goodbye\n");
            kys=1;
            return;
        case SIGTERM:
            wavaLog("Process termination requested -- goodbye\n");
            kys=1;
            return;
    }
}
#endif

// general: entry point
int main(int argc, char **argv)
{
    // general: define variables
    //int thr_id;

    int sleep = 0;
    int c, silence;
    char *usage = "\n\
Usage : " PACKAGE " [options]\n\
Visualize audio input in a window. \n\
\n\
Options:\n\
    -p          specify path to the config file\n\
    -v          print version\n\
\n\
Keys:\n\
        Up        Increase sensitivity\n\
        Down      Decrease sensitivity\n\
        Left      Decrease number of bars\n\
        Right     Increase number of bars\n\
        r         Reload config\n\
        c         Cycle foreground color\n\
        b         Cycle background color\n\
        q         Quit\n\
\n\
as of 0.4.0 all options are specified in config file, see in '/home/username/.config/wava/' \n";

    uint64_t oldTime = 0;

    setlocale(LC_ALL, "C");
    setbuf(stdout,NULL);
    setbuf(stderr,NULL);

    // general: handle Ctrl+C
    #if defined(__unix__)||defined(__APPLE__)
        struct sigaction action;
        memset(&action, 0, sizeof(action));
        action.sa_handler = &sig_handler;
        sigaction(SIGINT, &action, NULL);
        sigaction(SIGTERM, &action, NULL);
        sigaction(SIGUSR1, &action, NULL);
    #endif

    configPath = NULL;

    // general: handle command-line arguments
    while ((c = getopt (argc, argv, "p:vh")) != -1) {
        switch (c) {
            case 'p': // argument: fifo path
                configPath = optarg;
                break;
            case 'h': // argument: print usage
                printf ("%s", usage);
                return 1;
            case '?': // argument: print usage
                printf ("%s", usage);
                return 1;
            case 'v': // argument: print version
                printf (PACKAGE " " VERSION "\n");
                return 0;
            default:  // argument: no arguments; exit
                abort ();
        }
    }

    // general: main loop
    while (1) {
        // extract the shorthand sub-handles
        WAVA_CONFIG     *p      = &wava.conf;
        WAVA_AUDIO      *audio  = &wava.audio;
        WAVA_FILTER     *filter = &wava.filter;
        WAVA_OUTPUT     *output = &wava.output;

        // initialize ioNotify engine
        wava.ionotify = wavaIONotifySetup();

        // load config
        configPath = load_config(configPath, &wava);

        // attach that config to the IONotify thing
        struct wava_ionotify_watch_setup thing;
        thing.wava_ionotify_func = &handle_ionotify_call;
        thing.filename = configPath;
        thing.ionotify = wava.ionotify;
        thing.id = 1;
        thing.wava = &wava;
        wavaIONotifyAddWatch(&thing);

        // load symbols
        audio->func.loop        = wava_module_symbol_address_get(audio->module, "wavaInput");
        audio->func.load_config = wava_module_symbol_address_get(audio->module, "wavaInputLoadConfig");

        output->func.init         = wava_module_symbol_address_get(output->module, "wavaInitOutput");
        output->func.clear        = wava_module_symbol_address_get(output->module, "wavaOutputClear");
        output->func.apply        = wava_module_symbol_address_get(output->module, "wavaOutputApply");
        output->func.handle_input = wava_module_symbol_address_get(output->module, "wavaOutputHandleInput");
        output->func.draw         = wava_module_symbol_address_get(output->module, "wavaOutputDraw");
        output->func.cleanup      = wava_module_symbol_address_get(output->module, "wavaOutputCleanup");
        output->func.load_config  = wava_module_symbol_address_get(output->module, "wavaOutputLoadConfig");

        if(!p->flag.skipFilter) {
            filter->func.init        = wava_module_symbol_address_get(filter->module, "wavaFilterInit");
            filter->func.apply       = wava_module_symbol_address_get(filter->module, "wavaFilterApply");
            filter->func.loop        = wava_module_symbol_address_get(filter->module, "wavaFilterLoop");
            filter->func.cleanup     = wava_module_symbol_address_get(filter->module, "wavaFilterCleanup");
            filter->func.load_config = wava_module_symbol_address_get(filter->module, "wavaFilterLoadConfig");
        }

        // we're loading this first because I want output modes to adjust audio
        // "renderer/filter" properties

        // load output config
        output->func.load_config(&wava);

        // set up audio properties BEFORE the input is initialized
        audio->inputsize = p->inputsize;
        audio->fftsize   = p->fftsize;
        audio->format    = -1;
        audio->terminate = 0;
        audio->channels  = 1 + p->stereo;
        audio->rate      = p->samplerate;
        audio->latency   = p->samplelatency;

        // load input config
        audio->func.load_config(&wava);

        // load filter config
        if(!p->flag.skipFilter)
            filter->func.load_config(&wava);

        // setup audio garbo
        MALLOC_SELF(audio->audio_out_l, p->fftsize+1);
        if(p->stereo)
            MALLOC_SELF(audio->audio_out_r, p->fftsize+1);
        if(p->stereo) {
            for (uint32_t i = 0; i < audio->fftsize; i++) {
                audio->audio_out_l[i] = 0;
                audio->audio_out_r[i] = 0;
            }
        } else for(uint32_t i=0; i < audio->fftsize; i++) audio->audio_out_l[i] = 0;

        // thr_id = below
        pthread_create(&p_thread, NULL, audio->func.loop, (void*)audio);

        bool reloadConf = false;

        if(!p->flag.skipFilter)
            wavaBailCondition(filter->func.init(&wava),
                    "Failed to initialize filter! Bailing...");

        wavaBailCondition(output->func.init(&wava),
                "Failed to initialize output! Bailing...");

        wavaIONotifyStart(wava.ionotify);

        while(!reloadConf) { //jumbing back to this loop means that you resized the screen
            if(p->flag.ignoreWindowSize == false) {
                // handle for user setting too many bars
                if (p->fixedbars) {
                    p->autobars = 0;
                    if (p->fixedbars * p->bw + p->fixedbars * p->bs - p->bs >
                            wava.bar_space.w)
                        p->autobars = 1;
                }

                //getting orignial numbers of barss incase of resize
                if (p->autobars == 1)  {
                    wava.bars = (wava.bar_space.w + p->bs) / (p->bw + p->bs);

                    //if (p->bs != 0) bars = (w - bars * p->bs + p->bs) / bw;
                } else wava.bars = p->fixedbars;

                if (wava.bars < 1) wava.bars = 1; // must have at least 1 bars

                if (p->stereo) { // stereo must have even numbers of bars
                    if (wava.bars%2) wava.bars--;
                }
            } else {
                wava.bars = p->fixedbars;
            }

            if(!p->flag.skipFilter)
                filter->func.apply(&wava);

            output->func.apply(&wava);

            bool resizeWindow = false;
            bool redrawWindow = false;

            while  (!resizeWindow) {
                switch(output->func.handle_input(&wava)) {
                    case WAVA_QUIT:
                        cleanup();
                        return EXIT_SUCCESS;
                    case WAVA_RELOAD:
                        should_reload = 1;
                        break;
                    case WAVA_RESIZE:
                        resizeWindow = TRUE;
                        break;
                    case WAVA_REDRAW:
                        redrawWindow = TRUE;
                        break;
                    default:
                        // ignore other values
                        break;
                }

                if (should_reload) {
                    reloadConf = true;
                    resizeWindow = true;
                    should_reload = 0;
                }

                //if (cont == 0) break;

                // process: populate input buffer and check if input is present
                silence = 1;

                for (uint32_t i = 0; i < audio->fftsize; i++) {
                    if(audio->audio_out_l[i]) {
                        silence = 0;
                        break;
                    }
                }

                if(p->stereo) {
                    for (uint32_t i = 0; i < audio->fftsize; i++) {
                        if(audio->audio_out_r[i]) {
                            silence = 0;
                            break;
                        }
                    }
                }

                if (silence == 1) {
                    sleep++;
                } else {
                    sleep = 0;
                    wavaSpamCondition(wava.pauseRendering, "Resuming from sleep");
                }

                // process: if input was present for the last 5 seconds apply FFT to it
                if (sleep < p->framerate * 5) {
                    wava.pauseRendering = false;
                } else if(wava.pauseRendering) {
                    // unless the user requested that the program ends
                    if(kys||should_reload) sleep = 0;

                    // wait 100ms, then check sound again.
                    wavaSleep(100, 0);
                    continue;
                } else { // if in sleep mode wait and continue
                    wavaSpam("Going to sleep!");

                    // signal to any potential rendering threads to stop
                    wava.pauseRendering = true;
                }

                // Calculate the result through filters
                if(!p->flag.skipFilter) {
                    filter->func.loop(&wava);

                    // zero values causes divided by zero segfault
                    // and set max height
                    for (uint32_t i = 0; i < wava.bars; i++) {
                        if(wava.f[i] < 1)
                            wava.f[i] = 1;
                        else if(wava.f[i] > wava.bar_space.h)
                            wava.f[i] = wava.bar_space.h;
                    }
                }

                // output: draw processed input
                if(redrawWindow) {
                    output->func.clear(&wava);

                    // audio output is unallocated without the filter
                    if(!p->flag.skipFilter)
                        memset(wava.fl, 0x00, sizeof(int)*wava.bars);

                    redrawWindow = FALSE;
                }
                output->func.draw(&wava);

                if(!p->vsync) // the window handles frametimes instead of WAVA
                    oldTime = wavaSleep(oldTime, p->framerate);

                // save previous bar values
                if(!p->flag.skipFilter)
                    memcpy(wava.fl, wava.f, sizeof(int)*wava.bars);

                if(kys) {
                    resizeWindow=1;
                    reloadConf=1;
                }

                // checking if audio thread has exited unexpectedly
                wavaBailCondition(audio->terminate,
                        "Audio thread has exited unexpectedly.\nReason: %s",
                        audio->error_message);
            } // resize window
        } // reloading config

        // get rid of everything else
        cleanup();

        // since this is an infinite loop we need to break out of it
        if(kys) break;
    }
    return 0;
}
