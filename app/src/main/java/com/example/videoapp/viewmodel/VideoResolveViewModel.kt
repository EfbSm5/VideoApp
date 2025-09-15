package com.example.videoapp.viewmodel

import android.net.Uri
import androidx.lifecycle.ViewModel
import com.example.videoapp.ui.pages.Screen
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow

class VideoResolveViewModel : ViewModel() {
    private var _state = MutableStateFlow<Screen>(Screen.PermissionDenied)
    private var _selectedUri = MutableStateFlow<Uri?>(null)
    var state: StateFlow<Screen> = _state
    var selectedUri: StateFlow<Uri?> = _selectedUri
    fun changeState(state: Screen) {
        _state.value = state
    }

    fun changeUri(uri: Uri?) {
        _selectedUri.value = uri
    }

    fun runFFmpeg() {

    }
}
