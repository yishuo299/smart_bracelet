package com.example.test.data

import android.annotation.SuppressLint
import android.bluetooth.BluetoothDevice
import android.content.Context
import androidx.lifecycle.ViewModel
import androidx.core.content.edit
import com.example.test.bluetooth.BluetoothManager
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import java.util.regex.Pattern

class HealthViewModel : ViewModel() {
    
    /** 蓝牙接收缓冲，用于拼接可能被分片的数据 */
    private val receiveBuffer = StringBuilder()
    private val receiveBufferMaxLen = 512
    
    /**
     * 与 driver.c 最新版本完全一致：
     * 1. Temperature:  XX.X C\r\n
     * 2. Heart Rate:  XX bpm\r\n 或 Heart Rate:  ---\r\n
     * 3. Blood Oxygen: XX %\r\n 或 Blood Oxygen: ---\r\n
     * 4. AD8232: val1 val2 val3 ...\r\n（心电多点，取最后一个作为当前值）
     * 5. Danger!!! 报警
     */
    private val tempPattern = Pattern.compile("Temperature:\\s*([\\d]+\\.?[\\d]*)\\s*C", Pattern.CASE_INSENSITIVE)
    private val tempShortPattern = Pattern.compile("([\\d]+\\.?[\\d]*)\\s*C")
    private val dangerPattern = Pattern.compile("Danger!!!")
    private val heartRatePattern = Pattern.compile("Heart Rate:\\s*(\\d+)\\s*bpm", Pattern.CASE_INSENSITIVE)
    private val spo2Pattern = Pattern.compile("Blood Oxygen:\\s*(\\d+)\\s*%?", Pattern.CASE_INSENSITIVE)
    private val ecgPattern = Pattern.compile("AD8232:(?:\\s+\\d+)*\\s+(\\d+)", Pattern.CASE_INSENSITIVE)
    
    private val _heartRate = MutableStateFlow(HealthMetricState(
        threshold = MetricThreshold(MetricType.HEART_RATE.defaultMin, MetricType.HEART_RATE.defaultMax)
    ))
    val heartRate: StateFlow<HealthMetricState> = _heartRate.asStateFlow()
    
    private val _spo2 = MutableStateFlow(HealthMetricState(
        threshold = MetricThreshold(MetricType.SPO2.defaultMin, MetricType.SPO2.defaultMax)
    ))
    val spo2: StateFlow<HealthMetricState> = _spo2.asStateFlow()
    
    private val _temperature = MutableStateFlow(HealthMetricState(
        threshold = MetricThreshold(MetricType.TEMPERATURE.defaultMin, MetricType.TEMPERATURE.defaultMax)
    ))
    val temperature: StateFlow<HealthMetricState> = _temperature.asStateFlow()
    
    private val _ecg = MutableStateFlow(HealthMetricState(
        threshold = MetricThreshold(MetricType.ECG.defaultMin, MetricType.ECG.defaultMax)
    ))
    val ecg: StateFlow<HealthMetricState> = _ecg.asStateFlow()
    
    private val _debugLog = MutableStateFlow<List<String>>(emptyList())
    val debugLog: StateFlow<List<String>> = _debugLog.asStateFlow()
    
    /** 蓝牙发送日志（App → MCU） */
    private val _sendLog = MutableStateFlow<List<String>>(emptyList())
    val sendLog: StateFlow<List<String>> = _sendLog.asStateFlow()
    
    private val _connectionState = MutableStateFlow(ConnectionState.DISCONNECTED)
    val connectionState: StateFlow<ConnectionState> = _connectionState.asStateFlow()
    
    private val _connectedAddress = MutableStateFlow<String?>(null)
    val connectedAddress: StateFlow<String?> = _connectedAddress.asStateFlow()
    
    /** 警报状态：收到 STM32 的 "Danger!!!" 时置为 true */
    private val _alarmActive = MutableStateFlow(false)
    val alarmActive: StateFlow<Boolean> = _alarmActive.asStateFlow()
    
    /** 已配对的蓝牙设备列表 (名称, 地址) */
    private val _pairedDevices = MutableStateFlow<List<Pair<String, String>>>(emptyList())
    val pairedDevices: StateFlow<List<Pair<String, String>>> = _pairedDevices.asStateFlow()
    
    /** 历史记录列表 */
    fun getHistoryRecords(context: Context): List<HealthRecord> {
        return LocalHistoryRepository(context.applicationContext).getAll()
    }
    
    lateinit var bluetoothManager: BluetoothManager
    private var appContext: Context? = null
    private val prefsName = "health_thresholds"
    
    enum class ConnectionState { DISCONNECTED, CONNECTING, CONNECTED }
    
    fun initBluetooth(context: Context) {
        appContext = context.applicationContext
        loadPersistedThresholds()
        bluetoothManager = BluetoothManager(context)
        bluetoothManager.setOnDataReceived { bytes ->
            val hex = bytes.joinToString(" ") { "%02X".format(it) }
            val displayText = formatPacketForDisplay(bytes)
            addDebugLog("[${bytes.size}B] HEX: $hex | $displayText")
            parseBluetoothData(bytes)
        }
        bluetoothManager.setOnDataSent { bytes ->
            val displayText = formatPacketForDisplay(bytes)
            addSendLog("[${bytes.size}B] SEND | $displayText")
        }
        bluetoothManager.setOnDisconnected {
            _connectionState.value = ConnectionState.DISCONNECTED
            _connectedAddress.value = null
            addDebugLog("[系统] 蓝牙已断开")
        }
    }
    
    /**
     * 调试显示：driver.c 9600 波特率发送 ASCII，有效则显示，否则显示 HEX
     */
    private fun formatPacketForDisplay(bytes: ByteArray): String {
        if (bytes.isEmpty()) return "[空]"
        val s = String(bytes, Charsets.UTF_8)
        return if (!s.contains('\uFFFD')) "STM32: ${s.replace("\r", "\\r").replace("\n", "\\n")}"
        else "原始: " + bytes.joinToString(" ") { "%02X".format(it.toInt() and 0xFF) }
    }
    
    /**
     * 解析蓝牙数据 - 与 driver.c 最新版一致
     * AD8232 发送多值 "AD8232: v1 v2 v3..."，取最后一个作为心电当前值
     */
    private fun parseBluetoothData(bytes: ByteArray) {
        if (bytes.isEmpty()) return
        val text = String(bytes, Charsets.UTF_8)
        receiveBuffer.append(text)
        if (receiveBuffer.length > receiveBufferMaxLen) {
            receiveBuffer.delete(0, receiveBuffer.length - receiveBufferMaxLen)
        }
        while (true) {
            val content = receiveBuffer.toString()
            var parsed = false
            val dangerMatcher = dangerPattern.matcher(content)
            if (dangerMatcher.find()) {
                _alarmActive.value = true
                receiveBuffer.delete(dangerMatcher.start(), dangerMatcher.end())
                parsed = true
                continue
            }
            val hrMatcher = heartRatePattern.matcher(content)
            if (hrMatcher.find()) {
                hrMatcher.group(1)?.toIntOrNull()?.let { v -> if (v in 40..220) updateMetric(_heartRate, v.toFloat()) }
                receiveBuffer.delete(hrMatcher.start(), hrMatcher.end())
                parsed = true
                continue
            }
            val spo2Matcher = spo2Pattern.matcher(content)
            if (spo2Matcher.find()) {
                spo2Matcher.group(1)?.toIntOrNull()?.let { v -> if (v in 70..100) updateMetric(_spo2, v.toFloat()) }
                receiveBuffer.delete(spo2Matcher.start(), spo2Matcher.end())
                parsed = true
                continue
            }
            val tempMatcher = tempPattern.matcher(content)
            if (tempMatcher.find()) {
                tempMatcher.group(1)?.toFloatOrNull()?.let { v -> if (v in 20f..45f) updateMetric(_temperature, v) }
                receiveBuffer.delete(tempMatcher.start(), tempMatcher.end())
                parsed = true
                continue
            }
            val tempShortMatcher = tempShortPattern.matcher(content)
            if (tempShortMatcher.find()) {
                tempShortMatcher.group(1)?.toFloatOrNull()?.let { v -> if (v in 20f..45f) updateMetric(_temperature, v) }
                receiveBuffer.delete(tempShortMatcher.start(), tempShortMatcher.end())
                parsed = true
                continue
            }
            val ecgMatcher = ecgPattern.matcher(content)
            if (ecgMatcher.find()) {
                ecgMatcher.group(1)?.toIntOrNull()?.let { raw ->
                    val value = if (raw in 0..4095) raw.toFloat() else 0f
                    updateMetric(_ecg, value)
                }
                receiveBuffer.delete(ecgMatcher.start(), ecgMatcher.end())
                parsed = true
                continue
            }
            if (!parsed) break
        }
        
        // 缓冲过长时保留尾部，避免内存增长
        if (receiveBuffer.length > receiveBufferMaxLen / 2) {
            val keepLen = 64
            if (receiveBuffer.length > keepLen) {
                receiveBuffer.delete(0, receiveBuffer.length - keepLen)
            }
        }
    }
    
    /** 供外部调用：直接更新各指标值（调试/测试用） */
    fun updateHeartRate(value: Float) = updateMetric(_heartRate, value)
    fun updateSpo2(value: Float) = updateMetric(_spo2, value)
    fun updateTemperature(value: Float) = updateMetric(_temperature, value)
    fun updateEcg(value: Float) = updateMetric(_ecg, value)
    
    private var lastSaveTime = 0L
    private val SAVE_INTERVAL_MS = 2000L
    
    private fun updateMetric(flow: MutableStateFlow<HealthMetricState>, value: Float) {
        flow.update { state ->
            val point = HistoryPoint(System.currentTimeMillis(), value)
            val newHistory = (state.history + point).takeLast(100)
            val alarming = value < state.threshold.min || value > state.threshold.max
            state.copy(current = value, history = newHistory, isAlarming = alarming)
        }
        trySaveHistoryRecord()
    }
    
    private fun trySaveHistoryRecord() {
        val ctx = appContext ?: return
        val now = System.currentTimeMillis()
        if (now - lastSaveTime < SAVE_INTERVAL_MS) return
        lastSaveTime = now
        val hr = _heartRate.value.current
        val spo2 = _spo2.value.current
        val temp = _temperature.value.current
        val ecg = _ecg.value.current
        if (hr == 0f && spo2 == 0f && temp == 0f && ecg == 0f) return
        val repo = LocalHistoryRepository(ctx)
        repo.append(HealthRecord(now, hr, spo2, temp, ecg))
    }
    
    fun setThreshold(type: MetricType, min: Float, max: Float) {
        val threshold = MetricThreshold(min, max)
        val flow = getFlow(type)
        flow.update { it.copy(threshold = threshold) }
        persistThreshold(type, min, max)
    }
    
    private fun loadPersistedThresholds() {
        val ctx = appContext ?: return
        val prefs = ctx.getSharedPreferences(prefsName, Context.MODE_PRIVATE)
        val hrMin = prefs.getFloat("hr_min", MetricType.HEART_RATE.defaultMin)
        val hrMax = prefs.getFloat("hr_max", MetricType.HEART_RATE.defaultMax)
        val spo2Min = prefs.getFloat("spo2_min", MetricType.SPO2.defaultMin)
        val spo2Max = prefs.getFloat("spo2_max", MetricType.SPO2.defaultMax)
        val tempMin = prefs.getFloat("temp_min", MetricType.TEMPERATURE.defaultMin)
        val tempMax = prefs.getFloat("temp_max", MetricType.TEMPERATURE.defaultMax)
        val ecgMin = prefs.getFloat("ecg_min", MetricType.ECG.defaultMin)
        val ecgMax = prefs.getFloat("ecg_max", MetricType.ECG.defaultMax)
        _heartRate.update { it.copy(threshold = MetricThreshold(hrMin, hrMax)) }
        _spo2.update { it.copy(threshold = MetricThreshold(spo2Min, spo2Max)) }
        _temperature.update { it.copy(threshold = MetricThreshold(tempMin, tempMax)) }
        _ecg.update { it.copy(threshold = MetricThreshold(ecgMin, ecgMax)) }
    }
    
    private fun persistThreshold(type: MetricType, min: Float, max: Float) {
        val ctx = appContext ?: return
        ctx.getSharedPreferences(prefsName, Context.MODE_PRIVATE).edit {
            when (type) {
                MetricType.HEART_RATE -> { putFloat("hr_min", min); putFloat("hr_max", max) }
                MetricType.SPO2 -> { putFloat("spo2_min", min); putFloat("spo2_max", max) }
                MetricType.TEMPERATURE -> { putFloat("temp_min", min); putFloat("temp_max", max) }
                MetricType.ECG -> { putFloat("ecg_min", min); putFloat("ecg_max", max) }
            }
        }
    }
    
    private fun getFlow(type: MetricType): MutableStateFlow<HealthMetricState> = when(type) {
        MetricType.HEART_RATE -> _heartRate
        MetricType.SPO2 -> _spo2
        MetricType.TEMPERATURE -> _temperature
        MetricType.ECG -> _ecg
    }
    
    fun getState(type: MetricType): StateFlow<HealthMetricState> = when(type) {
        MetricType.HEART_RATE -> heartRate
        MetricType.SPO2 -> spo2
        MetricType.TEMPERATURE -> temperature
        MetricType.ECG -> ecg
    }
    
    fun connect(address: String) {
        if (_connectionState.value == ConnectionState.CONNECTING) return
        _connectionState.value = ConnectionState.CONNECTING
        addDebugLog("[系统] 正在连接 $address...")
        bluetoothManager.connect(address) { success, error ->
            if (success) {
                _connectionState.value = ConnectionState.CONNECTED
                _connectedAddress.value = address
                addDebugLog("[系统] 连接成功: $address")
            } else {
                _connectionState.value = ConnectionState.DISCONNECTED
                addDebugLog("[系统] 连接失败: ${error ?: "未知错误"}")
            }
        }
    }
    
    fun disconnect() {
        bluetoothManager.disconnect()
        _connectionState.value = ConnectionState.DISCONNECTED
        _connectedAddress.value = null
        addDebugLog("[系统] 已断开连接")
    }
    
    fun addDebugLog(message: String) {
        _debugLog.update { logs ->
            (logs + message).takeLast(200)
        }
    }
    
    fun clearDebugLog() {
        _debugLog.value = emptyList()
    }
    
    fun addSendLog(message: String) {
        _sendLog.update { logs -> (logs + message).takeLast(200) }
    }
    
    fun clearSendLog() {
        _sendLog.value = emptyList()
    }
    
    /** 通过蓝牙发送数据到 MCU */
    fun sendBluetoothData(text: String): Boolean {
        if (!::bluetoothManager.isInitialized || !bluetoothManager.isConnected()) return false
        return bluetoothManager.send(text)
    }
    
    /** 分开发送：仅发送当前指标的阈值到单片机，格式 TH:索引:值 */
    fun sendThresholdToMcu(type: MetricType, min: Float, max: Float): Boolean {
        val commands = when (type) {
            MetricType.HEART_RATE -> listOf("TH:0:${min.toInt()}\r\n", "TH:1:${max.toInt()}\r\n")
            MetricType.SPO2 -> listOf("TH:2:${min.toInt()}\r\n")  // MCU 仅血氧下限
            MetricType.TEMPERATURE -> listOf("TH:3:${min.toInt()}\r\n", "TH:4:${max.toInt()}\r\n")
            MetricType.ECG -> listOf("TH:5:${min.toInt()}\r\n", "TH:6:${max.toInt()}\r\n")
        }
        var ok = true
        for (cmd in commands) {
            if (!sendBluetoothData(cmd)) ok = false
        }
        return ok
    }
    
    /** 用户手动解除警报显示 */
    fun dismissAlarm() {
        _alarmActive.value = false
    }
    
    /** 刷新已配对的蓝牙设备列表 */
    @SuppressLint("MissingPermission")
    fun refreshPairedDevices() {
        if (!::bluetoothManager.isInitialized) return
        val devices = bluetoothManager.getPairedDevices()
        _pairedDevices.value = devices.map { device ->
            (device.name ?: "未知设备") to device.address
        }.sortedBy { it.first }
        addDebugLog("[系统] 已刷新 ${devices.size} 个配对设备")
    }
    
    override fun onCleared() {
        super.onCleared()
        if (::bluetoothManager.isInitialized) {
            bluetoothManager.release()
        }
    }
}
