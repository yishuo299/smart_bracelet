package com.example.test.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ModalBottomSheet
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.example.test.R
import androidx.compose.ui.unit.sp
import android.content.Intent
import android.net.Uri
import android.provider.Settings
import com.example.test.data.HealthViewModel
import com.example.test.data.MetricType
import com.example.test.ui.theme.AlarmRed
import com.example.test.ui.theme.CardSurface
import com.example.test.ui.theme.EcgGreen
import com.example.test.ui.theme.HeartRed
import com.example.test.ui.theme.Spo2Blue
import com.example.test.ui.theme.TempAmber
import com.example.test.ui.theme.TextDim
import com.example.test.ui.theme.TextPrimary
import com.example.test.ui.theme.CardSurfaceAlt
import com.example.test.ui.theme.CyanPrimary
import com.example.test.ui.theme.TextSecondary

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun HomeScreen(
    viewModel: HealthViewModel,
    onMetricClick: (MetricType) -> Unit,
    onHistoryClick: () -> Unit = {}
) {
    val heartRateState = viewModel.heartRate.collectAsState()
    val spo2State = viewModel.spo2.collectAsState()
    val tempState = viewModel.temperature.collectAsState()
    val ecgState = viewModel.ecg.collectAsState()
    val alarmActive = viewModel.alarmActive.collectAsState()
    val connectionState = viewModel.connectionState.collectAsState()
    val connectedAddress = viewModel.connectedAddress.collectAsState()
    val pairedDevices = viewModel.pairedDevices.collectAsState()
    var showDeviceSheet by remember { mutableStateOf(false) }
    
    // 设备选择底部弹窗
    if (showDeviceSheet) {
        ModalBottomSheet(
            onDismissRequest = { showDeviceSheet = false },
            containerColor = CardSurface
        ) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .heightIn(max = 400.dp)
                    .padding(horizontal = 16.dp, vertical = 8.dp)
            ) {
                Text(
                    text = "选择设备连接",
                    style = MaterialTheme.typography.titleLarge,
                    color = TextPrimary,
                    modifier = Modifier.padding(bottom = 16.dp)
                )
                if (pairedDevices.value.isEmpty()) {
                    val ctx = LocalContext.current
                    Column(modifier = Modifier.padding(bottom = 16.dp)) {
                        Text(
                            text = "暂无已配对设备。请检查：",
                            style = MaterialTheme.typography.bodyMedium,
                            color = TextSecondary
                        )
                        Text(
                            text = "1. 是否已授予本应用「蓝牙」权限",
                            style = MaterialTheme.typography.bodySmall,
                            color = TextSecondary,
                            modifier = Modifier.padding(top = 4.dp)
                        )
                        Text(
                            text = "2. 手机蓝牙是否已开启",
                            style = MaterialTheme.typography.bodySmall,
                            color = TextSecondary,
                            modifier = Modifier.padding(top = 2.dp)
                        )
                        Text(
                            text = "3. 手环是否已在系统蓝牙设置中配对",
                            style = MaterialTheme.typography.bodySmall,
                            color = TextSecondary,
                            modifier = Modifier.padding(top = 2.dp)
                        )
                        Button(
                            onClick = {
                                ctx.startActivity(Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS).apply {
                                    data = Uri.fromParts("package", ctx.packageName, null)
                                })
                            },
                            modifier = Modifier.padding(top = 12.dp),
                            colors = ButtonDefaults.buttonColors(containerColor = CyanPrimary)
                        ) {
                            Text("去设置中授予权限", color = TextPrimary)
                        }
                    }
                } else {
                    LazyColumn(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                        items(pairedDevices.value) { (name, address) ->
                            Row(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .clickable(enabled = connectionState.value != HealthViewModel.ConnectionState.CONNECTING) {
                                        viewModel.connect(address)
                                        showDeviceSheet = false
                                    }
                                    .background(
                                        if (connectedAddress.value == address) CyanPrimary.copy(alpha = 0.2f)
                                        else CardSurfaceAlt,
                                        RoundedCornerShape(8.dp)
                                    )
                                    .padding(16.dp),
                                verticalAlignment = Alignment.CenterVertically,
                                horizontalArrangement = Arrangement.SpaceBetween
                            ) {
                                Column {
                                    Text(text = name, color = TextPrimary, style = MaterialTheme.typography.bodyLarge)
                                    Text(text = address, color = TextDim, fontSize = 10.sp)
                                }
                                if (connectedAddress.value == address) {
                                    Text("已连接", color = CyanPrimary, style = MaterialTheme.typography.labelMedium)
                                }
                            }
                        }
                    }
                }
                Spacer(modifier = Modifier.height(24.dp))
            }
        }
    }
    
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        Text(
            text = stringResource(R.string.app_name),
            style = MaterialTheme.typography.headlineLarge,
            color = TextPrimary,
            modifier = Modifier.padding(bottom = 8.dp)
        )
        
        Text(
            text = "实时健康数据",
            style = MaterialTheme.typography.bodyMedium,
            color = TextSecondary,
            modifier = Modifier.padding(bottom = 12.dp)
        )
        
        // 连接状态卡片
        ConnectionCard(
            connectionState = connectionState.value,
            connectedAddress = connectedAddress.value,
            onConnectClick = {
                viewModel.refreshPairedDevices()
                showDeviceSheet = true
            },
            onDisconnectClick = { viewModel.disconnect() }
        )
        Spacer(modifier = Modifier.height(12.dp))
        
        // 警报横幅：收到 STM32 的 Danger!!! 时显示
        if (alarmActive.value) {
            AlarmBanner(
                onDismiss = { viewModel.dismissAlarm() }
            )
            Spacer(modifier = Modifier.height(12.dp))
        }
        
        LazyVerticalGrid(
            columns = GridCells.Fixed(2),
            horizontalArrangement = Arrangement.spacedBy(12.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            item {
                MetricCard(
                    type = MetricType.HEART_RATE,
                    value = heartRateState.value.current,
                    accentColor = HeartRed,
                    onClick = { onMetricClick(MetricType.HEART_RATE) }
                )
            }
            item {
                MetricCard(
                    type = MetricType.SPO2,
                    value = spo2State.value.current,
                    accentColor = Spo2Blue,
                    onClick = { onMetricClick(MetricType.SPO2) }
                )
            }
            item {
                MetricCard(
                    type = MetricType.TEMPERATURE,
                    value = tempState.value.current,
                    accentColor = TempAmber,
                    onClick = { onMetricClick(MetricType.TEMPERATURE) }
                )
            }
            item {
                MetricCard(
                    type = MetricType.ECG,
                    value = ecgState.value.current,
                    accentColor = EcgGreen,
                    onClick = { onMetricClick(MetricType.ECG) }
                )
            }
        }
        
        Spacer(modifier = Modifier.height(12.dp))
        
        // 历史数据入口（放在模块下面）
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .clickable(onClick = onHistoryClick),
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
                Text(text = "📋 查看历史数据", color = TextPrimary, style = MaterialTheme.typography.bodyLarge)
            }
        }
    }
}

@Composable
private fun ConnectionCard(
    connectionState: HealthViewModel.ConnectionState,
    connectedAddress: String?,
    onConnectClick: () -> Unit,
    onDisconnectClick: () -> Unit
) {
    Card(
        colors = CardDefaults.cardColors(containerColor = CardSurface),
        shape = RoundedCornerShape(12.dp),
        modifier = Modifier.fillMaxWidth()
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(12.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Column(modifier = Modifier.weight(1f)) {
                Text(
                    text = "蓝牙连接",
                    style = MaterialTheme.typography.labelSmall,
                    color = TextSecondary
                )
                Text(
                    text = when (connectionState) {
                        HealthViewModel.ConnectionState.DISCONNECTED -> "未连接"
                        HealthViewModel.ConnectionState.CONNECTING -> "连接中..."
                        HealthViewModel.ConnectionState.CONNECTED -> "已连接"
                    },
                    style = MaterialTheme.typography.bodyMedium,
                    color = when (connectionState) {
                        HealthViewModel.ConnectionState.DISCONNECTED -> TextDim
                        HealthViewModel.ConnectionState.CONNECTING -> CyanPrimary
                        HealthViewModel.ConnectionState.CONNECTED -> CyanPrimary
                    }
                )
                if (connectedAddress != null) {
                    Text(
                        text = connectedAddress,
                        style = MaterialTheme.typography.bodySmall,
                        color = TextDim,
                        fontSize = 10.sp
                    )
                }
            }
            when (connectionState) {
                HealthViewModel.ConnectionState.DISCONNECTED -> {
                    Button(
                        onClick = onConnectClick,
                        colors = ButtonDefaults.buttonColors(containerColor = CyanPrimary)
                    ) {
                        Text("连接设备", color = TextPrimary)
                    }
                }
                HealthViewModel.ConnectionState.CONNECTING -> {
                    Text("连接中...", color = TextSecondary, style = MaterialTheme.typography.bodySmall)
                }
                HealthViewModel.ConnectionState.CONNECTED -> {
                    Button(
                        onClick = onDisconnectClick,
                        colors = ButtonDefaults.buttonColors(containerColor = AlarmRed)
                    ) {
                        Text("断开", color = TextPrimary)
                    }
                }
            }
        }
    }
}

@Composable
private fun AlarmBanner(onDismiss: () -> Unit) {
    Card(
        colors = CardDefaults.cardColors(containerColor = AlarmRed.copy(alpha = 0.3f)),
        shape = RoundedCornerShape(12.dp),
        modifier = Modifier.fillMaxWidth()
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(12.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(
                text = "⚠️ 检测到异常！心率/血氧/体温/跌倒可能超出正常范围",
                style = MaterialTheme.typography.bodyMedium,
                color = AlarmRed,
                modifier = Modifier.weight(1f)
            )
            Button(
                onClick = onDismiss,
                colors = ButtonDefaults.buttonColors(containerColor = AlarmRed),
                modifier = Modifier.padding(start = 8.dp)
            ) {
                Text("知道了", color = TextPrimary)
            }
        }
    }
}

@Composable
private fun MetricCard(
    type: MetricType,
    value: Float,
    accentColor: Color,
    onClick: () -> Unit
) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .clickable(onClick = onClick),
        colors = CardDefaults.cardColors(containerColor = CardSurface),
        shape = RoundedCornerShape(16.dp)
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Text(
                text = type.displayName,
                style = MaterialTheme.typography.labelMedium,
                color = TextSecondary,
                fontSize = 12.sp
            )
            
            Spacer(modifier = Modifier.height(8.dp))
            
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.Center
            ) {
                Text(
                    text = if (value == 0f) "--" else (if (type == MetricType.ECG) "%.0f" else "%.1f").format(value),
                    style = MaterialTheme.typography.headlineSmall,
                    color = accentColor,
                    fontSize = 28.sp,
                    fontWeight = FontWeight.Bold
                )
                Spacer(modifier = Modifier.padding(4.dp))
                Text(
                    text = if (value == 0f) "等待数据" else type.unit,
                    style = MaterialTheme.typography.bodySmall,
                    color = TextDim,
                    fontSize = 12.sp
                )
            }
            
            Spacer(modifier = Modifier.height(12.dp))
            
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(2.dp)
                    .background(accentColor.copy(alpha = 0.3f), RoundedCornerShape(1.dp))
            )
        }
    }
}
