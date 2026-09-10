package com.example.test.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.test.data.HealthRecord
import com.example.test.data.HealthViewModel
import com.example.test.data.LocalHistoryRepository
import com.example.test.ui.theme.CardSurface
import com.example.test.ui.theme.CyanPrimary
import com.example.test.ui.theme.TextDim
import com.example.test.ui.theme.TextPrimary
import com.example.test.ui.theme.TextSecondary

@Composable
fun HistoryScreen(
    viewModel: HealthViewModel,
    onBack: () -> Unit
) {
    val context = LocalContext.current
    val repo = remember { LocalHistoryRepository(context) }
    var records by remember { mutableStateOf<List<HealthRecord>>(emptyList()) }
    LaunchedEffect(Unit) { records = repo.getAll() }
    
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = 16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Button(
                onClick = onBack,
                colors = ButtonDefaults.buttonColors(containerColor = CardSurface)
            ) {
                Text("← 返回", color = TextPrimary)
            }
            Spacer(modifier = Modifier.weight(1f))
            Text(
                text = "历史数据",
                style = MaterialTheme.typography.headlineMedium,
                color = TextPrimary,
                fontWeight = FontWeight.Bold
            )
        }
        
        Text(
            text = "本地存储的健康记录，按时间倒序显示",
            style = MaterialTheme.typography.bodySmall,
            color = TextSecondary,
            modifier = Modifier.padding(bottom = 12.dp)
        )
        
        if (records.isEmpty()) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(32.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.Center
            ) {
                Text(
                    text = "暂无历史数据",
                    color = TextDim,
                    style = MaterialTheme.typography.bodyLarge
                )
                Text(
                    text = "连接设备并接收数据后会在此保存",
                    color = TextDim,
                    fontSize = 12.sp,
                    modifier = Modifier.padding(top = 8.dp)
                )
            }
        } else {
            LazyColumn(
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                items(records) { record ->
                    HistoryRecordCard(record = record, repo = repo)
                }
            }
        }
    }
}

@Composable
private fun HistoryRecordCard(
    record: HealthRecord,
    repo: LocalHistoryRepository
) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = CardSurface),
        shape = RoundedCornerShape(12.dp)
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column(modifier = Modifier.weight(1f)) {
                Text(
                    text = repo.formatTime(record.timestamp),
                    style = MaterialTheme.typography.titleSmall,
                    color = CyanPrimary
                )
                Spacer(modifier = Modifier.height(6.dp))
                Row(
                    horizontalArrangement = Arrangement.spacedBy(12.dp)
                ) {
                    Text(
                        text = "心率 ${formatVal(record.heartRate)}",
                        fontSize = 12.sp,
                        color = TextSecondary
                    )
                    Text(
                        text = "血氧 ${formatVal(record.spo2)}",
                        fontSize = 12.sp,
                        color = TextSecondary
                    )
                    Text(
                        text = "体温 ${formatVal(record.temperature)}",
                        fontSize = 12.sp,
                        color = TextSecondary
                    )
                    Text(
                        text = "心电 ${formatVal(record.ecg, asInt = true)}",
                        fontSize = 12.sp,
                        color = TextSecondary
                    )
                }
            }
        }
    }
}

private fun formatVal(v: Float, asInt: Boolean = false): String =
    if (v == 0f) "--" else if (asInt) "%.0f".format(v) else "%.1f".format(v)
