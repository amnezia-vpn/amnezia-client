/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.amnezia.vpn
import android.os.Binder
import android.os.DeadObjectException
import android.os.IBinder
import android.os.Parcel
import com.wireguard.config.*
import org.json.JSONObject
import java.lang.Exception

class VPNServiceBinder(service: VPNService) : Binder() {

    private val mService = service
    private val tag = "VPNServiceBinder"
    private var mListener: IBinder? = null
    private var mResumeConfig: JSONObject? = null
    private var mImportedConfig: String? = null

    /**
    * The codes this Binder does accept in [onTransact]
    */
    object ACTIONS {
        const val activate = 1
        const val deactivate = 2
        const val registerEventListener = 3
        const val requestStatistic = 4
        const val requestGetLog = 5
        const val requestCleanupLog = 6
        const val resumeActivate = 7
        const val setNotificationText = 8
        const val setFallBackNotification = 9
        const val importConfig = 11
    }

    /**
    * Gets called when the VPNServiceBinder gets a request from a Client.
    * The [code] determines what action is requested. - see [ACTIONS]
    * [data] may contain a utf-8 encoded json string with optional args or is null.
    * [reply] is a pointer to a buffer in the clients memory, to reply results.
    * we use this to send result data.
    *
    * returns true if the [code] was accepted
    */
    override fun onTransact(code: Int, data: Parcel, reply: Parcel?, flags: Int): Boolean {
        Log.i(tag, "GOT TRANSACTION " + code)

        when (code) {
            ACTIONS.activate -> {
                try {
                    Log.i(tag, "Activation Requested, parsing Config")
                    // [data] is here a json containing the wireguard/openvpn conf
                    val buffer = data.createByteArray()
                    val json = buffer?.let { String(it) }
                    val config = JSONObject(json)
                    Log.v(tag, "Stored new Tunnel config in Service")
                    Log.i(tag, "Config: $config")
                    if (!mService.checkPermissions()) {
                        mResumeConfig = config
                        // The Permission prompt was already
                        // send, in case it's accepted we will
                        // receive ACTIONS.resumeActivate
                        return true
                    }
                    this.mService.turnOn(config)
                } catch (e: Exception) {
                    Log.e(tag, "An Error occurred while enabling the VPN: ${e.localizedMessage}")
                    dispatchEvent(EVENTS.activationError, e.localizedMessage)
                }
                return true
            }

            ACTIONS.resumeActivate -> {
            // [data] is empty
            // Activate the current tunnel
                Log.i(tag, "resume activate")
                try {
                    mResumeConfig?.let { this.mService.turnOn(it) }
                } catch (e: Exception) {
                    Log.e(tag, "An Error occurred while enabling the VPN: ${e.localizedMessage}")
                }
                return true
        }

            ACTIONS.deactivate -> {
                // [data] here is empty
                this.mService.turnOff()
                return true
            }

            ACTIONS.registerEventListener -> {
                Log.i(tag, "register: start")
                // [data] contains the Binder that we need to dispatch the Events
                val binder = data.readStrongBinder()
                mListener = binder
                val obj = JSONObject()
                obj.put("connected", mService.isUp)
                obj.put("time", mService.connectionTime)
                dispatchEvent(EVENTS.init, obj.toString())

                ////
                if (mImportedConfig != null) {
                    Log.i(tag, "register: config not null")
                    dispatchEvent(EVENTS.configImport, mImportedConfig)
                    mImportedConfig = null
                } else {
                    Log.i(tag, "register: config is null")
                }

                return true
            }

            ACTIONS.requestStatistic -> {
                dispatchEvent(EVENTS.statisticUpdate, mService.status.toString())
                return true
            }

            ACTIONS.requestGetLog -> {
                // Grabs all the Logs and dispatch new Log Event
                dispatchEvent(EVENTS.backendLogs, Log.getContent())
                return true
            }

            ACTIONS.requestCleanupLog -> {
                Log.clearFile()
                return true
            }

            ACTIONS.setNotificationText -> {
                NotificationUtil.update(data)
                return true
            }

            ACTIONS.setFallBackNotification -> {
                NotificationUtil.saveFallBackMessage(data, mService)
                return true
            }

            ACTIONS.importConfig -> {
                val buffer = data.readString()

                val obj = JSONObject()
                obj.put("config", buffer)

                val resultString = obj.toString()

                Log.i(tag, "Transact import config request")

                if (mListener != null) {
                    dispatchEvent(EVENTS.configImport, resultString)
                } else {
                    mImportedConfig = resultString
                }
            }

            IBinder.LAST_CALL_TRANSACTION -> {
                Log.e(tag, "The OS Requested to shut down the VPN")
                this.mService.turnOff()
                return true
            }

            else -> {
                Log.e(tag, "Received invalid bind request \t Code -> $code")
                // If we're hitting this there is probably something wrong in the client.
                return false
            }
        }

        return false
    }

    /**
    * Dispatches an Event to all registered Binders
    * [code] the Event that happened - see [EVENTS]
    * To register an Eventhandler use [onTransact] with
    * [ACTIONS.registerEventListener]
    */
    fun dispatchEvent(code: Int, payload: String?) {
        try {
            mListener?.let {
                if (it.isBinderAlive) {
                    val data = Parcel.obtain()
                    data.writeByteArray(payload?.toByteArray(charset("UTF-8")))
                    it.transact(code, data, Parcel.obtain(), 0)
                } else {
                    Log.i(tag, "Dispatching event: binder NOT alive")
                }
            }
        } catch (e: DeadObjectException) {
            // If the QT Process is killed (not just inactive)
            // we cant access isBinderAlive, so nothing to do here.
        }
    }

    /**
    *  The codes we Are Using in case of [dispatchEvent]
    */
    object EVENTS {
        const val init = 0
        const val connected = 1
        const val disconnected = 2
        const val statisticUpdate = 3
        const val backendLogs = 4
        const val activationError = 5
        const val permissionRequired = 6
        const val configImport = 7
    }
}
