package com.example.videoapp

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.IconButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import com.example.videoapp.pages.NavGrh
import com.example.videoapp.ui.theme.VideoAppTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            VideoApp()
        }
    }
}


@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun VideoApp() {
    var showMenu by remember { mutableStateOf<Boolean>(false) }
    VideoAppTheme {
        Scaffold(modifier = Modifier.fillMaxSize(), topBar = {
            IconButton(onClick = { showMenu = true }, content = { Icons.Default.Menu })
            TopAppBar(title = { Text("videos") })
        }) { innerPadding ->
            Surface(modifier = Modifier.padding(innerPadding)) {
                NavGrh()
            }
        }
    }
}