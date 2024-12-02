package com.example.videoapp.pages

import androidx.compose.runtime.Composable
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController

@Composable
fun NavGrh() {
    val navControl = rememberNavController()
    NavHost(navController = navControl, startDestination = "StartPage") {
        composable("StartPage") {
            MainPage()
        }
        composable("EditPage") {
            EditPage()
        }
    }
}