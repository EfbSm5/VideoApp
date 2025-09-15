package com.example.videoapp.utils

import android.content.ContentValues
import android.content.Context
import android.net.Uri
import android.os.Build
import android.os.Environment
import android.provider.MediaStore
import android.provider.OpenableColumns
import android.webkit.MimeTypeMap
import androidx.annotation.WorkerThread
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream

object FileResolver {
    @WorkerThread
    fun saveFileToDownload(
        context: Context,
        fileName: String,
        mimeType: String,
        subDir: String? = null,
        data: ByteArray
    ): Uri? {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            val values = ContentValues().apply {
                put(MediaStore.Downloads.DISPLAY_NAME, fileName)
                put(MediaStore.Downloads.MIME_TYPE, mimeType)
                val relativePath = if (subDir.isNullOrBlank()) {
                    Environment.DIRECTORY_DOWNLOADS + "/"
                } else {
                    Environment.DIRECTORY_DOWNLOADS + "/${subDir.trimEnd('/')}/"
                }
                put(MediaStore.Downloads.RELATIVE_PATH, relativePath)
                // 可选: 标记写入中
                put(MediaStore.Downloads.IS_PENDING, 1)
            }

            val resolver = context.contentResolver
            val collection = MediaStore.Downloads.EXTERNAL_CONTENT_URI
            val itemUri = resolver.insert(collection, values) ?: return null

            try {
                resolver.openOutputStream(itemUri, "w").use { os ->
                    if (os == null) throw IllegalStateException("Cannot open output stream")
                    os.write(data)
                    os.flush()
                }

                // 更新为非 pending
                values.clear()
                values.put(MediaStore.Downloads.IS_PENDING, 0)
                resolver.update(itemUri, values, null, null)

                itemUri
            } catch (e: Exception) {
                // 失败时删除已插入的占位
                resolver.delete(itemUri, null, null)
                e.printStackTrace()
                null
            }
        } else {
            // Android 9 及以下：写入公共目录（需要在清单声明 WRITE_EXTERNAL_STORAGE，且在运行时申请权限）
            @Suppress("DEPRECATION") val downloadsDir =
                Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)
            val targetDir = if (subDir.isNullOrBlank()) {
                downloadsDir
            } else {
                File(downloadsDir, subDir).apply { if (!exists()) mkdirs() }
            }
            if (!targetDir.exists() && !targetDir.mkdirs()) return null

            val outFile = File(targetDir, fileName)
            try {
                FileOutputStream(outFile).use { fos ->
                    fos.write(data)
                    fos.flush()
                }
                Uri.fromFile(outFile)
            } catch (e: Exception) {
                e.printStackTrace()
                null
            }
        }
    }

    private fun getTempFile(context: Context): File {
        val tempOut = File(context.cacheDir, "ffmpeg_out").apply { if (!exists()) mkdirs() }
        val outputTempFile = File(tempOut, "out_${System.currentTimeMillis()}.mp4")
        val path = outputTempFile.absolutePath
        return outputTempFile
    }

    private fun saveFileToDownloads(
        context: Context, src: File, displayName: String, mime: String
    ): Uri? {
        val resolver = context.contentResolver
        val values = ContentValues().apply {
            put(MediaStore.Downloads.DISPLAY_NAME, displayName)
            put(MediaStore.Downloads.MIME_TYPE, mime)
            put(MediaStore.Downloads.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS + "/MyApp")
            put(MediaStore.Downloads.IS_PENDING, 1)
        }
        val uri = resolver.insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, values) ?: return null
        try {
            resolver.openOutputStream(uri)?.use { out ->
                FileInputStream(src).use { inF ->
                    inF.copyTo(out, 16 * 1024)
                }
            }
            values.clear()
            values.put(MediaStore.Downloads.IS_PENDING, 0)
            resolver.update(uri, values, null, null)
        } catch (e: Exception) {
            e.printStackTrace()
            resolver.delete(uri, null, null)
            return null
        }
        return uri
    }

    fun downloadUri(context: Context) = saveFileToDownloads(
        context,
        getTempFile(context),
        "processed_${System.currentTimeMillis()}.mp4",
        "video/mp4"
    )

    fun copyUriToCacheFile(
        context: Context, uri: Uri, subDir: String = "files", keepOriginalName: Boolean = true
    ): File? {
        try {
            val (displayName, _) = queryDisplayNameAndSize(context, uri)
            val fileName = if (keepOriginalName && !displayName.isNullOrBlank()) {
                displayName
            } else {
                val mime = context.contentResolver.getType(uri)
                val ext = MimeTypeMap.getSingleton().getExtensionFromMimeType(mime) ?: "dat"
                "tmp_${System.currentTimeMillis()}.$ext"
            }

            val targetDir = File(context.cacheDir, subDir).apply { if (!exists()) mkdirs() }
            val outFile = File(targetDir, fileName)

            context.contentResolver.openInputStream(uri)?.use { input ->
                FileOutputStream(outFile).use { output ->
                    val buffer = ByteArray(DEFAULT_BUFFER_SIZE) // 8K
                    var read: Int
                    while (input.read(buffer).also { read = it } != -1) {
                        output.write(buffer, 0, read)
                    }
                    output.flush()
                }
            } ?: return null

            return outFile
        } catch (e: Exception) {
            e.printStackTrace()
        }
        return null
    }

    private fun queryDisplayNameAndSize(context: Context, uri: Uri): Pair<String?, Long?> {
        var name: String? = null
        var size: Long? = null
        context.contentResolver.query(
            uri, arrayOf(
                OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE
            ), null, null, null
        )?.use { c ->
            if (c.moveToFirst()) {
                val nameIdx = c.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                val sizeIdx = c.getColumnIndex(OpenableColumns.SIZE)
                if (nameIdx != -1) name = c.getString(nameIdx)
                if (sizeIdx != -1) size = c.getLong(sizeIdx)
            }
        }
        return name to size
    }

}