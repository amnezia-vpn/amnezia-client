package org.amnezia.vpn.ikev2

import android.text.TextUtils

class VpnProfile : kotlin.Cloneable {
    var name: String? = null
    var gateway: String? = null
    var username: String? = null
    var password: String? = null
    var certificateAlias: String? = null
    var userCertificateAlias: String? = null
    var remoteId: String? = null
    var localId: String? = null
    var excludedSubnets: String? = null
    var includedSubnets: String? = null
    var selectedApps: String? = null
    var ikeProposal: String? = null
    var espProposal: String? = null
    var dnsServers: String? = null
    var MTU: Int? = null
    var port: Int? = null
    var splitTunneling: Int? = null
    var nATKeepAlive: Int? = null
    private var mFlags: Int? = null
    var selectedAppsHandling = SelectedAppsHandling.SELECTED_APPS_DISABLE
    private var mVpnType: VpnType? = null
    private var mUUID: java.util.UUID?
    var id: Long = -1

    enum class SelectedAppsHandling(val value: Int) {
        SELECTED_APPS_DISABLE(0), SELECTED_APPS_EXCLUDE(1), SELECTED_APPS_ONLY(2);

    }

    init {
        mUUID = java.util.UUID.randomUUID()
    }

    var uUID: java.util.UUID?
        get() = mUUID
        set(uuid) {
            mUUID = uuid
        }
    var vpnType: VpnType?
        get() = mVpnType
        set(type) {
            mVpnType = type
        }

    fun setSelectedApps(selectedApps: java.util.SortedSet<String?>) {
        this.selectedApps = if (selectedApps.size > 0) TextUtils.join(" ", selectedApps) else null
    }

    val selectedAppsSet: java.util.SortedSet<String>
        get() {
            val set: java.util.TreeSet<String> = java.util.TreeSet<String>()
            if (!TextUtils.isEmpty(selectedApps)) {
                set.addAll(
                    java.util.Arrays.asList<String>(
                        *selectedApps!!.split("\\s+".toRegex()).dropLastWhile { it.isEmpty() }
                            .toTypedArray()))
            }
            return set
        }

    fun setSelectedAppsHandling(value: Int) {
        selectedAppsHandling = SelectedAppsHandling.SELECTED_APPS_DISABLE
        for (handling in VpnProfile.SelectedAppsHandling.values()) {
            if (handling.value == value) {
                selectedAppsHandling = handling
                break
            }
        }
    }

    var flags: Int?
        get() = if (mFlags == null) 0 else mFlags
        set(flags) {
            mFlags = flags
        }

    override fun toString(): String {
        return name!!
    }

    override fun equals(o: Any?): Boolean {
        if (o != null && o is VpnProfile) {
            val other = o
            return if (mUUID != null && other.uUID != null) {
                mUUID == other.uUID
            } else id == other.id
        }
        return false
    }

    override fun clone(): VpnProfile {
        return try {
            super.clone() as VpnProfile
        } catch (e: java.lang.CloneNotSupportedException) {
            throw java.lang.AssertionError()
        }
    }

    companion object {
        /* While storing this as EnumSet would be nicer this simplifies storing it in a database */
        const val SPLIT_TUNNELING_BLOCK_IPV4 = 1
        const val SPLIT_TUNNELING_BLOCK_IPV6 = 2
        const val FLAGS_SUPPRESS_CERT_REQS = 1 shl 0
        const val FLAGS_DISABLE_CRL = 1 shl 1
        const val FLAGS_DISABLE_OCSP = 1 shl 2
        const val FLAGS_STRICT_REVOCATION = 1 shl 3
        const val FLAGS_RSA_PSS = 1 shl 4
        const val FLAGS_IPv6_TRANSPORT = 1 shl 5
    }
}
