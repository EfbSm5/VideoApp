package com.example.videoapp.pages

import androidx.compose.foundation.layout.IntrinsicSize
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.wrapContentSize
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.example.videoapp.ui.theme.VideoAppTheme


@Composable
fun VideoApp() {
    var showMenu by remember { mutableStateOf(false) }
    VideoAppSurface(showMenu) { showMenu = false }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun VideoAppSurface(showMenu: Boolean, onMenu: (Boolean) -> Unit) {
    VideoAppTheme {
        Scaffold(modifier = Modifier.fillMaxSize(), topBar = {
            TopAppBar(title = { Text(text = "这是一个剪视频APP") }, navigationIcon = {
                IconButton(onClick = { onMenu(true) }) {
                    Icon(Icons.Default.Menu, contentDescription = "Menu")
                }
                Menu(showMenu = showMenu) { onMenu(false) }
            })
        }) { innerPadding ->
            Surface(
                modifier = Modifier.padding(innerPadding), color = MaterialTheme.colorScheme.surface
            ) {
                NavGrh()
            }
        }
    }
}

@Composable
fun NavGrh() {
    val navControl = rememberNavController()
    NavHost(navController = navControl, startDestination = "StartPage") {
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

@Composable
fun Menu(showMenu: Boolean, onCloseMenu: () -> Unit) {
    DropdownMenu(
        expanded = showMenu,
        onDismissRequest = { onCloseMenu() },
        modifier = Modifier
            .width(IntrinsicSize.Min)
            .wrapContentSize(Alignment.TopStart)
    ) {
        DropdownMenuItem(text = { Text(text = "test") }, onClick = {

        })
        DropdownMenuItem(text = { Text(text = "test") }, onClick = {

        })
    }
}