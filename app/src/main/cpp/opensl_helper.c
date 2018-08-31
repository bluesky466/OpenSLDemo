#include "opensl_helper.h"

static void bufferQueueCallback(
		SLAndroidSimpleBufferQueueItf queue,
		void* pContext) {
	LOG("bufferQueueCallback");
	OpenSLHelper* pHelper = (OpenSLHelper*) pContext;
	lockNotify(&(pHelper->threadLock));
}

void lockInit(ThreadLock* pThreadLock) {
	pthread_mutex_init(&(pThreadLock->m), NULL);
	pthread_cond_init(&(pThreadLock->c), NULL);
	pThreadLock->s = 1;
}

void lockWait(ThreadLock* pThreadLock) {
	pthread_mutex_lock(&(pThreadLock->m));
	while(!pThreadLock->s) {
		pthread_cond_wait(&(pThreadLock->c), &(pThreadLock->m));
	}
	pThreadLock->s = 0;
	pthread_mutex_unlock(&(pThreadLock->m));
}

void lockNotify(ThreadLock* pThreadLock) {
	pthread_mutex_lock(&(pThreadLock->m));
	pThreadLock->s = 1;
	pthread_cond_signal(&(pThreadLock->c));
	pthread_mutex_unlock(&(pThreadLock->m));
}

void lockDestroy(ThreadLock* pThreadLock) {
	lockNotify(pThreadLock);
	pthread_cond_destroy(&(pThreadLock->c));
	pthread_mutex_destroy(&(pThreadLock->m));
}

void openSLHelperInit(OpenSLHelper* pHelper) {
	//////Thread Lock//////
	lockInit(&(pHelper->threadLock));

	//////Engine Object//////
	slCreateEngine(&(pHelper->engineObject), 0, NULL, 0, NULL, NULL);
	(*pHelper->engineObject)->Realize(pHelper->engineObject, SL_BOOLEAN_FALSE);

	//////Engine Interface//////
	(*pHelper->engineObject)->GetInterface(pHelper->engineObject, SL_IID_ENGINE, &(pHelper->engineInterface));
}

void openSLHelperDestroy(OpenSLHelper* pHelper) {
	//////Thread Lock//////
	lockDestroy(&(pHelper->threadLock));

	//////Player//////
	if(pHelper->playerObject) {
		(*pHelper->playerObject)->Destroy(pHelper->playerObject);
		pHelper->playerObject = NULL;
		pHelper->playInterface = NULL;
	}

	//////Recoder//////
	if(pHelper->recorderObject) {
		(*pHelper->recorderObject)->Destroy(pHelper->recorderObject);
		pHelper->recorderObject = NULL;
		pHelper->recorderInterface = NULL;
	}

	//////Outpute Mix//////
	if(pHelper->outputMixObject) {
		(*pHelper->outputMixObject)->Destroy(pHelper->outputMixObject);
		pHelper->outputMixObject = NULL;
	}

	//////Queue Interface//////
	if(pHelper->queueInterface) {
		pHelper->queueInterface = NULL;
	}

	//////Engine//////
	if(pHelper->engineObject) {
		(*pHelper->engineObject)->Destroy(pHelper->engineObject);
		pHelper->engineObject = NULL;
		pHelper->engineInterface = NULL;
	}
}

void recorderInit(OpenSLHelper* pHelper, SLuint32 channels, SLuint32 samplingRate) {
	//////Source//////
	SLDataLocator_IODevice device;
	device.locatorType = SL_DATALOCATOR_IODEVICE;
	device.deviceType = SL_IODEVICE_AUDIOINPUT;
	device.deviceID =  SL_DEFAULTDEVICEID_AUDIOINPUT;
	device.device = NULL;

	SLDataSource source;
	source.pLocator = &device;
	source.pFormat = NULL; //This parameter is ignored if pLocator is SLDataLocator_IODevice.

	//////Sink//////
	SLDataLocator_AndroidSimpleBufferQueue queue;
	queue.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
	queue.numBuffers = 2;

	SLDataFormat_PCM format;
	format.formatType = SL_DATAFORMAT_PCM;
	format.numChannels = channels;
	format.samplesPerSec = samplingRate;
	format.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
	format.containerSize =  SL_PCMSAMPLEFORMAT_FIXED_16;
	format.channelMask = channels>1 ? SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT : SL_SPEAKER_FRONT_CENTER;
	format.endianness = SL_BYTEORDER_LITTLEENDIAN;

	SLDataSink sink;
	sink.pLocator = &queue;
	sink.pFormat = &format;

	//////Recorder//////
	SLInterfaceID id[] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
	SLboolean required[] = {SL_BOOLEAN_TRUE};

	(*pHelper->engineInterface)->CreateAudioRecorder(
			pHelper->engineInterface,
			&(pHelper->recorderObject),
			&source,
			&sink,
			1,
			id,
			required
	);
	(*pHelper->recorderObject)->Realize(pHelper->recorderObject, SL_BOOLEAN_FALSE);

	//////Register Callback//////
	(*pHelper->recorderObject)->GetInterface(
			pHelper->recorderObject,
			SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
			&(pHelper->queueInterface)
	);

	(*pHelper->queueInterface)->RegisterCallback(
			pHelper->queueInterface,
			bufferQueueCallback,
			pHelper
	);

	//////Start Recording//////
	(*pHelper->recorderObject)->GetInterface(
			pHelper->recorderObject,
			SL_IID_RECORD,
			&(pHelper->recorderInterface)
	);

	(*pHelper->recorderInterface)->SetRecordState(
			pHelper->recorderInterface,
			SL_RECORDSTATE_RECORDING
	);
}

void playerInit(OpenSLHelper* pHelper, SLuint32 channels, SLuint32 samplingRate) {
	//////Source//////
	SLDataLocator_AndroidSimpleBufferQueue queue;
	queue.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
	queue.numBuffers = 2;

	SLDataFormat_PCM format;
	format.formatType = SL_DATAFORMAT_PCM;
	format.numChannels = channels;
	format.samplesPerSec = samplingRate;
	format.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
	format.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
	format.channelMask = channels>1 ? SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT : SL_SPEAKER_FRONT_CENTER;
	format.endianness = SL_BYTEORDER_LITTLEENDIAN;

	SLDataSource source;
	source.pLocator = &queue;
	source.pFormat = &format;

	//////Sink//////
	SLInterfaceID id[] = {SL_IID_VOLUME};
	SLboolean required[] = {SL_BOOLEAN_FALSE};
	(*pHelper->engineInterface)->CreateOutputMix(
			pHelper->engineInterface,
			&(pHelper->outputMixObject),
			1,
			id,
			required
	);
	(*pHelper->outputMixObject)->Realize(
			pHelper->outputMixObject,
			SL_BOOLEAN_FALSE
	);

	SLDataLocator_OutputMix outputMix;
	outputMix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
	outputMix.outputMix = pHelper->outputMixObject;

	SLDataSink sink;
   	sink.pLocator = &outputMix;
   	sink.pFormat = NULL; //This parameter is ignored if pLocator is SLDataLocator_IODevice or SLDataLocator_OutputMix.

	//////Player//////
	id[0] = SL_IID_ANDROIDSIMPLEBUFFERQUEUE;
	required[0] = SL_BOOLEAN_TRUE;
	(*pHelper->engineInterface)->CreateAudioPlayer(
			pHelper->engineInterface,
			&(pHelper->playerObject),
			&source,
			&sink,
			1,
			id,
			required
	);
	(*pHelper->playerObject)->Realize(pHelper->playerObject, SL_BOOLEAN_FALSE);

	//////Register Callback//////
	(*pHelper->playerObject)->GetInterface(
			pHelper->playerObject,
			SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
			&(pHelper->queueInterface)
	);
	(*pHelper->queueInterface)->RegisterCallback(
			pHelper->queueInterface,
			bufferQueueCallback,
			pHelper
	);

	//////Begin Playing//////
	(*pHelper->playerObject)->GetInterface(
			pHelper->playerObject,
			SL_IID_PLAY,
			&(pHelper->playInterface)
	);
	(*pHelper->playInterface)->SetPlayState(
			pHelper->playInterface,
			SL_PLAYSTATE_PLAYING
	);
}
