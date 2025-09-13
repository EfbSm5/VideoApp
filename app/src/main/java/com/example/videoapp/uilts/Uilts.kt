package com.example.videoapp.uilts

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


    external fun run(cmd: Array<String>): Int
    external fun abort()
    external fun setLogCallback(cb: LogCallback?)
    external fun ffmpegVersion(): String

    @JvmStatic
    fun onNativeLog(level: Int, line: String) {
        // 如果没设置实例回调，可在这里打印
        android.util.Log.d("FFmpeg", "[$level] $line")
    }

    interface LogCallback {
        fun onLog(level: Int, line: String)
    }

    fun use(cmd: Array<String>) {
        Thread {
            val code = FFmpeg.run(cmd)
            Log.d("FFmpeg", "exit code=$code")
        }.start()
    }

    fun cancel() {
        FFmpeg.abort()
    }
}

