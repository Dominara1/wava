#ifndef XAVA_CAIRO_MODULE_MEDIA_INFO_MEDIA_DATA
#define XAVA_CAIRO_MODULE_MEDIA_INFO_MEDIA_DATA

#include "artwork.h"

#define MUSIC_DATA_STRING_LENGHT 1024

struct media_data {
    struct artwork cover;
    char           album [MUSIC_DATA_STRING_LENGHT];
    char           artist[MUSIC_DATA_STRING_LENGHT];
    char           title [MUSIC_DATA_STRING_LENGHT];
};

struct media_data_thread;

struct media_data *
xava_cairo_module_media_data_thread_data(struct media_data_thread *thread);
struct media_data_thread *
xava_cairo_module_media_data_thread_create(void);
void
xava_cairo_module_media_data_thread_destroy(struct media_data_thread *data);

#endif

