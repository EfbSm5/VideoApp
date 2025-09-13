//
// Created by EfbSm5 on 2025/9/13.
//
#include <jni.h>
#include <string>
#include <unistd.h>
#include "include/libavcodec/avcodec.h"
#include "include/libavformat/avformat.h"
#include "include/libavfilter/avfilter.h"
#include "include/libavcodec/jni.h"
#include <vector>
#include <mutex>
#include "ffmpeg_entry.h"

extern "C"
JNIEXPORT jstring
JNICALL
Java_com_example_videoapp_uilts_FFmpeg_ffmpegVersion(JNIEnv *env, jobject thiz) {
    unsigned ver = avformat_version();
    char info[40000] = {0};
    sprintf(info, "avformat_version %u:", ver);
    return env->NewStringUTF(info);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_videoapp_uilts_FFmpeg_abort(JNIEnv*, jclass) {
    ffmpeg_request_abort();
}
// 中止

static JavaVM *g_vm = nullptr;
static jclass g_FFmpegClass = nullptr;
static jmethodID g_onLogMethod = nullptr;

// 可选：缓存一个 Java 回调接口对象
static jobject g_logCallbackObj = nullptr;
static jmethodID g_logCallback_onMsg = nullptr;

static std::mutex g_cbMutex;

// 把 C 层的回调转成 Java 调用
static void c_log_callback(int level, const char *msg) {
    std::lock_guardstd::mutex lock(g_cbMutex);
    if (!g_vm) return;
    JNIEnv *env = nullptr;
    if (g_vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
    }

    if (g_logCallbackObj && g_logCallback_onMsg) {
        jstring jmsg = env->NewStringUTF(msg ? msg : "");
        env->CallVoidMethod(g_logCallbackObj, g_logCallback_onMsg, (jint) level, jmsg);
        env->DeleteLocalRef(jmsg);
    } else if (g_FFmpegClass && g_onLogMethod) {
        jstring jmsg = env->NewStringUTF(msg ? msg : "");
        env->CallStaticVoidMethod(g_FFmpegClass, g_onLogMethod, (jint) level, jmsg);
        env->DeleteLocalRef(jmsg);
    }
}

extern "C"
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *) {
    g_vm = vm;
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_videoapp_utils_FFmpeg_nativeInit(JNIEnv *env, jclass clazz) {
    if (!g_FFmpegClass) {
        g_FFmpegClass = (jclass) env->NewGlobalRef(clazz);
        g_onLogMethod = env->GetStaticMethodID(clazz, "onNativeLog", "(ILjava/lang/String;)V");
    }
    ffmpeg_set_log_callback(c_log_callback);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_videoapp_uilts_FFmpeg_setLogCallback(JNIEnv *env, jobject thiz, jobject cb) {
    std::lock_guardstd::mutex lock(g_cbMutex);
    if (g_logCallbackObj) {
        env->DeleteGlobalRef(g_logCallbackObj);
        g_logCallbackObj = nullptr;
        g_logCallback_onMsg = nullptr;
    }
    if (callbackObj) {
        jclass cls = env->GetObjectClass(callbackObj);
        g_logCallback_onMsg = env->GetMethodID(cls, "onLog", "(ILjava/lang/String;)V");
        g_logCallbackObj = env->NewGlobalRef(callbackObj);
    }
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_videoapp_utils_FFmpeg_run(JNIEnv env, jclass, jobjectArray cmdArray) {
    int argc = env->GetArrayLength(cmdArray);
    std::vectorstd::string args;
    args.reserve(argc);
    std::vector<char> argv(argc + 1);

    复制
    for (int i = 0; i < argc; ++i) {
        jstring jstr = (jstring) env->GetObjectArrayElement(cmdArray, i);
        const char *cstr = env->GetStringUTFChars(jstr, nullptr);
        args.emplace_back(cstr);
        env->ReleaseStringUTFChars(jstr, cstr);
        env->DeleteLocalRef(jstr);
    }
    for (int i = 0; i < argc; ++i) {
        argv[i] = args[i].data();
    }
    argv[argc] = nullptr;

// 调用 C 入口
    int ret = ffmpeg_main(argc, argv.data());
    return ret;
}



