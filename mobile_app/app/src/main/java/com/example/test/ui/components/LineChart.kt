package com.example.test.ui.components

import android.graphics.Paint
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.test.ui.theme.AlarmRed
import com.example.test.ui.theme.CardSurface
import com.example.test.ui.theme.DividerColor
import com.example.test.ui.theme.TextSecondary
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * Y轴使用实际数据范围与阈值的并集，确保超出阈值的数据也能完整绘制
 * @param chartHeightDp 图表高度，默认200.dp
 * @param showAxes 是否显示坐标轴（时间X轴、数据Y轴），大图模式使用
 * @param timestamps 时间戳列表，与data一一对应，showAxes时用于X轴标签
 * @param valueUnit 数值单位，用于Y轴标签（如 BPM、°C）
 */
@Composable
fun LineChart(
    data: List<Float>,
    modifier: Modifier = Modifier,
    lineColor: Color = Color(0xFF00E5FF),
    backgroundColor: Color = CardSurface,
    minValue: Float = 0f,
    maxValue: Float = 100f,
    showThresholdLines: Boolean = false,
    thresholdLineColor: Color = AlarmRed.copy(alpha = 0.75f),
    chartHeightDp: Dp = 200.dp,
    showAxes: Boolean = false,
    timestamps: List<Long>? = null,
    valueUnit: String? = null
) {
    val density = LocalDensity.current
    val axisLabelPaint = remember(density) {
        Paint().apply {
            color = TextSecondary.toArgb()
            textSize = with(density) { 11.sp.toPx() }
            isAntiAlias = true
        }
    }

    Box(
        modifier = modifier
            .fillMaxWidth()
            .height(chartHeightDp)
            .clip(RoundedCornerShape(12.dp))
            .background(backgroundColor)
    ) {
        androidx.compose.foundation.Canvas(
            modifier = Modifier
                .fillMaxWidth()
                .height(chartHeightDp)
        ) {
            if (data.isEmpty()) return@Canvas

            val width = size.width
            val height = size.height
            val leftPadding = if (showAxes) 52f else 16f
            val rightPadding = 16f
            val topPadding = 16f
            val bottomPadding = if (showAxes) 36f else 16f

            val chartLeft = leftPadding
            val chartTop = topPadding
            val chartWidth = width - leftPadding - rightPadding
            val chartHeight = height - topPadding - bottomPadding

            val dataMin = data.minOrNull() ?: minValue
            val dataMax = data.maxOrNull() ?: maxValue
            val effectiveMin = minOf(minValue, dataMin) - 0.05f * kotlin.math.max(1f, maxValue - minValue)
            val effectiveMax = maxOf(maxValue, dataMax) + 0.05f * kotlin.math.max(1f, maxValue - minValue)
            val range = (effectiveMax - effectiveMin).coerceAtLeast(0.01f)

            // Draw grid lines
            val gridLines = 4
            for (i in 0..gridLines) {
                val y = chartTop + (chartHeight / gridLines) * i
                drawLine(
                    color = DividerColor,
                    start = Offset(chartLeft, y),
                    end = Offset(chartLeft + chartWidth, y),
                    strokeWidth = 0.5f
                )
            }

            // Draw threshold reference lines (optional)
            if (showThresholdLines && data.isNotEmpty()) {
                val yMin = chartTop + chartHeight - ((minValue - effectiveMin) / range * chartHeight)
                val yMax = chartTop + chartHeight - ((maxValue - effectiveMin) / range * chartHeight)
                if (yMin in chartTop..(chartTop + chartHeight)) {
                    drawLine(
                        color = thresholdLineColor,
                        start = Offset(chartLeft, yMin),
                        end = Offset(chartLeft + chartWidth, yMin),
                        strokeWidth = 2f
                    )
                }
                if (yMax in chartTop..(chartTop + chartHeight) && kotlin.math.abs(yMax - yMin) > 2f) {
                    drawLine(
                        color = thresholdLineColor,
                        start = Offset(chartLeft, yMax),
                        end = Offset(chartLeft + chartWidth, yMax),
                        strokeWidth = 2f
                    )
                }
            }

            // Draw axes and labels when showAxes
            if (showAxes) {
                // Y-axis line
                drawLine(
                    color = DividerColor,
                    start = Offset(chartLeft, chartTop),
                    end = Offset(chartLeft, chartTop + chartHeight),
                    strokeWidth = 2f
                )
                // X-axis line
                drawLine(
                    color = DividerColor,
                    start = Offset(chartLeft, chartTop + chartHeight),
                    end = Offset(chartLeft + chartWidth, chartTop + chartHeight),
                    strokeWidth = 2f
                )

                // Y-axis labels (data values)
                val yLabelCount = 5
                for (i in 0 until yLabelCount) {
                    val value = effectiveMin + (effectiveMax - effectiveMin) * (yLabelCount - i) / yLabelCount
                    val y = chartTop + chartHeight * i / yLabelCount
                    val label = when {
                        value >= 1000 -> "%.0f".format(value)
                        value >= 100 -> "%.1f".format(value)
                        value >= 10 -> "%.1f".format(value)
                        value >= 1 -> "%.2f".format(value)
                        else -> "%.2f".format(value)
                    } + (valueUnit?.let { " $it" } ?: "")
                    drawContext.canvas.nativeCanvas.drawText(
                        label,
                        chartLeft - 48f,
                        y + axisLabelPaint.textSize / 3,
                        axisLabelPaint
                    )
                }

                // X-axis labels (time)
                val timestampsList = if (timestamps != null && timestamps.size == data.size) timestamps else null
                if (timestampsList != null && timestampsList.isNotEmpty()) {
                    val timeFormat = SimpleDateFormat("HH:mm:ss", Locale.getDefault())
                    val xLabelCount = 5
                    for (i in 0 until xLabelCount) {
                        val idx = (data.size - 1) * i / (xLabelCount - 1).coerceAtLeast(1)
                        val ts = timestampsList[idx]
                        val x = chartLeft + chartWidth * i / (xLabelCount - 1).coerceAtLeast(1)
                        val label = timeFormat.format(Date(ts))
                        drawContext.canvas.nativeCanvas.drawText(
                            label,
                            x - axisLabelPaint.measureText(label) / 2,
                            chartTop + chartHeight + axisLabelPaint.textSize + 4f,
                            axisLabelPaint
                        )
                    }
                }
            }

            // Draw line chart
            if (data.size > 1) {
                val pointSpacing = chartWidth / (data.size - 1).coerceAtLeast(1)

                val path = Path().apply {
                    for (i in data.indices) {
                        val x = chartLeft + i * pointSpacing
                        val normalizedValue = (data[i] - effectiveMin) / range
                        val y = chartTop + chartHeight - (normalizedValue * chartHeight)

                        if (i == 0) {
                            moveTo(x, y)
                        } else {
                            lineTo(x, y)
                        }
                    }
                }

                drawPath(
                    path = path,
                    color = lineColor,
                    style = Stroke(width = 2.5f)
                )

                // Draw points
                for (i in data.indices) {
                    val x = chartLeft + i * pointSpacing
                    val normalizedValue = (data[i] - effectiveMin) / range
                    val y = chartTop + chartHeight - (normalizedValue * chartHeight)

                    drawCircle(
                        color = lineColor,
                        radius = 3f,
                        center = Offset(x, y)
                    )
                }
            }
        }
    }
}
