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
#include <cstdio>
#include <string>
#include <vector>
#include <mutex>

extern "C" {
#include "libavformat/avformat.h"
}

// 声明可选(弱)符号：若未链接 CLI 源文件，则为 null
extern "C" {
__attribute__((weak)) int ffmpeg_main(int argc, char **argv);
__attribute__((weak)) void ffmpeg_request_abort(void);
__attribute__((weak)) void ffmpeg_set_log_callback(void (*cb)(int level, const char *msg));
}

static JavaVM *g_vm = nullptr;
static jclass g_FFmpegClass = nullptr; // global ref
static jmethodID g_onNativeLog = nullptr; // static void onNativeLog(int, String)
static std::mutex g_cbMutex;
static jobject g_logCb = nullptr; // global ref to FFmpeg.LogCallback
static jmethodID g_logCb_onLog = nullptr; // void onLog(int, String)

static void c_log_callback(int level, const char *line) {
    JNIEnv *env = nullptr;
    if (g_vm && g_vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
    }
    if (!env) return;
    jstring jline = env->NewStringUTF(line ? line : "");
    {
        std::lock_guard<std::mutex> _lk(g_cbMutex);
        if (g_logCb && g_logCb_onLog) {
            env->CallVoidMethod(g_logCb, g_logCb_onLog, (jint) level, jline);
        } else if (g_FFmpegClass && g_onNativeLog) {
            env->CallStaticVoidMethod(g_FFmpegClass, g_onNativeLog, (jint) level, jline);
        }
    }
    env->DeleteLocalRef(jline);
}

extern "C"
jint JNI_OnLoad(JavaVM *vm, void *) {
    g_vm = vm;
    JNIEnv *env = nullptr;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) return JNI_VERSION_1_6;

    jclass local = env->FindClass("com/example/videoapp/uilts/FFmpeg");
    if (local) {
        g_FFmpegClass = (jclass) env->NewGlobalRef(local);
        env->DeleteLocalRef(local);
        g_onNativeLog = env->GetStaticMethodID(g_FFmpegClass, "onNativeLog", "(ILjava/lang/String;)V");
    }
    if (ffmpeg_set_log_callback) {
        ffmpeg_set_log_callback(c_log_callback);
    }
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_videoapp_uilts_FFmpeg_ffmpegVersion(JNIEnv *env, jobject /*thiz*/) {
    unsigned ver = avformat_version();
    char buf[64];
    std::snprintf(buf, sizeof(buf), "avformat_version=%u", ver);
    return env->NewStringUTF(buf);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_videoapp_uilts_FFmpeg_abort(JNIEnv *, jobject) {
    if (ffmpeg_request_abort) ffmpeg_request_abort();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_videoapp_uilts_FFmpeg_setLogCallback(JNIEnv *env, jobject /*thiz*/, jobject cb) {
    std::lock_guard<std::mutex> _lk(g_cbMutex);
    if (g_logCb) {
        env->DeleteGlobalRef(g_logCb);
        g_logCb = nullptr;
        g_logCb_onLog = nullptr;
    }
    if (cb) {
        jclass cls = env->GetObjectClass(cb);
        g_logCb_onLog = env->GetMethodID(cls, "onLog", "(ILjava/lang/String;)V");
        env->DeleteLocalRef(cls);
        g_logCb = env->NewGlobalRef(cb);
    }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_videoapp_uilts_FFmpeg_run(JNIEnv *env, jobject /*thiz*/, jobjectArray cmdArray) {
    if (!cmdArray) return -1;
    const jint argc = env->GetArrayLength(cmdArray);
    if (!ffmpeg_main) {
        // CLI 未打包，返回特定错误码
        return -2;
    }
    std::vector<char*> argv;
    argv.reserve(argc + 1);
    std::vector<std::string> argsStr;
    argsStr.reserve(argc);
    for (jint i = 0; i < argc; ++i) {
        jstring jstr = (jstring) env->GetObjectArrayElement(cmdArray, i);
        const char *cstr = jstr ? env->GetStringUTFChars(jstr, nullptr) : "";
        argsStr.emplace_back(cstr);
        if (jstr) env->ReleaseStringUTFChars(jstr, cstr);
        if (jstr) env->DeleteLocalRef(jstr);
    }
    for (jint i = 0; i < argc; ++i) argv.push_back(const_cast<char*>(argsStr[i].c_str()));
    argv.push_back(nullptr);
    return ffmpeg_main(argc, argv.data());
}
