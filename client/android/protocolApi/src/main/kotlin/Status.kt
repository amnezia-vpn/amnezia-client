package org.amnezia.vpn.protocol

import android.os.Bundle

private const val IS_CONNECTED_KEY = "isConnected"

@Suppress("DataClassPrivateConstructor")
data class Status private constructor(
    val isConnected: Boolean = false
) {
    private constructor(builder: Builder) : this(builder.isConnected)

    class Builder {
        var isConnected: Boolean = false
            private set

        fun setConnected(isConnected: Boolean) = apply { this.isConnected = isConnected }

        fun build(): Status = Status(this)
    }

    companion object {
        inline fun build(block: Builder.() -> Unit): Status = Builder().apply(block).build()
    }
}

fun Bundle.putStatus(statistics: Status) {
    putBoolean(IS_CONNECTED_KEY, statistics.isConnected)
}

fun Bundle.getStatus(): Status =
    Status.build {
        setConnected(getBoolean(IS_CONNECTED_KEY))
    }
