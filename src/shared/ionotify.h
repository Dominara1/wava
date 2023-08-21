#ifndef __WAVA_SHARED_IONOTIFY_H
#define __WAVA_SHARED_IONOTIFY_H

#include <stdbool.h>

typedef void*                             WAVAIONOTIFY;
typedef struct wava_ionotify_watch_setup* WAVAIONOTIFYWATCHSETUP;

// make the compilers shuttings up
typedef struct WAVA WAVA;

// doubt that WAVA_IONOTIFY_DELETED ever be implemented, but I don't care
typedef enum wava_ionotify_event {
    WAVA_IONOTIFY_NOTHING,
    WAVA_IONOTIFY_ERROR,
    WAVA_IONOTIFY_CHANGED,
    WAVA_IONOTIFY_DELETED,
    WAVA_IONOTIFY_CLOSED
} WAVA_IONOTIFY_EVENT;

struct wava_ionotify_watch_setup {
    WAVAIONOTIFY       ionotify;
    int                id;
    char               *filename;
    WAVA *wava;
    void               (*wava_ionotify_func)(WAVA_IONOTIFY_EVENT,
                                            const char *filename,
                                            int id,
                                            WAVA*);
};

extern WAVAIONOTIFY      wavaIONotifySetup(void);
extern bool              wavaIONotifyAddWatch(WAVAIONOTIFYWATCHSETUP setup);
extern bool              wavaIONotifyStart(const WAVAIONOTIFY ionotify);
extern void              wavaIONotifyKill(const WAVAIONOTIFY ionotify);
#endif

