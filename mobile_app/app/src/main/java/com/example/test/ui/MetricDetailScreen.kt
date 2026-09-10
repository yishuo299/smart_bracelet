package com.example.test.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.test.data.HealthViewModel
import com.example.test.data.MetricType
import com.example.test.ui.components.LineChart
import com.example.test.ui.theme.AlarmRed
import com.example.test.ui.theme.CardSurface
import com.example.test.ui.theme.CyanPrimary
import com.example.test.ui.theme.EcgGreen
import com.example.test.ui.theme.HeartRed
import com.example.test.ui.theme.Spo2Blue
import com.example.test.ui.theme.SuccessGreen
import com.example.test.ui.theme.TempAmber
import com.example.test.ui.theme.TextDim
import com.example.test.ui.theme.TextPrimary
import com.example.test.ui.theme.TextSecondary

@Composable
fun MetricDetailScreen(
    type: MetricType,
    viewModel: HealthViewModel,
    onBack: () -> Unit,
    onChartClick: () -> Unit = {}
) {
    val state = viewModel.getState(type).collectAsState()
    val metricState = state.value
    
    val accentColor = when (type) {
        MetricType.HEART_RATE -> HeartRed
        MetricType.SPO2 -> Spo2Blue
        MetricType.TEMPERATURE -> TempAmber
        MetricType.ECG -> EcgGreen
    }
    
    val minInput = remember { mutableStateOf(metricState.threshold.min.toString()) }
    val maxInput = remember { mutableStateOf(metricState.threshold.max.toString()) }
    
    Column(
        modifier = Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState())
            .padding(16.dp)
    ) {
        // Header
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = 20.dp),
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
                text = type.displayName,
                style = MaterialTheme.typography.headlineMedium,
                color = accentColor,
                fontWeight = FontWeight.Bold
            )
        }
        
        // Current value card
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = 16.dp),
            colors = CardDefaults.cardColors(containerColor = CardSurface),
            shape = RoundedCornerShape(16.dp)
        ) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(24.dp),
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                Text(
                    text = "当前数值",
                    style = MaterialTheme.typography.bodyMedium,
                    color = TextSecondary
                )
                Spacer(modifier = Modifier.height(12.dp))
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.Center
                ) {
                    Text(
                        text = if (type == MetricType.ECG) "%.0f".format(metricState.current) else "%.1f".format(metricState.current),
                        style = MaterialTheme.typography.displaySmall,
                        color = accentColor,
                        fontSize = 48.sp,
                        fontWeight = FontWeight.Bold
                    )
                    Spacer(modifier = Modifier.padding(8.dp))
                    Text(
                        text = type.unit,
                        style = MaterialTheme.typography.headlineSmall,
                        color = TextDim
                    )
                }
                
                Spacer(modifier = Modifier.height(16.dp))
                
                // Alarm status
                val isAlarming = metricState.isAlarming
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .background(
                            if (isAlarming) AlarmRed.copy(alpha = 0.2f) else SuccessGreen.copy(alpha = 0.2f),
                            RoundedCornerShape(8.dp)
                        )
                        .padding(12.dp),
                    horizontalArrangement = Arrangement.Center
                ) {
                    Text(
                        text = if (isAlarming) "⚠️ 超出正常范围" else "✓ 正常范围内",
                        color = if (isAlarming) AlarmRed else SuccessGreen,
                        fontWeight = FontWeight.Bold
                    )
                }
            }
        }
        
        // Chart - 点击可进入纯绘图界面
        Text(
            text = "数据趋势（点击图表区域查看大图）",
            style = MaterialTheme.typography.titleMedium,
            color = TextPrimary,
            modifier = Modifier.padding(bottom = 8.dp)
        )

        Box(
            modifier = Modifier
                .clickable(onClick = onChartClick)
                .padding(bottom = 20.dp)
        ) {
            LineChart(
                data = metricState.history.map { it.value },
                lineColor = accentColor,
                minValue = metricState.threshold.min,
                maxValue = metricState.threshold.max,
                showThresholdLines = true
            )
        }
        
        // Threshold settings
        Text(
            text = "报警阈值设置",
            style = MaterialTheme.typography.titleMedium,
            color = TextPrimary,
            modifier = Modifier.padding(bottom = 12.dp)
        )
        
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = 16.dp),
            colors = CardDefaults.cardColors(containerColor = CardSurface),
            shape = RoundedCornerShape(16.dp)
        ) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(16.dp)
            ) {
                OutlinedTextField(
                    value = minInput.value,
                    onValueChange = { minInput.value = it },
                    label = { Text("最小值", color = TextSecondary) },
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(bottom = 12.dp),
                    keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
                    textStyle = MaterialTheme.typography.bodyMedium.copy(color = TextPrimary)
                )
                
                OutlinedTextField(
                    value = maxInput.value,
                    onValueChange = { maxInput.value = it },
                    label = { Text("最大值", color = TextSecondary) },
                    modifier = Modifier.fillMaxWidth(),
                    keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
                    textStyle = MaterialTheme.typography.bodyMedium.copy(color = TextPrimary)
                )
                
                Spacer(modifier = Modifier.height(16.dp))
                
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    Button(
                        onClick = {
                            val min = minInput.value.toFloatOrNull() ?: type.defaultMin
                            val max = maxInput.value.toFloatOrNull() ?: type.defaultMax
                            viewModel.setThreshold(type, min, max)
                            viewModel.sendThresholdToMcu(type, min, max)
                        },
                        modifier = Modifier.weight(1f),
                        colors = ButtonDefaults.buttonColors(containerColor = accentColor)
                    ) {
                        Text("保存并发送", color = Color.Black, fontWeight = FontWeight.Bold)
                    }
                    OutlinedButton(
                        onClick = {
                            val min = minInput.value.toFloatOrNull() ?: metricState.threshold.min
                            val max = maxInput.value.toFloatOrNull() ?: metricState.threshold.max
                            viewModel.sendThresholdToMcu(type, min, max)
                        },
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("仅发送", color = TextPrimary, fontSize = 12.sp)
                    }
                }
            }
        }
        
        // Data stats
        if (metricState.history.isNotEmpty()) {
            Text(
                text = "数据统计",
                style = MaterialTheme.typography.titleMedium,
                color = TextPrimary,
                modifier = Modifier.padding(bottom = 12.dp)
            )
            
            Card(
                modifier = Modifier.fillMaxWidth(),
                colors = CardDefaults.cardColors(containerColor = CardSurface),
                shape = RoundedCornerShape(16.dp)
            ) {
                Column(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(16.dp)
                ) {
                    val values = metricState.history.map { it.value }
                    val avg = values.average().toFloat()
                    val min = values.minOrNull() ?: 0f
                    val max = values.maxOrNull() ?: 0f
                    val fmt = if (type == MetricType.ECG) "%.0f" else "%.1f"
                    StatRow("平均值", String.format("$fmt %s", avg, type.unit))
                    StatRow("最小值", String.format("$fmt %s", min, type.unit))
                    StatRow("最大值", String.format("$fmt %s", max, type.unit))
                    StatRow("数据点数", metricState.history.size.toString())
                }
            }
        }
    }
}

@Composable
private fun StatRow(label: String, value: String) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 8.dp),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(text = label, color = TextSecondary)
        Text(text = value, color = TextPrimary, fontWeight = FontWeight.Bold)
    }
}
