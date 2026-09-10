package com.example.test.data

/**
 * 单条健康记录，用于本地历史存储
 */
data class HealthRecord(
    val timestamp: Long,
    val heartRate: Float,
    val spo2: Float,
    val temperature: Float,
    val ecg: Float
) {
    fun toLine(): String = "$timestamp|$heartRate|$spo2|$temperature|$ecg"
    
    companion object {
        fun fromLine(line: String): HealthRecord? {
            val parts = line.split("|")
            if (parts.size != 5) return null
            return try {
                HealthRecord(
                    timestamp = parts[0].toLong(),
                    heartRate = parts[1].toFloat(),
                    spo2 = parts[2].toFloat(),
                    temperature = parts[3].toFloat(),
                    ecg = parts[4].toFloat()
                )
            } catch (e: Exception) {
                null
            }
        }
    }
}
