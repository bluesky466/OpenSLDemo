#include <jni.h>
#include <stdlib.h>
#include "opensl_helper.h"

#define SAMPLERATE 44100
#define CHANNELS 1
#define PERIOD_TIME 20 //ms
#define FRAME_SIZE SAMPLERATE*PERIOD_TIME/1000
#define BUFFER_SIZE FRAME_SIZE*CHANNELS

#define PCM_FILE_PATH "/sdcard/audio.pcm"

#ifdef __cplusplus
extern "C" {
#endif

static volatile int gPlaying = 0;
static volatile int gRecording = 0;

static SLuint32 convertSampleRate(int sampleRate){
	switch(sampleRate) {
		case 8000:
			return SL_SAMPLINGRATE_8;
        case 11025:
            return SL_SAMPLINGRATE_11_025;
		case 16000:
            return SL_SAMPLINGRATE_16;
        case 22050:
            return SL_SAMPLINGRATE_22_05;
        case 24000:
            return SL_SAMPLINGRATE_24;
        case 32000:
            return SL_SAMPLINGRATE_32;
            break;
        case 44100:
	        return SL_SAMPLINGRATE_44_1;
	    case 48000:
	        return SL_SAMPLINGRATE_48;
	    case 64000:
	        return SL_SAMPLINGRATE_64;
	    case 88200:
	        return SL_SAMPLINGRATE_88_2;
	    case 96000:
	        return SL_SAMPLINGRATE_96;
	    case 192000:
	        return SL_SAMPLINGRATE_192;
	    default:
	        return 0;
	}
}

JNIEXPORT void JNICALL Java_me_linjw_opensldemo_OpenSLDemo_startRecord(JNIEnv * env, jobject obj) {
	LOG("start record");
	OpenSLHelper helper;
	openSLHelperInit(&helper);
	recorderInit(&helper, CHANNELS, convertSampleRate(SAMPLERATE));

	FILE * fp = fopen(PCM_FILE_PATH, "wb");

	short* buffer = (short *) calloc(BUFFER_SIZE, sizeof(short));

	gRecording = 1;
	while(gRecording) {
		lockWait(&(helper.threadLock));
		LOG("recording...");
		(*helper.queueInterface)->Enqueue(helper.queueInterface, buffer, BUFFER_SIZE*sizeof(short));
		fwrite((unsigned char *)buffer, BUFFER_SIZE*sizeof(short), 1, fp);
	}

	openSLHelperDestroy(&helper);
	free(buffer);
	fclose(fp);
}

JNIEXPORT void JNICALL Java_me_linjw_opensldemo_OpenSLDemo_stopRecord(JNIEnv * env, jobject obj) {
	LOG("stop record");
	gRecording = 0;
}

JNIEXPORT void JNICALL Java_me_linjw_opensldemo_OpenSLDemo_startPlay(JNIEnv * env, jobject obj) {
	LOG("start play");
	OpenSLHelper helper;
	openSLHelperInit(&helper);
	playerInit(&helper, CHANNELS, convertSampleRate(SAMPLERATE));

	FILE * fp = fopen(PCM_FILE_PATH, "rb");

	short* buffer = (short *) calloc(BUFFER_SIZE, sizeof(short));

	gPlaying = 1;
	while (gPlaying && !feof(fp)) {
		lockWait(&(helper.threadLock));
		if (fread((unsigned char *)buffer, BUFFER_SIZE*sizeof(short), 1, fp) != 1) {
	        break;
	    }
		LOG("playing...");
		(*helper.queueInterface)->Enqueue(helper.queueInterface, buffer, BUFFER_SIZE*sizeof(short));
	}
	openSLHelperDestroy(&helper);
	free(buffer);
	fclose(fp);
}

JNIEXPORT void JNICALL Java_me_linjw_opensldemo_OpenSLDemo_stopPlay(JNIEnv * env, jobject obj) {
	LOG("stop play");
	gPlaying = 0;
}

#ifdef __cplusplus
}
#endif
