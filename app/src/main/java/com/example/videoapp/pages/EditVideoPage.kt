package com.example.videoapp.pages

import android.net.Uri
import android.widget.Toast
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import com.example.videoapp.GetPermission
import com.example.videoapp.uilts.FFmpeg
import com.example.videoapp.uilts.OnChooseVideo
import com.example.videoapp.uilts.VideoPlayer
import com.google.accompanist.permissions.ExperimentalPermissionsApi

@OptIn(ExperimentalPermissionsApi::class)
@Composable
fun EditVideoPage() {
    var state by remember { mutableStateOf<Screen>(Screen.PermissionDenied) }
    val context = LocalContext.current
    GetPermission(
        onGranted = { state = Screen.GetVideo },
        onDenied = { Toast.makeText(context, "no permisson", Toast.LENGTH_LONG).show() }
    )
    var selectedVideoUri by remember { mutableStateOf<Uri?>(null) }
    EditVideoSurface(
        state = state,
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
            Text("no permission")
        }
    }
}


@Composable
fun EditVideoSurface(selectedVideoUri: Uri?) {
    Column {
        if (selectedVideoUri != null) {
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .weight(3f)
            ) {
                VideoPlayer(uri = selectedVideoUri)
            }
        }
        Row(modifier = Modifier.weight(1f)) {
            Button(onClick = {

            }) { Text("剪辑") }
            Button(onClick = {

            }) { Text("替换BGM") }
            Button(onClick = {

            }) { Text("输出为GIF") }
            Button(onClick = {

            }) { Text("分享") }
        }
        Text(FFmpeg.ffmpegVersion())

    }
}

sealed interface Screen {
    data object PermissionDenied : Screen
    data object GetVideo : Screen
    data object EditVideo : Screen
}