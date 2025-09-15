package com.example.videoapp.ui.components

import androidx.compose.foundation.layout.IntrinsicSize
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.wrapContentSize
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier

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