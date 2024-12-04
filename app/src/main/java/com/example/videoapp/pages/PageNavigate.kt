package com.example.videoapp.pages

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material3.Button
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
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.example.videoapp.ui.theme.VideoAppTheme


@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun VideoApp() {
    var showMenu by remember { mutableStateOf(false) }
    VideoAppTheme {
        Scaffold(modifier = Modifier.fillMaxSize(), topBar = {
            Row(modifier = Modifier.fillMaxSize()) {
                IconButton(onClick = { showMenu = true }, content = { Icons.Default.Menu })
                TopAppBar(title = { Text("videos") })
            }
        }) { innerPadding ->
            Surface(modifier = Modifier.padding(innerPadding)) {
                Column {
                    Button(onClick = {}) { Text("开始剪视频") }
                    Button(onClick = {}) { Text("浏览视频") }
                }
            }
        }
    }
}

@Composable
fun NavGrh() {
    val navControl = rememberNavController()
    NavHost(navController = navControl, startDestination = "StartPage") {
        composable("StartPage") {
            EditVideoPage()
        }
        composable("EditPage") {
            EditPage()
        }
    }
}