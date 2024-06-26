#include <assert.h>
#include <errno.h>
#include <sndio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "shared.h"

EXP_FUNC void* wavaInput(void* data)
{
    WAVA_AUDIO *audio = (WAVA_AUDIO *)data;
    struct sio_par par;
    struct sio_hdl *hdl;
    int16_t buf[256];
    unsigned int i, n, channels;

    assert(audio->channels > 0);
    channels = audio->channels;

    sio_initpar(&par);
    par.sig = 1;
    par.bits = 16;
    par.le = 1;
    par.rate = 44100;
    par.rchan = channels;
    par.appbufsz = sizeof(buf) / channels;

    wavaBailCondition((hdl = sio_open(audio->source, SIO_REC, 0)) == NULL,
        "Could not open sndio source: %s", audio->source);

    wavaBailCondition(!sio_setpar(hdl, &par) || !sio_getpar(hdl, &par) || par.sig != 1
            || par.le != 1 || par.rate != 44100 || par.rchan != channels,
            "Could not set required audio parameters");

    wavaBailCondition(!sio_start(hdl), "sio_start() failed");

    n = 0;
    while (audio->terminate != 1) {
        wavaBailCondition(sio_read(hdl, buf, sizeof(buf)) == 0,
            "sio_read() failed: %s\n", strerror(errno));

        for (i = 0; i < sizeof(buf)/sizeof(buf[0]); i += 2) {
            if (par.rchan == 1) {
                // sndiod has already taken care of averaging the samples
                audio->audio_out_l[n] = buf[i];
            } else if (par.rchan == 2) {
                audio->audio_out_l[n] = buf[i];
                audio->audio_out_r[n] = buf[i + 1];
            }
            n = (n + 1) % audio->inputsize;
        }
    }

    sio_stop(hdl);
    sio_close(hdl);

    return 0;
}

EXP_FUNC void wavaInputLoadConfig(WAVA *wava) {
    WAVA_AUDIO *audio = &wava->audio;
    wava_config_source config = wava->default_config.config;
    audio->rate = 44100;
    audio->source = (char*)wavaConfigGetString(config, "input", "source", SIO_DEVANY);
}

