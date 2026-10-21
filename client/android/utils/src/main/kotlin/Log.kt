package org.amnezia.vpn.util

import android.app.Application
import android.content.Context
import android.os.Build
import android.os.Process
import java.io.File
import java.io.IOException
import java.io.RandomAccessFile
import java.nio.channels.FileChannel
import java.nio.channels.FileLock
import java.time.LocalDateTime
import java.time.format.DateTimeFormatter
import java.time.ZonedDateTime
import java.time.ZoneOffset
import java.util.concurrent.locks.ReentrantLock
import org.amnezia.vpn.util.Log.Priority.D
import org.amnezia.vpn.util.Log.Priority.E
import org.amnezia.vpn.util.Log.Priority.F
import org.amnezia.vpn.util.Log.Priority.I
import org.amnezia.vpn.util.Log.Priority.V
import org.amnezia.vpn.util.Log.Priority.W
import android.util.Log as NativeLog

private const val TAG = "Log"
private const val APP_LOG_FILE_NAME = "app.log"
private const val APP_ROTATE_LOG_FILE_NAME = "app.rotate.log"
private const val TUNNEL_LOG_FILE_NAME = "tunnel.log"
private const val TUNNEL_ROTATE_LOG_FILE_NAME = "tunnel.rotate.log"
private const val LEGACY_LOG_FILE_NAME = "amneziaVPN.log"
private const val LEGACY_ROTATE_LOG_FILE_NAME = "amneziaVPN.rotate.log"
private const val LOCK_FILE_NAME = ".lock"
private const val TILE_SERVICE_PROCESS_SUFFIX = ":amneziaTileService"
private const val AWG_SERVICE_PROCESS_SUFFIX = ":amneziaAwgService"
private const val AWG_LOGCAT_TAG_PREFIX = " AmneziaWG/"
private const val STREAM_APP = 0
private const val STREAM_TUNNEL = 1
private const val DATE_TIME_PATTERN = "MM-dd HH:mm:ss.SSS"
private const val PREFS_SAVE_LOGS_KEY = "SAVE_LOGS"
private const val LOG_MAX_FILE_SIZE = 1024 * 1024

/**
 * | Priority          | Save to file | Logcat logging                               |
 * |-------------------|--------------|----------------------------------------------|
 * | Verbose           | Don't save   | Only in Debug build                          |
 * | Debug             | Save         | In Debug build or if log saving is enabled   |
 * | Info, Warn, Error | Save         | Enabled                                      |
 * | Fatal (Assert)    | Save         | Enabled. Depending on system configuration,  |
 * |                   |              | create a report and/or terminate the process |
 */
object Log {
    private val dateTimeFormat: DateTimeFormatter = DateTimeFormatter.ofPattern(DATE_TIME_PATTERN)

    private lateinit var logDir: File
    private lateinit var logFile: File
    private lateinit var rotateLogFile: File

    private val fileLock: FileChannel by lazy { RandomAccessFile(File(logDir, LOCK_FILE_NAME).path, "rw").channel }
    private val threadLock: ReentrantLock by lazy { ReentrantLock() }

    private var isAwgProcess: Boolean = false
    private val awgLogcatLock = Any()
    private var awgLogcatReader: AwgLogcatReader? = null

    @Volatile
    private var _saveLogs: Boolean = false
    var saveLogs: Boolean
        get() = _saveLogs
        set(value) {
            if (_saveLogs != value) {
                if (value && !logDir.exists() && !logDir.mkdir()) {
                    NativeLog.e(TAG, "Failed to create dir: $logDir")
                    return
                }
                _saveLogs = value
                Prefs.save(PREFS_SAVE_LOGS_KEY, value)
                if (isAwgProcess) {
                    if (value) startAwgLogcatReader() else stopAwgLogcatReader()
                }
            }
        }

    @JvmStatic
    fun v(tag: String, msg: String) = log(tag, msg, V)

    @JvmStatic
    fun d(tag: String, msg: String) = log(tag, msg, D)

    @JvmStatic
    fun i(tag: String, msg: String) = log(tag, msg, I)

    @JvmStatic
    fun w(tag: String, msg: String) = log(tag, msg, W)

    @JvmStatic
    fun e(tag: String, msg: String) = log(tag, msg, E)

    @JvmStatic
    fun f(tag: String, msg: String) = log(tag, msg, F)

    fun v(tag: String, msg: Any?) = v(tag, msg.toString())

    fun d(tag: String, msg: Any?) = d(tag, msg.toString())

    fun i(tag: String, msg: Any?) = i(tag, msg.toString())

    fun w(tag: String, msg: Any?) = w(tag, msg.toString())

    fun e(tag: String, msg: Any?) = e(tag, msg.toString())

    fun f(tag: String, msg: Any?) = f(tag, msg.toString())

    fun init(context: Context) {
        v(TAG, "Init Log")
        logDir = File(context.cacheDir, "logs")
        val processName = Application.getProcessName()
        val stream = if (':' in processName && !processName.endsWith(TILE_SERVICE_PROCESS_SUFFIX)) {
            STREAM_TUNNEL
        } else {
            STREAM_APP
        }
        val (rotate, current) = streamFiles(stream)
        rotateLogFile = rotate
        logFile = current
        isAwgProcess = processName.endsWith(AWG_SERVICE_PROCESS_SUFFIX)
        saveLogs = Prefs.load(PREFS_SAVE_LOGS_KEY)
    }

    @JvmStatic
    fun getLogFiles(stream: Int): String {
        val files = streamFiles(stream).toMutableList()
        if (stream == STREAM_APP) files.addAll(0, legacyFiles().filter { it.exists() })
        return files.joinToString("\n") { it.absolutePath }
    }

    @JvmStatic
    fun getDeviceInfo(): String {
        val sb = StringBuilder()
        sb.append("Model: ").appendLine(Build.MODEL)
        sb.append("Brand: ").appendLine(Build.BRAND)
        sb.append("Product: ").appendLine(Build.PRODUCT)
        sb.append("Device: ").appendLine(Build.DEVICE)
        sb.append("Codename: ").appendLine(Build.VERSION.CODENAME)
        sb.append("Release: ").appendLine(Build.VERSION.RELEASE)
        sb.append("SDK: ").appendLine(Build.VERSION.SDK_INT)
        sb.append("ABI: ").appendLine(Build.SUPPORTED_ABIS.joinToString())
        return sb.toString()
    }

    @JvmStatic
    fun clearStream(stream: Int) {
        if (logDir.exists()) {
            withLock {
                streamFiles(stream).forEach { it.delete() }
                if (stream == STREAM_APP) legacyFiles().forEach { it.delete() }
            }
        }
    }

    private fun streamFiles(stream: Int): List<File> =
        if (stream == STREAM_TUNNEL) {
            listOf(File(logDir, TUNNEL_ROTATE_LOG_FILE_NAME), File(logDir, TUNNEL_LOG_FILE_NAME))
        } else {
            listOf(File(logDir, APP_ROTATE_LOG_FILE_NAME), File(logDir, APP_LOG_FILE_NAME))
        }

    private fun legacyFiles(): List<File> =
        listOf(File(logDir, LEGACY_ROTATE_LOG_FILE_NAME), File(logDir, LEGACY_LOG_FILE_NAME))

    private fun log(tag: String, msg: String, priority: Priority) {
        if (saveLogs && priority != V) saveLogMsg(formatLogMsg(tag, msg, priority))

        if (priority == F) {
            NativeLog.wtf(tag, msg)
        } else if (
            (priority != V && priority != D) ||
            (priority == V && BuildConfig.DEBUG) ||
            (priority == D && (BuildConfig.DEBUG || saveLogs))
        ) {
            NativeLog.println(priority.level, tag, msg)
        }
    }

    private fun saveLogMsg(msg: String) {
        withTryLock(condition = { logFile.length() > LOG_MAX_FILE_SIZE }) {
            logFile.renameTo(rotateLogFile)
        }
        try {
            logFile.appendText(msg)
        } catch (e: IOException) {
            NativeLog.e(TAG, "Failed to write log: $e")
        }
    }

    private fun startAwgLogcatReader() {
        synchronized(awgLogcatLock) {
            if (awgLogcatReader == null) {
                awgLogcatReader = AwgLogcatReader().also { it.start() }
            }
        }
    }

    private fun stopAwgLogcatReader() {
        synchronized(awgLogcatLock) {
            awgLogcatReader?.shutdown()
            awgLogcatReader = null
        }
    }

    private class AwgLogcatReader : Thread("AwgLogcatReader") {
        private val lock = Any()
        private var process: java.lang.Process? = null
        @Volatile
        private var stopped = false

        init {
            isDaemon = true
        }

        override fun run() {
            try {
                follow()
            } finally {
                synchronized(awgLogcatLock) {
                    if (awgLogcatReader === this) awgLogcatReader = null
                }
            }
        }

        private fun follow() {
            val logcat = try {
                ProcessBuilder(
                    "logcat", "-v", "threadtime", "-v", "UTC", "--pid=${Process.myPid()}", "-T", "1"
                ).redirectErrorStream(true).start()
            } catch (e: IOException) {
                NativeLog.e(TAG, "Failed to start logcat: $e")
                return
            }
            synchronized(lock) {
                if (stopped) {
                    logcat.destroy()
                    return
                }
                process = logcat
            }
            try {
                logcat.inputStream.bufferedReader().useLines { lines ->
                    lines.filter { it.contains(AWG_LOGCAT_TAG_PREFIX) }.forEach {
                        if (!stopped) saveLogMsg("$it\n")
                    }
                }
                if (!stopped) NativeLog.w(TAG, "Logcat exited unexpectedly")
            } catch (e: IOException) {
                if (!stopped) NativeLog.e(TAG, "Failed to read logcat: $e")
            } finally {
                logcat.destroy()
            }
        }

        fun shutdown() {
            synchronized(lock) {
                stopped = true
                process?.destroy()
            }
        }
    }

    private fun formatLogMsg(tag: String, msg: String, priority: Priority): String {
        val utcDate = ZonedDateTime.now(ZoneOffset.UTC).format(dateTimeFormat)
        return "${utcDate}Z ${Process.myPid()} ${Process.myTid()} $priority [${Thread.currentThread().name}] " +
            "$tag: $msg\n"
    }

    private fun withLock(block: () -> Unit) {
        threadLock.lock()
        try {
            var l: FileLock? = null
            try {
                l = fileLock.lock()
                block()
            } catch (e: IOException) {
                NativeLog.e(TAG, "Failed to get file lock: $e")
            } finally {
                try {
                    l?.release()
                } catch (e: IOException) {
                    NativeLog.e(TAG, "Failed to release file lock: $e")
                }
            }
        } finally {
            threadLock.unlock()
        }
    }

    private fun withTryLock(condition: () -> Boolean, block: () -> Unit) {
        if (condition()) {
            if (threadLock.tryLock()) {
                try {
                    if (condition()) {
                        var l: FileLock? = null
                        try {
                            l = fileLock.tryLock()
                            if (l != null) {
                                if (condition()) {
                                    block()
                                }
                            }
                        } catch (e: IOException) {
                            NativeLog.e(TAG, "Failed to get file tryLock: $e")
                        } finally {
                            try {
                                l?.release()
                            } catch (e: IOException) {
                                NativeLog.e(TAG, "Failed to release file tryLock: $e")
                            }
                        }
                    }
                } finally {
                    threadLock.unlock()
                }
            }
        }
    }

    private enum class Priority(val level: Int) {
        V(2),
        D(3),
        I(4),
        W(5),
        E(6),
        F(7)
    }
}
