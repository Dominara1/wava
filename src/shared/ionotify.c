#include <x-watcher.h>

#include "shared/ionotify.h"
#include "shared.h"

// cringe but bare with me k
struct hax {
    void *original_data;
    void (*wava_ionotify_func)(WAVA_IONOTIFY_EVENT,
            const char *filename, int id, WAVA*);
};

static struct hax hax[100];
static size_t hax_count;

EXP_FUNC WAVAIONOTIFY wavaIONotifySetup(void) {
    hax_count = 0;
    return (WAVAIONOTIFY)xWatcher_create();
}

// before calling the acutal library, this function takes care of
// converting the xWatcher's event types to native ones
void __internal_wavaIONotifyWorkAroundDumbDecisions(XWATCHER_FILE_EVENT event,
        const char *name, int id, void* data) {
    struct hax *h = (struct hax*)data;
    WAVA_IONOTIFY_EVENT new_event = 0;

    switch(event) {
        case XWATCHER_FILE_CREATED:
        case XWATCHER_FILE_MODIFIED:
            new_event = WAVA_IONOTIFY_CHANGED;
            break;
        case XWATCHER_FILE_REMOVED:
            new_event = WAVA_IONOTIFY_DELETED;
            break;
        case XWATCHER_FILE_OPENED:             // this gets triggered by basically everything
        case XWATCHER_FILE_ATTRIBUTES_CHANGED: // usually not important
        case XWATCHER_FILE_NONE:
        case XWATCHER_FILE_RENAMED:            // ignoring because most likely breaks visualizer
        case XWATCHER_FILE_UNSPECIFIED:
            new_event = WAVA_IONOTIFY_NOTHING;
            break;
        default:
            new_event = WAVA_IONOTIFY_ERROR;
    }

    h->wava_ionotify_func(new_event, name, id, h->original_data);
}

EXP_FUNC bool wavaIONotifyAddWatch(WAVAIONOTIFYWATCHSETUP setup) {
    xWatcher_reference reference;

    // workaround in progress
    hax[hax_count].original_data      = setup->wava;
    hax[hax_count].wava_ionotify_func = setup->wava_ionotify_func;

    reference.callback_func   = __internal_wavaIONotifyWorkAroundDumbDecisions;
    reference.context         = setup->id;
    reference.path            = setup->filename;
    reference.additional_data = &hax[hax_count];

    // hax counter
    hax_count++;

    wavaBailCondition(hax_count > 100, "Scream at @nikp123 to fix this!");

    return xWatcher_appendFile(setup->ionotify, &reference);
}

EXP_FUNC bool wavaIONotifyStart(const WAVAIONOTIFY ionotify) {
    return xWatcher_start(ionotify);
}

EXP_FUNC void wavaIONotifyKill(const WAVAIONOTIFY ionotify) {
    xWatcher_destroy(ionotify);
}

