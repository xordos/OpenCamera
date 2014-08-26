/*
The contents of this file are subject to the Mozilla Public License
Version 1.1 (the "License"); you may not use this file except in
compliance with the License. You may obtain a copy of the License at
http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS"
basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
License for the specific language governing rights and limitations
under the License.

The Original Code is collection of files collectively known as Open Camera.

The Initial Developer of the Original Code is Almalence Inc.
Portions created by Initial Developer are Copyright (C) 2013
by Almalence Inc. All Rights Reserved.
*/

#include <stdio.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>

#include "ImageConversionUtils.h"

#include "bestshot.h"

#define MAX_BEST_FRAMES 10

static unsigned char *yuv[MAX_BEST_FRAMES] = {NULL};
static void *instance = NULL;
static int almashot_inited = 0;


// This triggers openmp constructors and destructors to be called upon library load/unload
void __attribute__((constructor)) initialize_openmp() {}
void __attribute__((destructor)) release_openmp() {}


extern "C" JNIEXPORT jstring JNICALL Java_com_mobiroo_n_almalence_plugins_processing_bestshot_AlmaShotBestShot_Initialize
(
	JNIEnv* env,
	jobject thiz
)
{
	char status[1024];
	int err=0;

	if (almashot_inited == 0)
	{
		err = AlmaShot_Initialize(0);

		if (err == 0)
			almashot_inited = 1;
	}

	sprintf (status, " err: %d\n", err);
	return env->NewStringUTF(status);
}


extern "C" JNIEXPORT jint JNICALL Java_com_mobiroo_n_almalence_plugins_processing_bestshot_AlmaShotBestShot_Release
(
	JNIEnv*,
	jobject
)
{
	int i;

	if (almashot_inited == 1)
	{
		AlmaShot_Release();

		almashot_inited = 0;
	}

	return 0;
}


extern "C" JNIEXPORT jstring JNICALL Java_com_mobiroo_n_almalence_plugins_processing_bestshot_AlmaShotBestShot_ConvertFromJpeg
(
	JNIEnv* env,
	jobject thiz,
	jintArray in,
	jintArray in_len,
	jint nFrames,
	jint sx,
	jint sy
)
{
	int i;
	int *jpeg_length;
	unsigned char * *jpeg;
	char status[1024];

	Uint8 *inp[4];
	int x, y;
	int x0_out, y0_out, w_out, h_out;

	jpeg = (unsigned char**)env->GetIntArrayElements(in, NULL);
	jpeg_length = (int*)env->GetIntArrayElements(in_len, NULL);

	DecodeAndRotateMultipleJpegs(yuv, jpeg, jpeg_length, sx, sy, nFrames, 0, 0, 0, false);

	env->ReleaseIntArrayElements(in, (jint*)jpeg, JNI_ABORT);
	env->ReleaseIntArrayElements(in_len, (jint*)jpeg_length, JNI_ABORT);

	sprintf (status, "frames total: %d\n", (int)nFrames);
	return env->NewStringUTF(status);
}


extern "C" JNIEXPORT jint JNICALL Java_com_mobiroo_n_almalence_plugins_processing_bestshot_AlmaShotBestShot_BestShotProcess
(
	JNIEnv* env,
	jobject thiz,
	jint nFrames,
	jint sx,
	jint sy,
	jintArray in
)
{
	int     fullScanMode = 1;
	int    	BestFrames[MAX_BEST_FRAMES] = {0};
	float 	FramesScores[MAX_BEST_FRAMES] = {0};
	int 	nFramesToSelect = 1;

	unsigned char * *jpeg;
	jpeg = (unsigned char**)env->GetIntArrayElements(in, NULL);

	BestShot_Select(yuv, sx, sy, nFrames, fullScanMode, BestFrames, FramesScores, nFramesToSelect);

	for (int i=0; i<nFrames; ++i)
	{
		free(yuv[i]);
		yuv[i] = NULL;
//		if (i!=BestFrames[0])
//			free (jpeg[i]);
	}

	env->ReleaseIntArrayElements(in, (jint*)jpeg, JNI_ABORT);

	return (jint)BestFrames[0];
}
