package com.example.videoapp.uilts

import android.app.Activity
import android.content.Intent
import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import com.google.accompanist.permissions.ExperimentalPermissionsApi
import com.google.accompanist.permissions.isGranted
import com.google.accompanist.permissions.rememberPermissionState


@Composable
fun OnChooseVideo(callback: (Uri?) -> Unit) {
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