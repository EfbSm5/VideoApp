package com.example.videoapp.viewmodel

import android.net.Uri
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.videoapp.ui.pages.Screen
import com.example.videoapp.utils.FFmpeg
import com.example.videoapp.utils.showMsg
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch

class VideoResolveViewModel : ViewModel() {
    private var _state = MutableStateFlow<Screen>(Screen.PermissionDenied)
    private var _selectedUri = MutableStateFlow<Uri?>(null)
    private var _showLoading = MutableStateFlow<Boolean>(false)
    var state: StateFlow<Screen> = _state
    var selectedUri: StateFlow<Uri?> = _selectedUri
    var showLoading: StateFlow<Boolean> = _showLoading
    fun changeState(state: Screen) {
        _state.value = state
    }

    fun changeUri(uri: Uri?) {
        _selectedUri.value = uri
    }

    fun changeShowLoading(boolean: Boolean) {
        _showLoading.value = boolean
    }

    fun toGif() {
        viewModelScope.launch {
            _showLoading.value = true
            val result = FFmpeg.toGif(
                input = _selectedUri.value, startMs = 0, durationMs = 2000, fps = 10, maxWidth = 480
            )
            _showLoading.value = false
            when (result) {
                is FFmpeg.VideoResult.Failure -> showMsg("失败: ${result.reason}")
                is FFmpeg.VideoResult.Success -> showMsg("输出: ${result.outputPath}")
            }
        }
    }

    fun clip() {
        viewModelScope.launch {
            _showLoading.value = true
            val result = FFmpeg.clipVideo(
                inputUri = selectedUri.value, startMs = 0, durationMs = 0
            )
            _showLoading.value = false
            when (result) {
                is FFmpeg.VideoResult.Failure -> showMsg("失败: ${result.reason}")
                is FFmpeg.VideoResult.Success -> showMsg("输出: ${result.outputPath}")
            }
        }
    }
}
