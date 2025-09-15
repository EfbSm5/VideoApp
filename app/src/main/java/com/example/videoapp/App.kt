package com.example.videoapp

import android.app.Application
import com.example.videoapp.utils.AppUtil

class VideoApp : Application() {
    override fun onCreate() {
        super.onCreate()
        AppUtil.init(this)
    }
}