package com.example.videoapp.utils

import android.annotation.SuppressLint
import android.content.Context
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

fun showMsg(text: String) {
    Toast.makeText(AppUtil.getContext(), text, Toast.LENGTH_LONG).show()
}