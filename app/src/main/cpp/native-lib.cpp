// Write C++ code here.
//
// Do not forget to dynamically load the C++ library into your application.
//
// For instance,
//
// In MainActivity.java:
//    static {
//       System.loadLibrary("videoapp");
//    }
//
// Or, in MainActivity.kt:
//    companion object {
//      init {
//         System.loadLibrary("videoapp")
//      }
//    }
#include <jni.h>
#include <string>
#include <unistd.h>

extern "C" {
JNIEXPORT jstring JNICALL
Java_com_example_videoapp_uilts_FFmepe_(JNIEnv *env, jobject  /* this */) {
//    unsigned ver = avformat_version();
//    char info[40000] = {0};
//    sprintf(info, "avformat_version %u:", ver);
//    return env->NewStringUTF(info);
}
#include "include/libavcodec/avcodec.h"
#include "include/libavformat/avformat.h"
#include "include/libavfilter/avfilter.h"
#include "include/libavcodec/jni.h"
extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_videoapp_uilts_FFmpeg_ffmpegVersion(JNIEnv *env, jobject thiz) {
    unsigned ver = avformat_version();
    char info[40000] = {0};
    sprintf(info, "avformat_version %u:", ver);
    return env->NewStringUTF(info);
}
}