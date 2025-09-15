package com.example.videoapp.utils

import android.annotation.SuppressLint
import android.content.Context
import android.os.Handler
import android.os.Looper.getMainLooper
import android.os.Looper.myLooper
import android.widget.Toast

@SuppressLint("StaticFieldLeak")
object AppUtil {
    private lateinit var context: Context
    fun getContext(): Context {
        return context
    }

    fun init(context: Context) {
        this.context = context
    }
}

fun showMsg(text: CharSequence, long: Boolean = false) {
    val ctx = AppUtil.getContext()
    if (myLooper() == getMainLooper()) {
        Toast.makeText(ctx, text, if (long) Toast.LENGTH_LONG else Toast.LENGTH_SHORT).show()
    } else {
        Handler(getMainLooper()).post {
            Toast.makeText(ctx, text, if (long) Toast.LENGTH_LONG else Toast.LENGTH_SHORT).show()
        }
    }
}