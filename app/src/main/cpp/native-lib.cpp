//// Write C++ code here.
////
//// Do not forget to dynamically load the C++ library into your application.
////
//// For instance,
////
//// In MainActivity.java:
////    static {
////       System.loadLibrary("videoapp");
////    }
////
//// Or, in MainActivity.kt:
////    companion object {
////      init {
////         System.loadLibrary("videoapp")
////      }
////    }
//
#include <jni.h>
#include <cstdio>
#include <string>
#include <vector>
#include <mutex>
#include <android/log.h>
extern "C" {
#include "libavformat/avformat.h"
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_videoapp_utils_FFmpeg_ffmpegVersion(JNIEnv *env, jobject /*thiz*/) {
    unsigned ver = avformat_version();
    char buf[64];
    std::snprintf(buf, sizeof(buf), "avformat_version=%u", ver);
    return env->NewStringUTF(buf);
}
extern "C" {
#include "libavcodec/avcodec.h"
JNIEXPORT jstring JNICALL
Java_com_example_videoapp_utils_FFmpeg_ffmpegConfig(JNIEnv *env, jobject thiz) {
    return env->NewStringUTF(avcodec_configuration());
}
}

#include <jni.h>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_videoapp_utils_FFmpeg_hasGifEncoder(JNIEnv *env, jobject /*thiz*/) {
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_GIF);
    return codec ? JNI_TRUE : JNI_FALSE;
}

