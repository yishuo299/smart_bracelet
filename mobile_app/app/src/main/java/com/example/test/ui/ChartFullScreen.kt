package com.example.test.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.example.test.data.HealthViewModel
import com.example.test.data.MetricType
import com.example.test.ui.components.LineChart
import com.example.test.ui.theme.CardSurface
import com.example.test.ui.theme.EcgGreen
import com.example.test.ui.theme.HeartRed
import com.example.test.ui.theme.Spo2Blue
import com.example.test.ui.theme.TempAmber
import com.example.test.ui.theme.TextDim
import com.example.test.ui.theme.TextPrimary
import com.example.test.ui.theme.TextSecondary

/**
 * 纯数据绘图界面：仅展示折线图，点击阈值详情页的图表区域可进入
 */
@Composable
fun ChartFullScreen(
    type: MetricType,
    viewModel: HealthViewModel,
    onBack: () -> Unit
) {
    val state = viewModel.getState(type).collectAsState()
    val metricState = state.value

    val accentColor = when (type) {
        MetricType.HEART_RATE -> HeartRed
        MetricType.SPO2 -> Spo2Blue
        MetricType.TEMPERATURE -> TempAmber
        MetricType.ECG -> EcgGreen
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(CardSurface)
    ) {
        // 顶部栏
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp)
        ) {
            Button(
                onClick = onBack,
                colors = ButtonDefaults.buttonColors(containerColor = CardSurface),
                modifier = Modifier.align(Alignment.CenterStart)
            ) {
                Text("← 返回", color = TextPrimary)
            }
            Text(
                text = "${type.displayName} 数据曲线",
                style = MaterialTheme.typography.titleLarge,
                color = accentColor,
                fontWeight = FontWeight.Bold,
                modifier = Modifier.align(Alignment.Center)
            )
        }

        if (metricState.history.isEmpty()) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(32.dp),
                verticalArrangement = Arrangement.Center,
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                Text(
                    text = "暂无数据",
                    style = MaterialTheme.typography.bodyLarge,
                    color = TextDim
                )
                Text(
                    text = "请连接设备并等待数据采集",
                    style = MaterialTheme.typography.bodySmall,
                    color = TextSecondary,
                    modifier = Modifier.padding(top = 8.dp)
                )
            }
        } else {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(horizontal = 16.dp)
            ) {
                Text(
                    text = "数据点数: ${metricState.history.size}",
                    style = MaterialTheme.typography.bodySmall,
                    color = TextSecondary,
                    modifier = Modifier.padding(bottom = 8.dp)
                )
                LineChart(
                    data = metricState.history.map { it.value },
                    lineColor = accentColor,
                    minValue = metricState.threshold.min,
                    maxValue = metricState.threshold.max,
                    showThresholdLines = true,
                    chartHeightDp = 400.dp,
                    showAxes = true,
                    timestamps = metricState.history.map { it.timestamp },
                    valueUnit = type.unit
                )
            }
        }
    }
}
