#pragma once

#include <mutex>

extern "C" {
#include <libavutil/error.h>
#include <libavutil/log.h>
}

#ifdef __ANDROID__

#include <android/log.h>

#define FF_LOG_TAG "FFmpegGif"
#define FF_ANDROID_LOG(prio, fmt, ...) __android_log_print(prio, FF_LOG_TAG, fmt, ##__VA_ARGS__)
#else
#include <cstdio>
#define FF_ANDROID_LOG(prio, fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#endif

void ff_init_logger(int level = AV_LOG_DEBUG);

void ff_store_last_error(int err, const char *where);

const char *ff_get_last_error();

