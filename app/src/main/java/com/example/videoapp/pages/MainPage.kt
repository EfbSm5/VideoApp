package com.example.videoapp.pages

import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable

@Composable
fun MainPage(start: () -> Unit) {
    Button(onClick = { start() }) { Text("start") }
}