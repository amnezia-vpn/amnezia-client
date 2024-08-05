package org.amnezia.vpn

import android.app.Application
import androidx.datastore.core.CorruptionException
import androidx.datastore.core.MultiProcessDataStoreFactory
import androidx.datastore.core.Serializer
import androidx.datastore.core.handlers.ReplaceFileCorruptionHandler
import androidx.datastore.dataStoreFile
import java.io.InputStream
import java.io.OutputStream
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.catch
import kotlinx.coroutines.flow.firstOrNull
import kotlinx.serialization.ExperimentalSerializationApi
import kotlinx.serialization.Serializable
import kotlinx.serialization.SerializationException
import kotlinx.serialization.decodeFromByteArray
import kotlinx.serialization.encodeToByteArray
import kotlinx.serialization.protobuf.ProtoBuf
import org.amnezia.vpn.protocol.ProtocolState
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTED
import org.amnezia.vpn.util.Log

private const val TAG = "VpnState"
private const val STORE_FILE_NAME = "vpnState"

@Serializable
data class VpnState(
    val protocolState: ProtocolState,
    val serverName: String? = null,
    val serverIndex: Int = -1,
    val vpnProto: VpnProto? = null
) {
    companion object {
        val defaultState: VpnState = VpnState(DISCONNECTED)
    }
}

object VpnStateStore {
    private lateinit var app: Application

    private val dataStore = MultiProcessDataStoreFactory.create(
        serializer = VpnStateSerializer(),
        produceFile = { app.dataStoreFile(STORE_FILE_NAME) },
        corruptionHandler = ReplaceFileCorruptionHandler { e ->
            Log.e(TAG, "VpnState DataStore corrupted: $e")
            VpnState.defaultState
        }
    )

    fun init(app: Application) {
        Log.v(TAG, "Init VpnStateStore")
        this.app = app
    }

    fun dataFlow(): Flow<VpnState> = dataStore.data.catch { e ->
        Log.e(TAG, "Failed to read VpnState from store: ${e.message}")
        emit(VpnState.defaultState)
    }

    suspend fun getVpnState(): VpnState = dataFlow().firstOrNull() ?: VpnState.defaultState

    suspend fun store(f: (vpnState: VpnState) -> VpnState) {
        try {
            dataStore.updateData(f)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to store VpnState: $e")
            Log.w(TAG, "Remove DataStore file")
            app.dataStoreFile(STORE_FILE_NAME).delete()
        }
    }
}

@OptIn(ExperimentalSerializationApi::class)
private class VpnStateSerializer : Serializer<VpnState> {
    override val defaultValue: VpnState = VpnState.defaultState

    override suspend fun readFrom(input: InputStream): VpnState = try {
        ProtoBuf.decodeFromByteArray<VpnState>(input.readBytes())
    } catch (e: SerializationException) {
        Log.e(TAG, "Failed to deserialize data: $e")
        throw CorruptionException("Failed to deserialize data", e)
    }

    @Suppress("BlockingMethodInNonBlockingContext")
    override suspend fun writeTo(t: VpnState, output: OutputStream) =
        output.write(ProtoBuf.encodeToByteArray(t))
}
