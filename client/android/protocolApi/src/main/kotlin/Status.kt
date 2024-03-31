package org.amnezia.vpn.protocol

import android.os.Bundle

private const val STATE_KEY = "state"

@Suppress("DataClassPrivateConstructor")
data class Status private constructor(
    val state: ProtocolState
) {
    private constructor(builder: Builder) : this(builder.state)

    class Builder {
        lateinit var state: ProtocolState
            private set

        fun setState(state: ProtocolState) = apply { this.state = state }

        fun build(): Status = Status(this)
    }

    companion object {
        inline fun build(block: Builder.() -> Unit): Status = Builder().apply(block).build()
    }
}

fun Bundle.putStatus(status: Status) {
    putInt(STATE_KEY, status.state.ordinal)
}

fun Bundle.putStatus(state: ProtocolState) {
    putInt(STATE_KEY, state.ordinal)
}

fun Bundle.getStatus(): Status =
    Status.build {
        setState(ProtocolState.entries[getInt(STATE_KEY)])
    }
