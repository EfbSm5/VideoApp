package com.example.videoapp.utils

import android.util.Log

object FFmpeg {
    init {
        System.loadLibrary("native-lib")
        System.loadLibrary("avutil")
        System.loadLibrary("swresample")
        System.loadLibrary("swscale")
        System.loadLibrary("avcodec")
        System.loadLibrary("avformat")
        System.loadLibrary("avfilter")
        System.loadLibrary("avdevice")
    }

    external fun ffmpegConfig(): String

    external fun ffmpegVersion(): String
    external fun hasGifEncoder(): Boolean

    @JvmStatic
    fun onNativeLog(level: Int, line: String) {
        Log.d("FFmpeg", "[$level] $line")
    }

    fun ffinfo() {
        Log.d("FFmpeg", ffmpegConfig())
    }

    fun ffversion() {
        Log.d("FFmpeg", ffmpegVersion())
    }

    fun ffGif() {
        Log.d("FFmpeg", hasGifEncoder().toString())

    }

}
