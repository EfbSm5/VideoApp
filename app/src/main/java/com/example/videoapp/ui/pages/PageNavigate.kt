package com.example.videoapp.ui.pages

import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Scaffold
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
import com.example.videoapp.ui.components.Menu


@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun VideoApp() {
    var showMenu by remember { mutableStateOf(false) }
    val navControl = rememberNavController()

    Scaffold(modifier = Modifier.fillMaxSize(), topBar = {
        TopAppBar(title = { Text(text = "这是一个剪视频APP") }, navigationIcon = {
            IconButton(onClick = { showMenu = true }) {
                Icon(Icons.Default.Menu, contentDescription = "Menu")
            }
            Menu(showMenu = showMenu) { showMenu = false }
        })
    }) { innerPadding ->
        NavHost(
            modifier = Modifier
                .padding(innerPadding)
                .fillMaxSize(),
            navController = navControl,
            startDestination = "StartPage"
        ) {
            composable("StartPage") {
                MainPage {
                    navControl.navigate("EditPage")
                }
            }
            composable("EditPage") {
                EditVideoPage()
            }
        }
    }
}