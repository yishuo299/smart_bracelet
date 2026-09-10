package com.example.test.data

enum class MetricType(
    val displayName: String,
    val unit: String,
    val defaultMin: Float,
    val defaultMax: Float
) {
    HEART_RATE("心率", "BPM", 60f, 100f),
    SPO2("血氧", "%", 95f, 100f),
    TEMPERATURE("体温", "°C", 36.0f, 37.5f),
    ECG("心电", "ADC", 0f, 4095f)
}

data class MetricThreshold(
    val min: Float,
    val max: Float
)

/** 历史数据点，包含时间戳和数值 */
data class HistoryPoint(val timestamp: Long, val value: Float)

data class HealthMetricState(
    val current: Float = 0f,
    val history: List<HistoryPoint> = emptyList(),
    val threshold: MetricThreshold = MetricThreshold(0f, 0f),
    val isAlarming: Boolean = false
)
