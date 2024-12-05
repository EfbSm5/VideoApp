package com.example.videoapp.pages

import android.app.Activity
import android.content.Intent
import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import com.example.videoapp.uilts.VideoPlayer
import com.google.accompanist.permissions.ExperimentalPermissionsApi
import com.google.accompanist.permissions.PermissionState
import com.google.accompanist.permissions.isGranted
import com.google.accompanist.permissions.rememberPermissionState

@OptIn(ExperimentalPermissionsApi::class)
@Composable
fun EditVideoPage() {
    var state by remember { mutableStateOf<Screen>(Screen.PermissionDenied) }
    var selectedVideoUri by remember { mutableStateOf<Uri?>(null) }
    HandlePermission { state = Screen.GetVideo }
    EditVideoSurface(state = state, selectedVideoUri = selectedVideoUri) { selectedVideoUri = it }
}

@Composable
fun EditVideoSurface(state: Screen, selectedVideoUri: Uri?, onSelectedVideo: (Uri?) -> Unit) {
    when (state) {
        Screen.EditVideo -> {

        }

        Screen.GetVideo -> {
            onChooseVideo { onSelectedVideo(it) }
        }

        Screen.PermissionDenied -> {
            Text("no permission")
        }
    }
}

@Composable
@OptIn(ExperimentalPermissionsApi::class)
fun HandlePermission(permissionGranted: () -> Unit) {
    val readPermissionState =
        rememberPermissionState(android.Manifest.permission.READ_EXTERNAL_STORAGE)
    val writePermissionState =
        rememberPermissionState(android.Manifest.permission.WRITE_EXTERNAL_STORAGE)
    LaunchedEffect(readPermissionState.status, writePermissionState.status) {
        if (!writePermissionState.status.isGranted) {
            writePermissionState.launchPermissionRequest()
        } else if (!readPermissionState.status.isGranted) {
            readPermissionState.launchPermissionRequest()
        } else permissionGranted()
    }
}

@Composable
fun onChooseVideo(callback: (Uri?) -> Unit) {
    val launcher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.StartActivityForResult()
    ) { result ->
        if (result.resultCode == Activity.RESULT_OK) {
            callback(result.data?.data)
        }
    }
    val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
        addCategory(Intent.CATEGORY_OPENABLE)
        type = "video/*"
    }
    launcher.launch(intent)
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
            Button(onClick = {}) { Text("剪辑") }
            Button(onClick = {}) { Text("替换BGM") }
            Button(onClick = {}) { Text("输出为GIF") }
            Button(onClick = {}) { Text("分享") }
        }
    }
}

sealed interface Screen {
    data object PermissionDenied : Screen
    data object GetVideo : Screen
    data object EditVideo : Screen
}