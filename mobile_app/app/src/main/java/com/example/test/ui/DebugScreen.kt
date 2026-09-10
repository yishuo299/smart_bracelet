package com.example.test.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.clickable
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlin.random.Random
import com.example.test.data.HealthViewModel
import com.example.test.ui.theme.AlarmRed
import com.example.test.ui.theme.CardSurface
import com.example.test.ui.theme.CyanPrimary
import com.example.test.ui.theme.TextDim
import com.example.test.ui.theme.TextPrimary
import com.example.test.ui.theme.TextSecondary

@Composable
fun DebugScreen(
    viewModel: HealthViewModel
) {
    val debugLog = viewModel.debugLog.collectAsState()
    val sendLog = viewModel.sendLog.collectAsState()
    val connectionState = viewModel.connectionState.collectAsState()
    val connectedAddress = viewModel.connectedAddress.collectAsState()
    val pairedDevices = viewModel.pairedDevices.collectAsState()
    val recvListState = rememberLazyListState()
    val sendListState = rememberLazyListState()
    
    LaunchedEffect(debugLog.value.size) {
        if (debugLog.value.isNotEmpty()) {
            recvListState.animateScrollToItem(debugLog.value.lastIndex)
        }
    }
    LaunchedEffect(sendLog.value.size) {
        if (sendLog.value.isNotEmpty()) {
            sendListState.animateScrollToItem(sendLog.value.lastIndex)
        }
    }
    
    // 进入调试页时自动刷新已配对设备
    LaunchedEffect(Unit) {
        viewModel.refreshPairedDevices()
    }
    
    Column(
        modifier = Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState())
            .padding(16.dp)
    ) {
        Text(
            text = "调试终端",
            style = MaterialTheme.typography.headlineLarge,
            color = TextPrimary,
            modifier = Modifier.padding(bottom = 8.dp)
        )
        
        // 已配对设备列表
        Text(
            text = "选择设备连接",
            style = MaterialTheme.typography.titleMedium,
            color = TextPrimary,
            modifier = Modifier.padding(bottom = 8.dp)
        )
        
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .heightIn(min = 60.dp, max = 200.dp)
                .padding(bottom = 12.dp),
            colors = CardDefaults.cardColors(containerColor = CardSurface),
            shape = RoundedCornerShape(12.dp)
        ) {
            if (pairedDevices.value.isEmpty()) {
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(16.dp),
                    contentAlignment = Alignment.Center
                ) {
                    Column(horizontalAlignment = Alignment.CenterHorizontally) {
                        Text(
                            text = "点击「刷新设备」获取已配对的蓝牙设备",
                            style = MaterialTheme.typography.bodySmall,
                            color = TextSecondary
                        )
                        Text(
                            text = "若仍无设备：检查是否已授予蓝牙权限、蓝牙已开启、手环已配对",
                            style = MaterialTheme.typography.bodySmall,
                            color = TextDim,
                            modifier = Modifier.padding(top = 4.dp),
                            fontSize = 11.sp
                        )
                    }
                }
            } else {
                LazyColumn(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(12.dp),
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    items(pairedDevices.value) { (name, address) ->
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .clickable(enabled = connectionState.value != HealthViewModel.ConnectionState.CONNECTING) {
                                    viewModel.connect(address)
                                }
                                .background(
                                    if (connectedAddress.value == address) CyanPrimary.copy(alpha = 0.2f)
                                    else CardSurface,
                                    RoundedCornerShape(8.dp)
                                )
                                .padding(12.dp),
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.SpaceBetween
                        ) {
                            Column {
                                Text(
                                    text = name,
                                    style = MaterialTheme.typography.bodyMedium,
                                    color = TextPrimary
                                )
                                Text(
                                    text = address,
                                    style = MaterialTheme.typography.bodySmall,
                                    color = TextDim,
                                    fontSize = 10.sp
                                )
                            }
                            if (connectedAddress.value == address) {
                                Text(
                                    text = "已连接",
                                    style = MaterialTheme.typography.labelSmall,
                                    color = CyanPrimary
                                )
                            } else if (connectionState.value == HealthViewModel.ConnectionState.CONNECTING) {
                                Text(
                                    text = "连接中...",
                                    style = MaterialTheme.typography.labelSmall,
                                    color = TextSecondary
                                )
                            }
                        }
                    }
                }
            }
        }
        
        // Connection status
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = 12.dp),
            colors = CardDefaults.cardColors(containerColor = CardSurface),
            shape = RoundedCornerShape(12.dp)
        ) {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(16.dp),
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Column {
                    Text(
                        text = "蓝牙状态",
                        style = MaterialTheme.typography.labelSmall,
                        color = TextSecondary
                    )
                    Text(
                        text = when (connectionState.value) {
                            HealthViewModel.ConnectionState.DISCONNECTED -> "未连接"
                            HealthViewModel.ConnectionState.CONNECTING -> "连接中..."
                            HealthViewModel.ConnectionState.CONNECTED -> "已连接"
                        },
                        style = MaterialTheme.typography.bodyMedium,
                        color = when (connectionState.value) {
                            HealthViewModel.ConnectionState.DISCONNECTED -> TextDim
                            HealthViewModel.ConnectionState.CONNECTING -> CyanPrimary
                            HealthViewModel.ConnectionState.CONNECTED -> CyanPrimary
                        }
                    )
                    if (connectedAddress.value != null) {
                        Text(
                            text = connectedAddress.value!!,
                            style = MaterialTheme.typography.bodySmall,
                            color = TextDim,
                            fontSize = 10.sp
                        )
                    }
                }
                
                if (connectionState.value == HealthViewModel.ConnectionState.CONNECTED) {
                    Button(
                        onClick = { viewModel.disconnect() },
                        colors = ButtonDefaults.buttonColors(containerColor = AlarmRed)
                    ) {
                        Text("断开", color = TextPrimary)
                    }
                }
            }
        }
        
        // 接收 / 发送 双板块
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = 12.dp),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            // 接收板块
            Column(modifier = Modifier.weight(1f)) {
                Text(
                    text = "查看接收",
                    style = MaterialTheme.typography.titleMedium,
                    color = TextPrimary,
                    modifier = Modifier.padding(bottom = 8.dp)
                )
                Card(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(160.dp),
                    colors = CardDefaults.cardColors(containerColor = CardSurface),
                    shape = RoundedCornerShape(12.dp)
                ) {
                    if (debugLog.value.isEmpty()) {
                        Box(
                            modifier = Modifier
                                .fillMaxSize()
                                .padding(12.dp),
                            contentAlignment = Alignment.Center
                        ) {
                            Text(
                                text = when (connectionState.value) {
                                    HealthViewModel.ConnectionState.DISCONNECTED -> "未连接"
                                    HealthViewModel.ConnectionState.CONNECTING -> "连接中..."
                                    HealthViewModel.ConnectionState.CONNECTED -> "等待接收..."
                                },
                                style = MaterialTheme.typography.bodySmall,
                                color = TextSecondary,
                                fontSize = 10.sp
                            )
                        }
                    } else {
                        LazyColumn(
                            modifier = Modifier
                                .fillMaxSize()
                                .padding(12.dp),
                            state = recvListState,
                            verticalArrangement = Arrangement.spacedBy(4.dp)
                        ) {
                            items(debugLog.value) { line ->
                                Text(
                                    text = line,
                                    style = MaterialTheme.typography.bodySmall,
                                    fontFamily = FontFamily.Monospace,
                                    color = TextDim,
                                    fontSize = 9.sp
                                )
                            }
                        }
                    }
                }
            }
            
            // 发送板块
            Column(modifier = Modifier.weight(1f)) {
                Text(
                    text = "查看发送",
                    style = MaterialTheme.typography.titleMedium,
                    color = TextPrimary,
                    modifier = Modifier.padding(bottom = 8.dp)
                )
                Card(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(160.dp),
                    colors = CardDefaults.cardColors(containerColor = CardSurface),
                    shape = RoundedCornerShape(12.dp)
                ) {
                    if (sendLog.value.isEmpty()) {
                        Box(
                            modifier = Modifier
                                .fillMaxSize()
                                .padding(12.dp),
                            contentAlignment = Alignment.Center
                        ) {
                            Text(
                                text = "等待发送...",
                                style = MaterialTheme.typography.bodySmall,
                                color = TextSecondary,
                                fontSize = 10.sp
                            )
                        }
                    } else {
                        LazyColumn(
                            modifier = Modifier
                                .fillMaxSize()
                                .padding(12.dp),
                            state = sendListState,
                            verticalArrangement = Arrangement.spacedBy(4.dp)
                        ) {
                            items(sendLog.value) { line ->
                                Text(
                                    text = line,
                                    style = MaterialTheme.typography.bodySmall,
                                    fontFamily = FontFamily.Monospace,
                                    color = CyanPrimary.copy(alpha = 0.9f),
                                    fontSize = 9.sp
                                )
                            }
                        }
                    }
                }
            }
        }
        
        // Control buttons
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = 12.dp),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            OutlinedButton(
                onClick = { viewModel.clearDebugLog() },
                modifier = Modifier.weight(1f)
            ) {
                Text("清空接收", color = TextPrimary)
            }
            OutlinedButton(
                onClick = { viewModel.clearSendLog() },
                modifier = Modifier.weight(1f)
            ) {
                Text("清空发送", color = TextPrimary)
            }
            Button(
                onClick = { viewModel.refreshPairedDevices() },
                modifier = Modifier.weight(1f),
                colors = ButtonDefaults.buttonColors(containerColor = CyanPrimary)
            ) {
                Text("刷新设备", color = TextPrimary)
            }
        }
        
        // Test data input
        Text(
            text = "测试数据输入",
            style = MaterialTheme.typography.titleMedium,
            color = TextPrimary,
            modifier = Modifier.padding(bottom = 8.dp)
        )
        
        Row(
            modifier = Modifier
                .fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Button(
                onClick = { viewModel.updateHeartRate(Random.nextInt(60, 101).toFloat()) },
                modifier = Modifier.weight(1f),
                colors = ButtonDefaults.buttonColors(containerColor = CardSurface)
            ) {
                Text("心率", color = TextPrimary, fontSize = 12.sp)
            }
            Button(
                onClick = { viewModel.updateSpo2(Random.nextInt(95, 101).toFloat()) },
                modifier = Modifier.weight(1f),
                colors = ButtonDefaults.buttonColors(containerColor = CardSurface)
            ) {
                Text("血氧", color = TextPrimary, fontSize = 12.sp)
            }
            Button(
                onClick = { viewModel.updateTemperature(Random.nextDouble(36.0, 37.5).toFloat()) },
                modifier = Modifier.weight(1f),
                colors = ButtonDefaults.buttonColors(containerColor = CardSurface)
            ) {
                Text("体温", color = TextPrimary, fontSize = 12.sp)
            }
            Button(
                onClick = { viewModel.updateEcg(Random.nextInt(0, 4096).toFloat()) },
                modifier = Modifier.weight(1f),
                colors = ButtonDefaults.buttonColors(containerColor = CardSurface)
            ) {
                Text("心电", color = TextPrimary, fontSize = 12.sp)
            }
        }
    }
}
