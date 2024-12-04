package com.example.videoapp.pages

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.widget.Toast
import androidx.activity.compose.ManagedActivityResultLauncher
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.ActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.Button
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import com.example.videoapp.uilts.VideoPlayer
import com.google.accompanist.permissions.ExperimentalPermissionsApi
import com.google.accompanist.permissions.isGranted
import com.google.accompanist.permissions.rememberPermissionState

@Composable
fun EditVideoPage() {
    HandlePermission()
    var selectedVideoUri by remember { mutableStateOf<Uri?>(null) }
    onChooseVideo { selectedVideoUri = it }
    EditVideoSurface(selectedVideoUri)

}

@Composable
@OptIn(ExperimentalPermissionsApi::class)
fun HandlePermission() {
    val storagePermissionState = rememberPermissionState(android.Manifest.permission.CAMERA)
    LaunchedEffect(storagePermissionState.status) {
        if (!storagePermissionState.status.isGranted) {
            storagePermissionState.launchPermissionRequest()
        }
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

    }

}

