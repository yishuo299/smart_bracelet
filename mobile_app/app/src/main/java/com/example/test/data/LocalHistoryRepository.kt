package com.example.test.data

import android.content.Context
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * 本地历史数据存储，数据存于应用内部存储
 */
class LocalHistoryRepository(private val context: Context) {
    
    private val historyFile: File
        get() = File(context.filesDir, HISTORY_FILE)
    
    companion object {
        private const val HISTORY_FILE = "health_history.txt"
        private const val MAX_RECORDS = 2000
    }
    
    /**
     * 追加一条记录
     */
    fun append(record: HealthRecord) {
        try {
            val line = record.toLine() + "\n"
            historyFile.appendText(line)
            trimIfNeeded()
        } catch (e: Exception) {
            // 忽略写入失败
        }
    }
    
    private fun trimIfNeeded() {
        try {
            val lines = historyFile.readLines()
            if (lines.size > MAX_RECORDS) {
                historyFile.writeText(lines.takeLast(MAX_RECORDS).joinToString("\n") + "\n")
            }
        } catch (e: Exception) {
            // 忽略
        }
    }
    
    /**
     * 获取所有历史记录（按时间倒序，最新的在前）
     */
    fun getAll(): List<HealthRecord> {
        return try {
            if (!historyFile.exists()) return emptyList()
            historyFile.readLines()
                .mapNotNull { HealthRecord.fromLine(it) }
                .sortedByDescending { it.timestamp }
        } catch (e: Exception) {
            emptyList()
        }
    }
    
    fun formatTime(timestamp: Long): String {
        return SimpleDateFormat("MM-dd HH:mm", Locale.getDefault()).format(Date(timestamp))
    }
}
