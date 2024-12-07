package com.example.videoapp.pages

import android.net.Uri
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import com.example.videoapp.uilts.HandlePermission
import com.example.videoapp.uilts.OnChooseVideo
import com.example.videoapp.uilts.VideoPlayer

@Composable
fun EditVideoPage() {
    var state by remember { mutableStateOf<Screen>(Screen.PermissionDenied) }
    var selectedVideoUri by remember { mutableStateOf<Uri?>(null) }
    EditVideoSurface(state = state,
        selectedVideoUri = selectedVideoUri,
        onSelectedVideo = { selectedVideoUri = it },
        onChangeState = { state = it })
}

@Composable
fun EditVideoSurface(
    state: Screen,
    selectedVideoUri: Uri?,
    onSelectedVideo: (Uri?) -> Unit,
    onChangeState: (Screen) -> Unit
) {
    when (state) {
        Screen.EditVideo -> {
            EditVideoSurface(selectedVideoUri = selectedVideoUri)
        }

        Screen.GetVideo -> {
            OnChooseVideo {
                onSelectedVideo(it)
                onChangeState(Screen.EditVideo)
            }
        }

        Screen.PermissionDenied -> {
            HandlePermission { onChangeState(Screen.GetVideo) }
            Text("no permission")
        }
    }
}


@Composable
fun EditVideoSurface(selectedVideoUri: Uri?) {
    Column(
        modifier = Modifier.fillMaxSize()
    ) {
        if (selectedVideoUri != null) {
            Box() {
                VideoPlayer(uri = selectedVideoUri)
            }
        } else {
            Text("no video")
        }
        Row {
            Button(onClick = {

            }) { Text("剪辑") }
            Button(onClick = {

            }) { Text("替换BGM") }
            Button(onClick = {

            }) { Text("输出为GIF") }
            Button(onClick = {

            }) { Text("分享") }
        }
    }
}

sealed interface Screen {
    data object PermissionDenied : Screen
    data object GetVideo : Screen
    data object EditVideo : Screen
}