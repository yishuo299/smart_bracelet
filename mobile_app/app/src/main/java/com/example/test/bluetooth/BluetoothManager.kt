package com.example.test.bluetooth

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothSocket
import android.content.Context
import android.os.Handler
import android.os.Looper
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream
import java.util.UUID
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

/**
 * 蓝牙连接与数据接收管理器
 * 从 led-matrix 项目提取并简化，仅保留：查询已配对设备、连接、接收数据
 */
class BluetoothManager(private val context: Context) {

    companion object {
        private val SPP_UUID: UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB")
    }

    private val mainHandler = Handler(Looper.getMainLooper())
    private val executor = Executors.newSingleThreadExecutor()
    private val lock = Any()

    private var bluetoothSocket: BluetoothSocket? = null
    private var isConnected = false
    private var receiveThread: Thread? = null

    private val adapter: BluetoothAdapter?
        get() = (context.getSystemService(Context.BLUETOOTH_SERVICE) as? android.bluetooth.BluetoothManager)?.adapter

    enum class BluetoothStatus {
        NONE,       // 无蓝牙
        CLOSED,     // 蓝牙未开启
        OK          // 蓝牙就绪
    }

    /**
     * 检查蓝牙状态
     */
    fun checkBluetooth(): BluetoothStatus {
        val adapter = this.adapter ?: return BluetoothStatus.NONE
        return if (!adapter.isEnabled) BluetoothStatus.CLOSED else BluetoothStatus.OK
    }

    /**
     * 获取已配对的蓝牙设备列表
     */
    @SuppressLint("MissingPermission")
    fun getPairedDevices(): Set<BluetoothDevice> {
        return adapter?.bondedDevices ?: emptySet()
    }

    /**
     * 获取当前连接状态
     */
    fun isConnected(): Boolean = isConnected && bluetoothSocket?.isConnected == true

    /**
     * 获取当前连接设备地址
     */
    fun getConnectedAddress(): String? {
        return if (isConnected && bluetoothSocket != null) {
            try {
                bluetoothSocket!!.remoteDevice.address
            } catch (e: Exception) {
                null
            }
        } else null
    }

    /**
     * 连接指定地址的蓝牙设备
     */
    @SuppressLint("MissingPermission")
    fun connect(address: String, callback: (Boolean, String?) -> Unit) {
        if (checkBluetooth() != BluetoothStatus.OK) {
            mainHandler.post { callback(false, "蓝牙未开启") }
            return
        }
        val device = adapter?.getRemoteDevice(address)
        if (device == null) {
            mainHandler.post { callback(false, "设备不存在") }
            return
        }
        disconnect()
        executor.execute {
            try {
                var socket: BluetoothSocket? = null
                try {
                    socket = device.createInsecureRfcommSocketToServiceRecord(SPP_UUID)
                    socket.connect()
                } catch (e: IOException) {
                    socket?.close()
                    socket = createRfcommSocketByReflect(device)
                    socket?.connect()
                }
                if (socket != null && socket.isConnected) {
                    synchronized(lock) {
                        bluetoothSocket = socket
                        isConnected = true
                    }
                    startReceiveLoop()
                    mainHandler.post { callback(true, null) }
                } else {
                    mainHandler.post { callback(false, "连接失败") }
                }
            } catch (e: Exception) {
                mainHandler.post { callback(false, e.message ?: "连接异常") }
            }
        }
    }

    @SuppressLint("PrivateApi")
    private fun createRfcommSocketByReflect(device: BluetoothDevice): BluetoothSocket? {
        return try {
            val m = device.javaClass.getMethod("createRfcommSocket", Int::class.javaPrimitiveType)
            m.invoke(device, 1) as? BluetoothSocket
        } catch (e: Exception) {
            null
        }
    }

    /**
     * 启动数据接收线程
     */
    private fun startReceiveLoop() {
        receiveThread?.interrupt()
        receiveThread = Thread {
            try {
                val inputStream: InputStream = bluetoothSocket?.inputStream ?: return@Thread
                val buffer = ByteArray(1024)
                var bytes: Int
                while (isConnected && bluetoothSocket?.isConnected == true && !Thread.currentThread().isInterrupted) {
                    bytes = inputStream.read(buffer)
                    if (bytes > 0) {
                        val data = buffer.copyOf(bytes)
                        mainHandler.post { onDataReceived?.invoke(data) }
                    }
                }
            } catch (e: IOException) {
                if (isConnected) {
                    isConnected = false
                    try { bluetoothSocket?.close() } catch (_: IOException) { }
                    bluetoothSocket = null
                    mainHandler.post { onDisconnected?.invoke() }
                }
            } catch (e: Exception) {
                // ignore
            }
        }.apply { start() }
    }

    private var onDataReceived: ((ByteArray) -> Unit)? = null
    private var onDisconnected: (() -> Unit)? = null
    private var onDataSent: ((ByteArray) -> Unit)? = null

    /**
     * 设置数据接收回调
     */
    fun setOnDataReceived(callback: (ByteArray) -> Unit) {
        onDataReceived = callback
    }

    /**
     * 设置断开连接回调
     */
    fun setOnDisconnected(callback: () -> Unit) {
        onDisconnected = callback
    }

    /**
     * 设置数据发送回调（用于调试日志）
     */
    fun setOnDataSent(callback: (ByteArray) -> Unit) {
        onDataSent = callback
    }

    /**
     * 通过蓝牙发送字符串到设备（UTF-8 编码）
     * @return 是否发送成功
     */
    fun send(text: String): Boolean {
        return send(text.toByteArray(Charsets.UTF_8))
    }

    /**
     * 通过蓝牙发送字节数组到设备
     * @return 是否发送成功
     */
    fun send(bytes: ByteArray): Boolean {
        if (bytes.isEmpty()) return true
        return try {
            val socket = synchronized(lock) { bluetoothSocket }
            val out: OutputStream? = socket?.outputStream
            if (out != null && socket?.isConnected == true) {
                out.write(bytes)
                out.flush()
                mainHandler.post { onDataSent?.invoke(bytes) }
                true
            } else false
        } catch (e: Exception) {
            false
        }
    }

    /**
     * 断开连接
     */
    fun disconnect() {
        isConnected = false
        receiveThread?.interrupt()
        receiveThread = null
        try {
            bluetoothSocket?.close()
        } catch (e: IOException) { }
        bluetoothSocket = null
    }

    fun release() {
        disconnect()
        executor.shutdown()
    }
}
