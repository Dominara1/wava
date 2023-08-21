#include <cstdio>
#include <ctime>
#include <windows.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <functiondiscoverykeys_devpkey.h>

#include "main.h"

extern "C" {
    #include "shared.h"
}

// This variable is read by the main executable
const char WAVA_DEFAULT_AUDIO_SORUCE[] = "loopback";

using namespace std;

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

static WAVA_AUDIO *audio;
static u32 n;

HRESULT sinkSetFormat(WAVEFORMATEX * pWF) {
    // For the time being, just return OK.
    pWF->wFormatTag = WAVE_FORMAT_PCM;
    pWF->nChannels = audio->channels;
    pWF->nSamplesPerSec = audio->rate;
    pWF->nAvgBytesPerSec = 2*audio->rate*audio->channels;
    pWF->nBlockAlign = 2*audio->channels;
    pWF->wBitsPerSample = 16;
    return ( S_OK ) ;
}

HRESULT sinkCopyData(BYTE * pData, UINT32 NumFrames) {
    float *pBuffer = (float*)pData;
    // convert 32-bit float to something usable
    switch(audio->channels) {
        case 1:
            for(UINT32 i=0; i<NumFrames; i++) {
                audio->audio_out_l[n] = *pBuffer++;
                //audio->audio_out_l[n] += *pBuffer++;
                audio->audio_out_l[n] *= 16383.5f;
                n++;
                if(n == audio->inputsize) n = 0;
            }
            break;
        case 2:
            for(UINT32 i=0; i<NumFrames; i++) {
                audio->audio_out_l[n] = (*pBuffer++)*32767.0f;
                audio->audio_out_r[n] = (*pBuffer++)*32767.0f;
                n++;
                if(n == audio->inputsize) n = 0;
            }
            break;
    }
    return S_OK;
}

//-----------------------------------------------------------
// Record an audio stream from the default audio capture
// device. The RecordAudioStream function allocates a shared
// buffer big enough to hold one second of PCM audio data.
// The function uses this buffer to stream data from the
// capture device. The main loop runs every 1/2 second.
//-----------------------------------------------------------

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

extern "C" {

EXP_FUNC void* wavaInput(void *audiodata) {
    HRESULT hr;
    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
    UINT32 bufferFrameCount;
    UINT32 numFramesAvailable;
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDevice *pDevice = NULL;
    IAudioClient *pAudioClient = NULL;
    IAudioCaptureClient *pCaptureClient = NULL;
    WAVEFORMATEX *pwfx = NULL;
    UINT32 packetLength = 0;
    BYTE *pData;
    DWORD flags;
    audio = (WAVA_AUDIO *)audiodata;
    REFERENCE_TIME hnsDefaultDevicePeriod;

    n = 0; // reset the buffer counter

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoCreateInstance(
            CLSID_MMDeviceEnumerator, NULL,
            CLSCTX_ALL, IID_IMMDeviceEnumerator,
            (void**)&pEnumerator);
    wavaBailCondition(hr, "CoCreateInstance failed - Couldn't get IMMDeviceEnumerator");

    hr = pEnumerator->GetDefaultAudioEndpoint(
            strcmp(audio->source, "loopback") ? eCapture : eRender, eConsole, &pDevice);
    wavaBailCondition(hr, "Failed to get default audio endpoint");

    hr = pDevice->Activate(
        IID_IAudioClient, CLSCTX_ALL,
        NULL, (void**)&pAudioClient);
    wavaBailCondition(hr, "Failed setting up default audio endpoint");

    hr = pAudioClient->GetMixFormat(&pwfx);
    wavaBailCondition(hr, "Failed getting default audio endpoint mix format");

    hr = pAudioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            strcmp(audio->source, "loopback") ? 0 : AUDCLNT_STREAMFLAGS_LOOPBACK,
            hnsRequestedDuration,
            0,
            pwfx,
            NULL);
    wavaBailCondition(hr, "Failed initilizing default audio endpoint");

    // Get the size of the allocated buffer.
    hr = pAudioClient->GetBufferSize(&bufferFrameCount);
    wavaBailCondition(hr, "Failed getting buffer size");

    hr = pAudioClient->GetService(
        IID_IAudioCaptureClient,
        (void**)&pCaptureClient);
    wavaBailCondition(hr, "Failed getting capture service");

    // Set the audio sink format to use.
    hr = sinkSetFormat(pwfx);
    wavaBailCondition(hr, "Failed setting format");

    hr = pAudioClient->Start();  // Start recording.
    wavaBailCondition(hr, "Failed starting capture");

    hr = pAudioClient->GetDevicePeriod(&hnsDefaultDevicePeriod, NULL);
    wavaBailCondition(hr, "Error getting device period");

    LONG lTimeBetweenFires = 1000*audio->latency/audio->rate; // convert to milliseconds

    // Each loop fills about half of the shared buffer.
    while (!audio->terminate) {
        Sleep(lTimeBetweenFires);

        hr = pCaptureClient->GetNextPacketSize(&packetLength);
        wavaBailCondition(hr, "Failure getting buffer size");

        while (packetLength != 0) {
            // Get the available data in the shared buffer.
            hr = pCaptureClient->GetBuffer(
                    &pData,
                    &numFramesAvailable,
                    &flags, NULL, NULL);
            wavaBailCondition(hr, "Failure to capture available buffer data");

            //if (flags & AUDCLNT_BUFFERFLAGS_SILENT) pData = NULL;  // Tell CopyData to write silence.

            // Copy the available capture data to the audio sink.
            hr = sinkCopyData(pData, numFramesAvailable);
            wavaBailCondition(hr, "Failure copying buffer data");

            hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
            wavaBailCondition(hr, "Failed to release buffer");

            hr = pCaptureClient->GetNextPacketSize(&packetLength);
            wavaBailCondition(hr, "Failure getting buffer size");
        }
    }

    hr = pAudioClient->Stop();  // Stop recording.
    wavaBailCondition(hr, "Failure stopping capture");

    CoTaskMemFree(pwfx);
    pEnumerator->Release();
    pDevice->Release();
    pAudioClient->Release();
    pCaptureClient->Release();

    CoUninitialize();

    return 0;
}


EXP_FUNC void wavaInputLoadConfig(WAVA *wava) {
    WAVA_AUDIO *audio = &wava->audio;
    wava_config_source config = wava->default_config.config;

    audio->source = wavaConfigGetString(config, "input", "source", "loopback");
}

} // extern "C"

