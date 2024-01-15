package org.amnezia.vpn.util

import android.util.Log as NativeLog

class Log {
    companion object {
        fun v(tag: String, msg: String) = debugLog(tag, msg, NativeLog::v)

        fun d(tag: String, msg: String) = debugLog(tag, msg, NativeLog::d)

        fun i(tag: String, msg: String) = log(tag, msg, NativeLog::i)

        fun w(tag: String, msg: String) = log(tag, msg, NativeLog::w)

        fun e(tag: String, msg: String) = log(tag, msg, NativeLog::e)

        fun v(tag: String, msg: Any?) = v(tag, msg.toString())

        fun d(tag: String, msg: Any?) = d(tag, msg.toString())

        fun i(tag: String, msg: Any?) = i(tag, msg.toString())

        fun w(tag: String, msg: Any?) = w(tag, msg.toString())

        fun e(tag: String, msg: Any?) = e(tag, msg.toString())

        private inline fun log(tag: String, msg: String, delegate: (String, String) -> Unit) = delegate(tag, msg)

        private inline fun debugLog(tag: String, msg: String, delegate: (String, String) -> Unit) {
            if (BuildConfig.DEBUG) delegate(tag, msg)
        }
    }
}
