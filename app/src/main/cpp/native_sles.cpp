//
// Created by youngpark on 2023/4/4.
//

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>


// for __android_log_print(ANDROID_LOG_INFO, "YourApp", "formatted message");
// #include <android/log.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <stdbool.h>
#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))

static const char android[] =

#include "android_clip.h"

static const char pcm1[] =

#include "pcm1.h"

static const char pcm2[] =

#include "pcm2.h"

// engine interfaces
static SLObjectItf engineObject = NULL;
static SLEngineItf engineEngine;

// output mix interfaces
static SLObjectItf outputMixObject = NULL;
static SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;

// buffer queue player interfaces
static SLObjectItf bqPlayerObject = NULL;
static SLPlayItf bqPlayerPlay;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
static SLEffectSendItf bqPlayerEffectSend;
static SLMuteSoloItf bqPlayerMuteSolo;
static SLVolumeItf bqPlayerVolume;
static SLmilliHertz bqPlayerSampleRate = 0;
static jint bqPlayerBufSize = 0;
static short *resampleBuf = NULL;
// a mutext to guard against re-entrance to record & playback
// as well as make recording and playing back to be mutually exclusive
// this is to avoid crash at situations like:
//    recording is in session [not finished]
//    user presses record button and another recording coming in
// The action: when recording/playing back is not finished, ignore the new request
static pthread_mutex_t audioEngineLock = PTHREAD_MUTEX_INITIALIZER;

// aux effect on the output mix, used by the buffer queue player
static const SLEnvironmentalReverbSettings reverbSettings =
        SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
#define offset 12

#define C1 0
#define C1S 1
#define D1 2
#define D1S 3
#define E1 4
#define F1 5
#define F1S 6
#define G1 7
#define G1S 8
#define A1 9
#define A1S 10
#define B1 11

#define C2 C1+12
#define C2S C1S+12
#define D2 D1+12
#define D2S D1S+12
#define E2 E1+12
#define F2 F1+12
#define F2S F1S+12
#define G2 G1+12
#define G2S G1S+12
#define A2 A1+12
#define A2S A1S+12
#define B2 B1+12

#define C3 C2+offset
#define C3S C2S+offset
#define D3 D2+offset
#define D3S D2S+offset
#define E3 E2+offset
#define F3 F2+offset
#define F3S F2S+offset
#define G3 G2+offset
#define G3S G2S+offset
#define A3 A2+offset
#define A3S A2S+offset
#define B3 B2+offset

#define C4 C3+offset
#define C4S C3S+offset
#define D4 D3+offset
#define D4S D3S+offset
#define E4 E3+offset
#define F4 F3+offset
#define F4S F3S+offset
#define G4 G3+offset
#define G4S G3S+offset
#define A4 A3+offset
#define A4S A3S+offset
#define B4 B3+offset

#define C5 C4+offset
#define C5S C4S+offset
#define D5 D4+offset
#define D5S D4S+offset
#define E5 E4+offset
#define F5 F4+offset
#define F5S F4S+offset
#define G5 G4+offset
#define G5S G4S+offset
#define A5 A4+offset
#define A5S A4S+offset
#define B5 B4+offset

#define L1 0
#define L1S 1
#define L2 2
#define L2S 3
#define L3 4
#define L4 5
#define L4S 6
#define L5 7
#define L5S 8
#define L6 9
#define L6S 10
#define L7 11

#define LL1 L1-12
#define LL1S L1S-12
#define LL2 L2-12
#define LL2S L2S-12
#define LL3 L3-12
#define LL4 L4-12
#define LL4S L4S-12
#define LL5 L5-12
#define LL5S L5S-12
#define LL6 L6-12
#define LL6S L6S-12
#define LL7 L7-12

#define M1 L1+12
#define M1S L1S+12
#define M2 L2+12
#define M2S L2S+12
#define M3 L3+12
#define M4 L4+12
#define M4S L4S+12
#define M5 L5+12
#define M5S L5S+12
#define M6 L6+12
#define M6S L6S+12
#define M7 L7+12

#define H1 M1+12
#define H1S M1S+12
#define H2 M2+12
#define H2S M2S+12
#define H3 M3+12
#define H4 M4+12
#define H4S M4S+12
#define H5 M5+12
#define H5S M5S+12
#define H6 M6+12
#define H6S M6S+12
#define H7 M7+12

#define S 0

const char music1[] = {
        M3, 4, L6, 4, M1, 4, M3, 4, M4S, 4, L7, 4, M2, 4, M4, 4,
        M5, 4, M1, 4, M3, 4, M5, 4, M4S, 4, L7, 4, M2, 4, M4, 4,
        M3, 4, L6, 4, M1, 4, M3, 4, M4S, 4, L7, 4, M2, 4, M4, 4,
        M5, 4, M1, 4, M3, 4, M5, 4, M4S, 4, L7, 4, M2, 4, M4, 4,
        M5, 4, L6, 4, M1, 4, M5, 4, M6, 4, L7, 4, M2, 4, M6, 4,

        M7, 4, M1, 4, M3, 4, M7, 4, M6, 4, L7, 4, M2, 4, M6, 4,
        M5, 4, L6, 4, M1, 4, M5, 4, M6, 4, L7, 4, M2, 4, M6, 4,
        M7, 4, M1, 4, M3, 4, M7, 4, M6, 4, L7, 4, H2, 4, M4, 4,
        H1, 4, M3, 4, M6, 4, H1, 4, M7, 4, M3, 4, M5S, 4, M7, 4,
        H2, 4, M3, 4, M7, 4, H2, 4, H1, 4, M3, 4, M6, 4, H1, 4,

        H1, 4, M3, 4, M6, 4, H1, 4, M7, 4, M3, 4, M5S, 4, M7, 4,
        H4, 4, M6, 4, H1, 4, H4, 4, H3, 4, M6, 4, H1, 4, H3, 4,
        H1, 4, M3, 4, M6, 4, H1, 4, M7, 4, M3, 4, M5S, 4, M7, 4,
        H2, 4, M3, 4, M7, 4, H2, 4, H1, 4, H1, 4, H2, 4, H3, 4,
        H1, 4, M3, 4, M6, 4, H1, 4, H4, 4, H3, 4, H2, 4, H1, 4,

        M7, 2, H1, 2, M7, 2, H1, 2, M7, 4, H1, 4, M7, 8, S, 4,
        H3, 4, M6, 4, H1, 4, H3, 4, H2, 4, M5S, 4, M7, 4, H2, 4,
        H4, 4, M6, 4, H1, 4, H4, 4, H3, 4, M6, 4, H1, 4, H3, 4,
        H3, 4, M6, 4, H1, 4, H3, 4, H2, 4, M6S, 4, M7, 4, H2, 4,
        H6, 4, H1, 4, H3, 4, H6, 4, H5, 4, H1, 4, H3, 4, H6, 4,

        H3, 4, M6, 4, H1, 4, H3, 4, H2, 4, M5S, 4, M7, 4, H2, 4,
        H4, 4, M6, 4, H1, 4, H4, 4, H3, 4, M6, 4, H1, 4, H3, 4,
        H3, 4, M6, 4, H1, 4, H3, 4, H6, 4, H5, 4, H4, 4, H3, 4,
        H2, 2, H3, 2, H2, 2, H3, 2, H2, 4, H3, 4, H2, 8, S, 4,
        M6, 16, M6S, 16, M7, 8,

        M7, 4, M6, 16, M5S, 16,
        M7, 16, H1, 16, H2, 8,
        H2, 4, H4, 16, H6, 8, H7, 8

};

const char music2[] = {
        S, 4, S, 4, S, 4, S, 4, S, 4, S, 4, S, 4, S, 4,
        S, 4, S, 4, S, 4, S, 4, S, 4, S, 4, S, 4, S, 4,
        S, 4, S, 4, S, 4, S, 4, S, 4, S, 4, S, 4, S, 4,
        S, 4, S, 4, S, 4, S, 4, S, 4, S, 4, S, 4, S, 4,
        M3, 4, S, 4, S, 4, M3, 4, M4S, 4, S, 4, S, 4, M4, 4,

        M5, 4, S, 4, S, 4, M5, 4, M4S, 4, S, 4, S, 4, M4, 4,
        M3, 4, S, 4, S, 4, M3, 4, M4S, 4, S, 4, S, 4, M4, 4,
        M5, 4, S, 4, S, 4, M5, 4, M4S, 4, S, 4, M7, 4, S, 4,
        S, 4, S, 4, S, 4, S, 4, S, 4, S, 4, S, 4, S, 4,
        S, 4, S, 4, S, 4, S, 4, S, 4, S, 4, S, 4, S, 4,

        S, 4, S, 4, S, 4, S, 4, S, 4, S, 4, S, 4, S, 4,
        S, 4, S, 4, S, 4, S, 4, S, 4, S, 4, S, 4, S, 4,
        M6, 4, S, 4, S, 4, M6, 4, M5, 4, S, 4, S, 4, M5S, 4,
        M7, 4, S, 4, S, 4, M7, 4, M6, 4, M6, 4, M7, 4, H1, 4,
        M6, 4, S, 4, S, 4, S, 4, H2, 4, H1, 4, M7, 4, M6, 4,

        M5, 2, M6, 2, M5, 2, M6, 2, M5, 4, M6, 4, M5, 8, S, 4,
        H1, 4, M3, 4, M6, 4, H1, 4, M7, 4, M3, 4, S, 4, M7, 4,
        H2, 4, H3, 4, M6, 4, H2, 4, H1, 4, H3, 4, M6, 4, H1, 4,
        H1, 4, H3, 4, M6, 4, H1, 4, M7, 4, M3, 4, S, 4, M7, 4,
        H4, 4, M6, 4, H1, 4, H4, 4, H3, 4, M6, 4, H1, 4, H3, 4,

        H1, 4, M3, 4, M6, 4, H1, 4, M7, 4, M3, 4, M5, 4, M7, 4,
        H2, 4, M3, 4, M6, 4, H2, 4, H1, 4, M3, 4, M6, 4, H1, 4,
        H1, 4, M3, 4, M6, 4, H1, 4, H4, 4, H3, 4, H2, 4, H1, 4,
        M7, 2, H1, 2, M7, 2, H1, 2, M7, 4, H1, 4, M7, 8, S, 4,
        L7, 16, M1, 16, M2, 8,

        M2, 4, M1, 16, L7, 16,
        M2, 16, M3, 16, M6, 8,
        M4, 4, M5S, 16, M6S, 8, M7, 8
};

#define MUSIC_LEN 10*5//多少行

// synthesized sawtooth clip
#define SAWTOOTH_FRAME 300
#define SAWTOOTH_LENGTH 8*2*MUSIC_LEN//8个音符（音名 + 节拍）
#define SAWTOOTH_FRAMES 273600

static short sawtoothBuffer[SAWTOOTH_FRAMES];

// pointer and size of the next player buffer to enqueue, and number of remaining buffers
static short *nextBuffer;
static unsigned nextSize;
static int nextCount;

short mixPCM(short a, short b) {
    int c = a + b;
    if (c < -32768) {
        c = -32768;
    } else if (c > 32767) {
        c = 32767;
    }
    return c;
}

double standardFrequency = 440.0;

// 用于存放 0 ~ 127 音高 对应的 音符频率
double noteFrequency[128];//12

short getSinPCM(double x, int freq) {
    //16000 hz +-32768
    return 32768 * sin(2 * 3.1415926535 * freq * (x / 8000));
}

short getTriPCM(double x, int freq) {
    double T = 1.0 / freq;
    double A = 32768 / T;
    __android_log_print(ANDROID_LOG_INFO, "test", "%d,%d", T,A);
    x= fmod(x/8000, T);
    __android_log_print(ANDROID_LOG_INFO, "test", "x = %d", x);
    if (x < T / 2.0) {
        return A * x;
    } else {
        return 32768/2 - A * (x - T / 2.0);
    }
}

short getFastTriPCM(double x, int freq) {
    //16000 hz +-32768
    return 32768 * ((x / 8000) * freq)/2;
}

short getSquarePCM(double x, int freq) {
    //16000 hz +-32768
    return ((int) ((x / 8000) * freq)) % 2 == 0 ? 32768 / 3 : -32768 / 3;
}

int compose_offset = 0;
int max_index = 0;

void composeMusic(const char music[], short (*getPCM)(double,int)) {
    unsigned i, j;
    int bufferIndex = compose_offset;
    compose_offset++;
    for (j = 0; j < 205; j++) {
        int frameLength = music[j * 2 + 1] * SAWTOOTH_FRAME;
        for (i = 0; i < frameLength; ++i) {
            int step = music[j * 2];
            int freq = noteFrequency[music[j * 2] + 60];
            if (step != S) {
                sawtoothBuffer[bufferIndex] = mixPCM(sawtoothBuffer[bufferIndex],
                                                     getPCM(i, freq));
            }
            bufferIndex++;
        }
    }
    max_index = max_index > bufferIndex ? max_index : bufferIndex;
}

// synthesize a mono sawtooth wave and place it into a buffer (called automatically on load)
__attribute__((constructor)) static void onDlOpen(void) {
    for (int i = 0; i <= 127; i++) {
        noteFrequency[i] = (standardFrequency / 32.0) * pow(2, (i - 9.0) / 12.0);
    }
    memset(sawtoothBuffer, 0, sizeof(sawtoothBuffer));
    int total = 0;
    for (int j = 0; j < 205; j++) {
        int frameLength1 = music1[j * 2 + 1];
        int frameLength2 = music2[j * 2 + 1];
        total += frameLength1;
        if (frameLength1 != frameLength2) {
            __android_log_print(ANDROID_LOG_INFO, "test", "%d", j);
        }
        __android_log_print(ANDROID_LOG_INFO, "total test", "total=%d", total);
    }
    composeMusic(music1, getSquarePCM);
//    composeMusic(music2, getFastTriPCM);
    composeMusic(music2, getSquarePCM);
}

void releaseResampleBuf(void) {
    if (0 == bqPlayerSampleRate) {
        /*
         * we are not using fast path, so we were not creating buffers, nothing to do
         */
        return;
    }

    free(resampleBuf);
    resampleBuf = NULL;
}

/*
 * Only support up-sampling
 */
short *createResampledBuf(uint32_t idx, uint32_t srcRate, unsigned *size) {
    short *src = NULL;
    short *workBuf;
    int upSampleRate;
    int32_t srcSampleCount = 0;

    if (0 == bqPlayerSampleRate) {
        return NULL;
    }
    if (bqPlayerSampleRate % srcRate) {
        /*
         * simple up-sampling, must be divisible
         */
        return NULL;
    }
    upSampleRate = bqPlayerSampleRate / srcRate;

    switch (idx) {
        case 0:
            return NULL;
        case 1: // ANDROID_CLIP
            srcSampleCount = sizeof(android) >> 1;
            src = (short *) android;
            break;
        case 2: // SAWTOOTH_CLIP
            srcSampleCount = SAWTOOTH_FRAMES;
            src = sawtoothBuffer;
            break;
        case 3:
            srcSampleCount = sizeof(pcm2) >> 1;
            src = (short *) pcm2;
            break;
        default:
            assert(0);
    }

    resampleBuf = (short *) malloc((srcSampleCount * upSampleRate) << 1);
    if (resampleBuf == NULL) {
        return resampleBuf;
    }
    workBuf = resampleBuf;
    for (int sample = 0; sample < srcSampleCount; sample++) {
        for (int dup = 0; dup < upSampleRate; dup++) {
            *workBuf++ = src[sample];
        }
    }

    *size = (srcSampleCount * upSampleRate) << 1;     // sample format is 16 bit
    return resampleBuf;
}

// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    assert(bq == bqPlayerBufferQueue);
    assert(NULL == context);
    // for streaming playback, replace this test by logic to find and fill the next buffer
    if (NULL != nextBuffer && 0 != nextSize) {
        SLresult result;
        // enqueue another buffer
        result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, nextBuffer, nextSize);
        // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
        // which for this code example would indicate a programming error
        if (SL_RESULT_SUCCESS != result) {
            pthread_mutex_unlock(&audioEngineLock);
        }
        (void) result;
    } else {
        releaseResampleBuf();
        pthread_mutex_unlock(&audioEngineLock);
    }
}


// create the engine and output mix objects
void createEngine() {
    SLresult result;

    // create engine
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // realize the engine
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // get the engine interface, which is needed in order to create other objects
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // create output mix, with environmental reverb specified as a non-required interface
    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // realize the output mix
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // get the environmental reverb interface
    // this could fail if the environmental reverb effect is not available,
    // either because the feature is not present, excessive CPU load, or
    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &reverbSettings);
        (void) result;
    }
    // ignore unsuccessful result codes for environmental reverb, as it is optional for this example

}


// create buffer queue audio player
void createBufferQueueAudioPlayer(int sampleRate, int bufSize) {
    SLresult result;
    if (sampleRate >= 0 && bufSize >= 0) {
        bqPlayerSampleRate = sampleRate * 1000;
        /*
         * device native buffer size is another factor to minimize audio latency, not used in this
         * sample: we only play one giant buffer here
         */
        bqPlayerBufSize = bufSize;
    }

    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_8,
                                   SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    /*
     * Enable Fast Audio when possible:  once we set the same rate to be the native, fast audio path
     * will be triggered
     */
    if (bqPlayerSampleRate) {
        format_pcm.samplesPerSec = bqPlayerSampleRate;       //sample rate in mili second
    }
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    /*
     * create audio player:
     *     fast audio does not support when SL_IID_EFFECTSEND is required, skip it
     *     for fast audio case
     */
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_EFFECTSEND,
            /*SL_IID_MUTESOLO,*/};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
            /*SL_BOOLEAN_TRUE,*/ };

    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk,
                                                bqPlayerSampleRate ? 2 : 3, ids, req);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // realize the player
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // get the play interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // get the buffer queue interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                             &bqPlayerBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // register callback on the buffer queue
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // get the effect send interface
    bqPlayerEffectSend = NULL;
    if (0 == bqPlayerSampleRate) {
        result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_EFFECTSEND,
                                                 &bqPlayerEffectSend);
        assert(SL_RESULT_SUCCESS == result);
        (void) result;
    }

#if 0   // mute/solo is not supported for sources that are known to be mono, as this is
    // get the mute/solo interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_MUTESOLO, &bqPlayerMuteSolo);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
#endif

    // get the volume interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // set the player's state to playing
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
}

// expose the volume APIs to Java for one of the 3 players

static SLVolumeItf getVolume() {
    return bqPlayerVolume;
}

// select the desired clip and play count, and enqueue the first buffer if idle
bool selectClip(int which, int count) {
    if (pthread_mutex_trylock(&audioEngineLock)) {
        // If we could not acquire audio engine lock, reject this request and client should re-try
        return JNI_FALSE;
    }
    switch (which) {
        case 0:     // CLIP_NONE
            nextBuffer = (short *) NULL;
            nextSize = 0;
            break;
        case 1:     // CLIP_ANDROID
            nextBuffer = createResampledBuf(1, SL_SAMPLINGRATE_8, &nextSize);
            if (!nextBuffer) {
                nextBuffer = (short *) android;
                nextSize = sizeof(android);
            }
            break;
        case 2:    // CLIP_SAWTOOTH
            nextBuffer = createResampledBuf(2, SL_SAMPLINGRATE_8, &nextSize);
            if (!nextBuffer) {
                nextBuffer = (short *) sawtoothBuffer;
                nextSize = sizeof(sawtoothBuffer);
            }
            break;
        case 3:
            nextBuffer = createResampledBuf(3, SL_SAMPLINGRATE_8, &nextSize);
            if (!nextBuffer) {
                nextBuffer = (short *) pcm2;
                nextSize = sizeof(pcm2);
            }
            break;
        default:
            nextBuffer = NULL;
            nextSize = 0;
            break;
    }
    if (nextSize > 0) {
        // here we only enqueue one buffer because it is a long clip,
        // but for streaming playback we would typically enqueue at least 2 buffers to start
        SLresult result;
        result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, nextBuffer, nextSize);
        if (SL_RESULT_SUCCESS != result) {
            pthread_mutex_unlock(&audioEngineLock);
            return JNI_FALSE;
        }
    } else {
        pthread_mutex_unlock(&audioEngineLock);
    }

    return JNI_TRUE;
}

void shutdown() {

    // destroy buffer queue audio player object, and invalidate all associated interfaces
    if (bqPlayerObject != NULL) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = NULL;
        bqPlayerPlay = NULL;
        bqPlayerBufferQueue = NULL;
        bqPlayerEffectSend = NULL;
        bqPlayerMuteSolo = NULL;
        bqPlayerVolume = NULL;
    }

    // destroy output mix object, and invalidate all associated interfaces
    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        outputMixEnvironmentalReverb = NULL;
    }

    // destroy engine object, and invalidate all associated interfaces
    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }

    pthread_mutex_destroy(&audioEngineLock);
}

const int CLIP_NONE = 0;
const int CLIP_HELLO = 1;
const int CLIP_SAWTOOTH = 2;

extern "C" void initSL() {
    createEngine();
    int sampleRate = 16000;
    int bufSize = 2048;
    createBufferQueueAudioPlayer(sampleRate, bufSize);
    selectClip(CLIP_SAWTOOTH, 2);
}