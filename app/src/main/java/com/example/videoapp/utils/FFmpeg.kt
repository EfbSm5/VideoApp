package com.example.videoapp.utils

import android.net.Uri
import android.util.Log
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ensureActive
import kotlinx.coroutines.withContext
import kotlin.coroutines.cancellation.CancellationException

private const val TAG = "FFmpeg"

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
        initLogger()
    }

    private external fun videoToGif(
        inputPath: String,
        outputPath: String,
        maxWidth: Int,
        fps: Int,
        startMs: Long,
        durationMs: Long,   // 0 表示直到结尾
        qualityPreset: Int  // 预留可映射不同过滤链
    ): Int   // 0 成功，非 0 失败

    private external fun clip(
        inputPath: String,
        outputPath: String,
        startMs: Long,
        durationMs: Long,
        reEncode: Boolean  // false=快速裁剪; true=精确(后期再实现)
    ): Int

    private external fun ffmpegConfig(): String
    private external fun getLastError(): String?
    private external fun ffmpegVersion(): String
    private external fun hasGifEncoder(): Boolean
    private external fun initLogger()

    fun info(): String {
        try {
            val r = ffmpegConfig()
            Log.d(TAG, "info: $r")
            return r
        } catch (e: Exception) {
            throw e
        }
    }

    fun version(): String {
        try {
            val r = ffmpegVersion()
            Log.d(TAG, "version: $r")
            return r
        } catch (e: Exception) {
            throw e
        }
    }

    fun canGif(): Boolean {
        try {
            val r = hasGifEncoder()
            Log.d(TAG, "canGif: $r")
            return r
        } catch (e: Exception) {
            throw e
        }
    }


    sealed class VideoResult {
        data class Success(val outputPath: String) : VideoResult()
        data class Failure(
            val reason: String, val code: Int? = null, val throwable: Throwable? = null
        ) : VideoResult()
    }

    suspend fun toGif(
        input: Uri?,
        maxWidth: Int = 0,
        fps: Int = 0,
        startMs: Long,
        durationMs: Long,
        dispatcherIO: CoroutineDispatcher = Dispatchers.IO,
        dispatcherCPU: CoroutineDispatcher = Dispatchers.Default
    ): VideoResult = withContext(dispatcherIO) {
        if (null == input) return@withContext VideoResult.Failure("no-data")
        try {
            val ctx = AppUtil.getContext()
            val inputPath = FileResolver.copyUriToCacheFile(ctx, input)
                ?: return@withContext VideoResult.Failure("input-error:noResponse")
            ensureActive()
            val outputPath = FileResolver.createOutputAbsolutePath(ctx, ".mp4")
            val resultCode = withContext(dispatcherCPU) {
                ensureActive()
                videoToGif(
                    inputPath = inputPath,
                    outputPath = outputPath,
                    startMs = startMs,
                    durationMs = durationMs,
                    maxWidth = maxWidth,
                    fps = fps,
                    qualityPreset = 0,
                )
            }
            if (resultCode == 0) {
                VideoResult.Success(outputPath)
            } else {
                VideoResult.Failure("clip-failed", code = resultCode)
            }
        } catch (ce: CancellationException) {
            throw ce
        } catch (t: Throwable) {
            VideoResult.Failure("exception:${t.message}", throwable = t)
        }
    }

    suspend fun clipVideo(
        inputUri: Uri?,
        startMs: Long,
        durationMs: Long,
        reEncode: Boolean = false,
        dispatcherIO: CoroutineDispatcher = Dispatchers.IO,
        dispatcherCPU: CoroutineDispatcher = Dispatchers.Default
    ): VideoResult = withContext(dispatcherIO) {
        if (null == inputUri) return@withContext VideoResult.Failure("no-data")
        try {
            val ctx = AppUtil.getContext()
            val inputPath = FileResolver.copyUriToCacheFile(ctx, inputUri)
                ?: return@withContext VideoResult.Failure("input-error:noResponse")
            ensureActive()
            val outputPath = FileResolver.createOutputAbsolutePath(ctx, ".mp4")
            val resultCode = withContext(dispatcherCPU) {
                ensureActive()
                clip(
                    inputPath = inputPath,
                    outputPath = outputPath,
                    startMs = startMs,
                    durationMs = durationMs,
                    reEncode = reEncode
                )
            }
            if (resultCode == 0) {
                VideoResult.Success(outputPath)
            } else {
                VideoResult.Failure("clip-failed", code = resultCode)
            }
        } catch (ce: CancellationException) {
            throw ce
        } catch (t: Throwable) {
            VideoResult.Failure("exception:${t.message}", throwable = t)
        }
    }
}
