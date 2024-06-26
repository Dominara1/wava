#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>

#include "shared.h"

typedef unsigned int u32_t;
typedef short s16_t;


// #define BUFSIZE 1024
#define BUFSIZE 4096

#define VIS_BUF_SIZE 16384
#define VB_OFFSET 8192+4096

typedef struct {
    pthread_rwlock_t rwlock;
    u32_t buf_size;
    u32_t buf_index;
    unsigned char running;
    u32_t rate;
    time_t updated;
    s16_t buffer[VIS_BUF_SIZE];
}  vis_t;


//input: SHMEM
EXP_FUNC void* wavaInput(void* data)
{
    WAVA_AUDIO *audio = (WAVA_AUDIO *)data;
    vis_t *mmap_area;
    int fd; /* file descriptor to mmaped area */
    int mmap_count = sizeof( vis_t);
    /*
    struct timespec req = { .tv_sec = 0, .tv_nsec = 10000000 };
    int16_t buf[BUFSIZE / 2];
    int bytes = 0;
    int t = 0;
    */
    int n = 0;
    int i;

    wavaSpam("SHMEM source: %s", audio->source);

    fd = shm_open(audio->source, O_RDWR, 0666);

    wavaBailCondition(fd<0, "Could not open source '%s': %s", audio->source, strerror(errno));

    mmap_area = mmap(NULL, sizeof( vis_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    wavaBailCondition((intptr_t)mmap_area==-1, "mmap failed - check if squeezelite is running with visualization enabled");

    wavaSpam("bufs: %u / run: %u / rate: %u\n",mmap_area->buf_size, mmap_area->running, mmap_area->rate);
    audio->rate = mmap_area->rate;

    while (1) {
//        for (i = 0; i < BUFSIZE; i += 2) { // BUFSIZE / 2
        for (i = VB_OFFSET; i < BUFSIZE+VB_OFFSET; i += 2) {
            if (audio->channels == 1) {
                audio->audio_out_l[n] = (mmap_area->buffer[i] + mmap_area->buffer[i + 1]) / 2;
            } else if (audio->channels == 2) {
                audio->audio_out_l[n] = mmap_area->buffer[i];
                audio->audio_out_r[n] = mmap_area->buffer[i + 1];
            }
            n++;
            if (n == 2048 - 1) n = 0;
        }
        if (audio->terminate == 1) {
            break;
        }
    }

    // cleanup
    wavaErrorCondition(fd<1, "Wrong file descriptor %d", fd);
    wavaErrorCondition(close(fd)!=0, "Could not close file descriptor %d: %s", fd, strerror(errno));

    wavaErrorCondition(munmap(mmap_area, mmap_count)!=0, "Could not munmap() area %p+%d. %s",
            mmap_area, mmap_count, strerror(errno));
    return 0;
}

EXP_FUNC void wavaInputLoadConfig(WAVA *wava) {
    WAVA_AUDIO *audio = &wava->audio;
    wava_config_source config = wava->default_config.config;
    audio->rate = 44100;
    audio->source = (char*)wavaConfigGetString(config, "input", "source", "/squeezelite-00:00:00:00:00:00");
}

