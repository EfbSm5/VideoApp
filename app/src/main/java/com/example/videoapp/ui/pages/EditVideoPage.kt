package com.example.videoapp.ui.pages

import android.app.Activity
import android.content.Intent
import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.videoapp.ui.components.VideoPlayer
import com.example.videoapp.utils.FFmpeg
import com.example.videoapp.utils.GetPermission
import com.example.videoapp.utils.showMsg
import com.example.videoapp.viewmodel.VideoResolveViewModel
import com.google.accompanist.permissions.ExperimentalPermissionsApi

@OptIn(ExperimentalPermissionsApi::class)
@Composable
fun EditVideoPage() {
    val viewModel: VideoResolveViewModel = viewModel()
    val state by viewModel.state.collectAsState()
    val selectedVideoUri by viewModel.selectedUri.collectAsState()
    val showLoading by viewModel.showLoading.collectAsState()
    GetPermission(
        onGranted = { viewModel.changeState(Screen.GetVideo) },
        onDenied = { showMsg("no permisson") })
    EditVideoSurface(
        state = state,
        selectedVideoUri = selectedVideoUri,
        onSelectedVideo = { viewModel.changeUri(it) },
        onChangeState = viewModel::changeState,
        showLoading = showLoading,
        ifShowLoading = viewModel::changeShowLoading,
        toGif = viewModel::toGif,
        clipVideo = viewModel::clip
    )

}

@Composable
fun EditVideoSurface(
    state: Screen,
    selectedVideoUri: Uri?,
    showLoading: Boolean,
    onSelectedVideo: (Uri?) -> Unit,
    onChangeState: (Screen) -> Unit,
    ifShowLoading: (Boolean) -> Unit,
    toGif: () -> Unit,
    clipVideo: () -> Unit
) {
    when (state) {
        Screen.EditVideo -> {
            EditVideo(
                selectedVideoUri = selectedVideoUri,
                showLoading = showLoading,
                toGif = toGif,
                clipVideo = clipVideo
            )
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
fun EditVideo(
    selectedVideoUri: Uri?,
    showLoading: Boolean,
    toGif: () -> Unit,
    clipVideo: () -> Unit
) {
    val scope = rememberCoroutineScope()
    Box(modifier = Modifier.fillMaxSize()) {
        if (showLoading) CircularProgressIndicator()
        Column(
            verticalArrangement = Arrangement.SpaceEvenly,
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            if (selectedVideoUri != null) {
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .weight(3f)
                ) {
                    VideoPlayer(uri = selectedVideoUri, modifier = Modifier.fillMaxSize())
                }
            }
            if (selectedVideoUri != null) Row(modifier = Modifier.weight(1f)) {
                Button(onClick = {
                    clipVideo()
                }) { Text("剪辑") }
                Button(onClick = {

                }) { Text("替换BGM") }
                Button(onClick = {
                    toGif()
                }) { Text("输出为GIF") }
                Button(onClick = {

                }) { Text("分享") }
            }
            Text(FFmpeg.version())

        }
    }

}

@Composable
fun OnChooseVideo(callback: (Uri?) -> Unit) {
    val launcher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.StartActivityForResult()
    ) { result ->
        if (result.resultCode == Activity.RESULT_OK) {
            callback(result.data?.data)
        }
    }
    LaunchedEffect(Unit) {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "video/*"
        }
        launcher.launch(intent)
    }
    Text("no video")
}

sealed interface Screen {
    data object PermissionDenied : Screen
    data object GetVideo : Screen
    data object EditVideo : Screen
}