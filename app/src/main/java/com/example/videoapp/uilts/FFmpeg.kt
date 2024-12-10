package com.example.videoapp.uilts

import android.net.Uri
import java.util.ArrayList

class FFmpeg() {
    external fun ffmpegVersion(): String;
    external fun ffmpegCmd(cmd: Array<String>)

    companion object {
        init {
            System.loadLibrary("native-lib")
        }
    }
}

