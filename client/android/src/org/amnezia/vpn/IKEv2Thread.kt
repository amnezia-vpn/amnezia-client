package org.amnezia.vpn

import android.annotation.TargetApi
import android.content.pm.PackageManager
import android.net.VpnService
import android.net.VpnService.Builder
import android.os.Build
import android.os.ParcelFileDescriptor
import android.system.OsConstants

import java.lang.Runnable

import org.amnezia.vpn.ikev2.VpnProfile
import org.amnezia.vpn.ikev2.VpnProfile.SelectedAppsHandling
import org.amnezia.vpn.ikev2.VpnType
import org.amnezia.vpn.ikev2.VpnType.VpnTypeFeature
import org.amnezia.vpn.ikev2.utils.IPRange
import org.amnezia.vpn.ikev2.utils.IPRangeSet
import org.amnezia.vpn.ikev2.utils.Utils
import org.amnezia.vpn.ikev2.utils.Constants
import org.amnezia.vpn.ikev2.utils.SettingsWriter
import org.amnezia.vpn.ikev2.utils.SimpleFetcher

const val PACKAGE_NAME = "org.amnezia.vpn"
const val LOG_FILE = "charon.log"
const val TAG = "amnezia_ikev2"

class IKEv2Thread(
    val vpnServiceBuilder: android.net.VpnService.Builder,
    val filesDirAbsolutePath: String
): Runnable {

    private val mBuilderAdapter: BuilderAdapter = BuilderAdapter(vpnServiceBuilder)

    private var mCurrentProfile: VpnProfile? = null
    private var mNextProfile: VpnProfile? = null

    @kotlin.jvm.Volatile
    private var mCurrentCertificateAlias: String? = null

    @kotlin.jvm.Volatile
    private var mCurrentUserCertificateAlias: String? = null

    @kotlin.jvm.Volatile
    private var mProfileUpdated = false

    @kotlin.jvm.Volatile
    private var mTerminate = false

    @kotlin.jvm.Volatile
    private var mIsDisconnecting = false

    private val mLogFile: String
    private val mAppDir: String

    init {
        mLogFile = filesDirAbsolutePath + java.io.File.separator + LOG_FILE
        mAppDir = filesDirAbsolutePath
    }

    override fun run() {
        while (true) {
            synchronized(this) {
                try {
                    while (!mProfileUpdated) {
                        continue
                    }
                    mProfileUpdated = false
                    stopCurrentConnection()

                    if (mNextProfile == null) {
                        setState(State.DISABLED)
                        if (mTerminate) {
                            return@synchronized
                        }
                    } else {
                        mCurrentProfile = mNextProfile
                        mNextProfile = null

                        val currentProfile = mCurrentProfile

                        /* store this in a separate (volatile) variable to avoid
                         * a possible deadlock during deinitialization */
                        mCurrentCertificateAlias = currentProfile!!.certificateAlias
                        mCurrentUserCertificateAlias = currentProfile!!.userCertificateAlias

                        startConnection(currentProfile)

                        mIsDisconnecting = false
                        SimpleFetcher.enable()
                        addNotification()
                        mBuilderAdapter.setProfile(currentProfile)

                        val flags: Int = currentProfile!!.flags

                        if (initializeCharon(
                                mBuilderAdapter,
                                mLogFile,
                                mAppDir,
                                currentProfile.vpnType!!.has(VpnTypeFeature.BYOD),
                                flags and VpnProfile.FLAGS_IPv6_TRANSPORT !== 0
                            )
                        ) {
                            Log.i(TAG, "charon started")

                            if (currentProfile.vpnType!!.has(VpnTypeFeature.USER_PASS) &&
                                currentProfile.password == null
                            ) {    /* this can happen if Always-on VPN is enabled with an incomplete profile */
                                setError(ErrorState.PASSWORD_MISSING)
                                return@synchronized
                            }

                            val writer = SettingsWriter()
                            writer.setValue("global.language", java.util.Locale.getDefault().getLanguage())
                            writer.setValue("global.mtu", currentProfile.MTU)
                            writer.setValue("global.nat_keepalive", currentProfile.NATKeepAlive)
                            writer.setValue("global.rsa_pss", flags and VpnProfile.FLAGS_RSA_PSS !== 0)
                            writer.setValue("global.crl", flags and VpnProfile.FLAGS_DISABLE_CRL === 0)
                            writer.setValue("global.ocsp", flags and VpnProfile.FLAGS_DISABLE_OCSP === 0)
                            writer.setValue("connection.type", currentProfile.vpnType!!.identifier)
                            writer.setValue("connection.server", currentProfile.gateway)
                            writer.setValue("connection.port", currentProfile.port)
                            writer.setValue("connection.username", currentProfile.username)
                            writer.setValue("connection.password", currentProfile.password)
                            writer.setValue("connection.local_id", currentProfile.localId)
                            writer.setValue("connection.remote_id", currentProfile.remoteId)
                            writer.setValue("connection.certreq", flags and VpnProfile.FLAGS_SUPPRESS_CERT_REQS === 0)
                            writer.setValue("connection.strict_revocation", flags and VpnProfile.FLAGS_STRICT_REVOCATION !== 0)
                            writer.setValue("connection.ike_proposal", currentProfile.ikeProposal)
                            writer.setValue("connection.esp_proposal", currentProfile.espProposal)
                            initiate(writer.serialize())
                        } else {
                            Log.e(TAG, "failed to start charon")
                            setError(ErrorState.GENERIC_ERROR)
                            setState(State.DISABLED)
                            mCurrentProfile = null
                        }
                    }
                } catch (ex: java.lang.InterruptedException) {
                    stopCurrentConnection()
                    setState(State.DISABLED)
                }
            }
        }
    }

    /**
     * Notify the state service about a new connection attempt.
     * Called by the handler thread.
     *
     * @param profile currently active VPN profile
     */
    private fun startConnection(profile: VpnProfile) {
//        synchronized(mServiceLock) {
//            if (mService != null) {
//                mService.startConnection(profile)
//            }
//        }
    }

    /**
     * Stop any existing connection by deinitializing charon.
     */
    private fun stopCurrentConnection() {
        synchronized(this) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                mNextProfile?.let {
                    mBuilderAdapter.setProfile(it)
                    mBuilderAdapter.establishBlocking()
                }
            }

            if (mCurrentProfile != null) {
                setState(State.DISCONNECTING)
                mIsDisconnecting = true
                SimpleFetcher.disable()
                deinitializeCharon()
                Log.i(TAG, "charon stopped")
                mCurrentProfile = null
                if (mNextProfile == null) {    /* only do this if we are not connecting to another profile */
                    removeNotification()
                    mBuilderAdapter.closeBlocking()
                }
            }
        }
    }

    /**
     * Update the current VPN state on the state service. Called by the handler
     * thread and any of charon's threads.
     *
     * @param state current state
     */
    private fun setState(state: State) {
//        synchronized(mServiceLock) {
//            if (mService != null) {
//                mService.setState(state)
//            }
//        }
    }

    /**
     * Initialization of charon, provided by libandroidbridge.so
     *
     * @param builder BuilderAdapter for this connection
     * @param logfile absolute path to the logfile
     * @param appdir absolute path to the data directory of the app
     * @param byod enable BYOD features
     * @param ipv6 enable IPv6 transport
     * @return TRUE if initialization was successful
     */
    external fun initializeCharon(
        builder: BuilderAdapter?,
        logfile: String?,
        appdir: String?,
        byod: Boolean,
        ipv6: Boolean
    ): Boolean

    /**
     * Deinitialize charon, provided by libandroidbridge.so
     */
    external fun deinitializeCharon()

    /**
     * Initiate VPN, provided by libandroidbridge.so
     */
    external fun initiate(config: String?)

    /**
     * Adapter for VpnService.Builder which is used to access it safely via JNI.
     * There is a corresponding C object to access it from native code.
     */
    class BuilderAdapter(val builder: VpnService.Builder) {
        private lateinit var mProfile: VpnProfile
        private lateinit var mBuilder: VpnService.Builder
        private lateinit var mCache: BuilderCache
        private lateinit var mEstablishedCache: BuilderCache
        private val mDropper: PacketDropper = PacketDropper()

        init {
            mBuilder = builder
        }

        @kotlin.jvm.Synchronized
        fun setProfile(profile: VpnProfile) {
            mProfile = profile
            mBuilder = createBuilder(mProfile.name ?: "")
            mCache = BuilderCache(mProfile)
        }

        private fun createBuilder(name: String): VpnService.Builder {
//            val builder: VpnService.Builder = Builder()

            mBuilder.setSession(name)

            /* even though the option displayed in the system dialog says "Configure"
			 * we just use our main Activity */
//            val context: Context = getApplicationContext()
//            val intent = Intent(context, MainActivity::class.java)
//            var flags: Int = PendingIntent.FLAG_UPDATE_CURRENT
//            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
//                flags = flags or PendingIntent.FLAG_IMMUTABLE
//            }
//            val pending: PendingIntent = PendingIntent.getActivity(context, 0, intent, flags)
//            builder.setConfigureIntent(pending)

            /* mark all VPN connections as unmetered (default changed for Android 10) */
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                mBuilder.setMetered(false)
            }
            return mBuilder
        }

        @kotlin.jvm.Synchronized
        fun addAddress(address: String?, prefixLength: Int): Boolean {
            try {
                mCache.addAddress(address, prefixLength)
            } catch (ex: java.lang.IllegalArgumentException) {
                return false
            }
            return true
        }

        @kotlin.jvm.Synchronized
        fun addDnsServer(address: String?): Boolean {
            try {
                mCache.addDnsServer(address)
            } catch (ex: java.lang.IllegalArgumentException) {
                return false
            }
            return true
        }

        @kotlin.jvm.Synchronized
        fun addRoute(address: String?, prefixLength: Int): Boolean {
            try {
                mCache.addRoute(address, prefixLength)
            } catch (ex: java.lang.IllegalArgumentException) {
                return false
            }
            return true
        }

        @kotlin.jvm.Synchronized
        fun addSearchDomain(domain: String): Boolean {
            try {
                mBuilder.addSearchDomain(domain)
            } catch (ex: java.lang.IllegalArgumentException) {
                return false
            }
            return true
        }

        @kotlin.jvm.Synchronized
        fun setMtu(mtu: Int): Boolean {
            try {
                mCache.setMtu(mtu)
            } catch (ex: java.lang.IllegalArgumentException) {
                return false
            }
            return true
        }

        @kotlin.jvm.Synchronized
        private fun establishIntern(): ParcelFileDescriptor? {
            val fd: ParcelFileDescriptor?
            try {
                mCache.applyData(mBuilder)
                fd = mBuilder.establish()
                if (fd != null) {
                    closeBlocking()
                }
            } catch (ex: java.lang.Exception) {
                ex.printStackTrace()
                return null
            }
            if (fd == null) {
                return null
            }
            /* now that the TUN device is created we don't need the current
			 * builder anymore, but we might need another when reestablishing */
            mBuilder = createBuilder(mProfile.name ?: "")
            mEstablishedCache = mCache
            mCache = BuilderCache(mProfile)
            return fd
        }

        @kotlin.jvm.Synchronized
        fun establish(): Int {
            val fd: ParcelFileDescriptor? = establishIntern()
            return if (fd != null) fd.detachFd() else -1
        }

        @TargetApi(Build.VERSION_CODES.LOLLIPOP)
        @kotlin.jvm.Synchronized
        fun establishBlocking() {
            /* just choose some arbitrary values to block all traffic (except for what's configured in the profile) */
            mCache.addAddress("172.16.252.1", 32)
            mCache.addAddress("fd00::fd02:1", 128)
            mCache.addRoute("0.0.0.0", 0)
            mCache.addRoute("::", 0)
            /* set DNS servers to avoid DNS leak later */
            mBuilder.addDnsServer("8.8.8.8")
            mBuilder.addDnsServer("2001:4860:4860::8888")
            /* use blocking mode to simplify packet dropping */
            mBuilder.setBlocking(true)
            val fd: ParcelFileDescriptor? = establishIntern()
            if (fd != null) {
                mDropper.start(fd)
            }
        }

        @kotlin.jvm.Synchronized
        fun closeBlocking() {
            mDropper.stop()
        }

        @kotlin.jvm.Synchronized
        fun establishNoDns(): Int {
            val fd: ParcelFileDescriptor?
            if (mEstablishedCache == null) {
                return -1
            }
            fd = try {
                val builder: VpnService.Builder = createBuilder(mProfile.name ?: "")
                mEstablishedCache.applyData(builder)
                builder.establish()
            } catch (ex: java.lang.Exception) {
                ex.printStackTrace()
                return -1
            }
            return if (fd == null) {
                -1
            } else fd.detachFd()
        }

        private inner class PacketDropper : java.lang.Runnable {
            private var mFd: ParcelFileDescriptor? = null
            private lateinit var mThread: java.lang.Thread

            fun start(fd: ParcelFileDescriptor) {
                mFd = fd
                mThread = java.lang.Thread(this)
                mThread.start()
            }

            fun stop() {
                if (mFd != null) {
                    try {
                        mThread.interrupt()
                        mThread.join()
                        mFd?.close()
                    } catch (e: java.lang.InterruptedException) {
                        e.printStackTrace()
                    } catch (e: java.io.IOException) {
                        e.printStackTrace()
                    }
                    mFd = null
                }
            }

            @kotlin.jvm.Synchronized
            override fun run() {
                try {
                    val plain: java.io.FileInputStream = java.io.FileInputStream(mFd?.getFileDescriptor())
                    val packet: java.nio.ByteBuffer = java.nio.ByteBuffer.allocate(mCache.mMtu)
                    while (true) {
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {    /* just read and ignore all data, regular read() is not interruptible */
                            val len: Int = plain.getChannel().read(packet)
                            packet.clear()
                            if (len < 0) {
                                break
                            }
                        } else {    /* this is rather ugly but on older platforms not even the NIO version of read() is interruptible */
                            var wait = true
                            if (plain.available() > 0) {
                                val len: Int = plain.read(packet.array())
                                packet.clear()
                                if (len < 0 || java.lang.Thread.interrupted()) {
                                    break
                                }
                                /* check again right away, there may be another packet */wait = false
                            }
                            if (wait) {
                                java.lang.Thread.sleep(250)
                            }
                        }
                    }
                } catch (e: java.nio.channels.ClosedByInterruptException) {
                    /* regular interruption */
                } catch (e: java.lang.InterruptedException) {
                } catch (e: java.io.IOException) {
                    e.printStackTrace()
                }
            }
        }
    }

    /**
     * Cache non DNS related information so we can recreate the builder without
     * that information when reestablishing IKE_SAs
     */
    class BuilderCache(profile: VpnProfile) {
        val mAddresses: MutableList<IPRange> = java.util.ArrayList<IPRange>()
        val mRoutesIPv4: MutableList<IPRange> = java.util.ArrayList<IPRange>()
        val mRoutesIPv6: MutableList<IPRange> = java.util.ArrayList<IPRange>()
        val mIncludedSubnetsv4: IPRangeSet = IPRangeSet()
        val mIncludedSubnetsv6: IPRangeSet = IPRangeSet()
        val mExcludedSubnets: IPRangeSet
        val mSplitTunneling: Int
        val mAppHandling: SelectedAppsHandling
        val mSelectedApps: java.util.SortedSet<String>
        val mDnsServers: MutableList<java.net.InetAddress> = java.util.ArrayList<java.net.InetAddress>()
        var mMtu: Int
        var mIPv4Seen = false
        var mIPv6Seen = false
        var mDnsServersConfigured = false

        init {
            val included: IPRangeSet = IPRangeSet.fromString(profile?.includedSubnets)
            for (range in included) {
                if (range.getFrom() is java.net.Inet4Address) {
                    mIncludedSubnetsv4.add(range)
                } else if (range.getFrom() is java.net.Inet6Address) {
                    mIncludedSubnetsv6.add(range)
                }
            }
            mExcludedSubnets = IPRangeSet.fromString(profile.excludedSubnets ?: "")
            val splitTunneling: Int? = profile.splitTunneling
            mSplitTunneling = splitTunneling ?: 0
            var appHandling: SelectedAppsHandling = profile.selectedAppsHandling
            mSelectedApps = profile.selectedAppsSet

            when (appHandling) {
                SelectedAppsHandling.SELECTED_APPS_DISABLE -> {
                    appHandling = SelectedAppsHandling.SELECTED_APPS_EXCLUDE
                    mSelectedApps.clear()
                    mSelectedApps.add(PACKAGE_NAME)
                }

                SelectedAppsHandling.SELECTED_APPS_EXCLUDE -> {
                    mSelectedApps.add(PACKAGE_NAME)
                }

                SelectedAppsHandling.SELECTED_APPS_ONLY -> {
                    mSelectedApps.remove(PACKAGE_NAME)
                }
            }
            mAppHandling = appHandling

            val dnsServers = profile.dnsServers
            if (dnsServers != null) {
                for (server in dnsServers.split("\\s+")) {
                    try {
                        mDnsServers.add(Utils.parseInetAddress(server))
                        recordAddressFamily(server)
                        mDnsServersConfigured = true
                    } catch (e: java.net.UnknownHostException) {
                        e.printStackTrace()
                    }
                }
            }

            /* set a default MTU, will be set by the daemon for regular interfaces */
            val mtu: Int? = profile.MTU
            mMtu = mtu ?: Constants.MTU_MAX
        }

        fun addAddress(address: String?, prefixLength: Int) {
            try {
                mAddresses.add(IPRange(address, prefixLength))
                recordAddressFamily(address)
            } catch (ex: java.net.UnknownHostException) {
                ex.printStackTrace()
            }
        }

        fun addDnsServer(address: String?) {
            /* ignore received DNS servers if any were configured */
            if (mDnsServersConfigured) {
                return
            }
            try {
                mDnsServers.add(Utils.parseInetAddress(address))
                recordAddressFamily(address)
            } catch (e: java.net.UnknownHostException) {
                e.printStackTrace()
            }
        }

        fun addRoute(address: String?, prefixLength: Int) {
            try {
                if (isIPv6(address)) {
                    mRoutesIPv6.add(IPRange(address, prefixLength))
                } else {
                    mRoutesIPv4.add(IPRange(address, prefixLength))
                }
            } catch (ex: java.net.UnknownHostException) {
                ex.printStackTrace()
            }
        }

        fun setMtu(mtu: Int) {
            mMtu = mtu
        }

        fun recordAddressFamily(address: String?) {
            try {
                if (isIPv6(address)) {
                    mIPv6Seen = true
                } else {
                    mIPv4Seen = true
                }
            } catch (ex: java.net.UnknownHostException) {
                ex.printStackTrace()
            }
        }

        @TargetApi(Build.VERSION_CODES.LOLLIPOP)
        fun applyData(builder: VpnService.Builder) {
            for (address in mAddresses) {
                builder.addAddress(address.getFrom(), address.getPrefix())
            }
            for (server in mDnsServers) {
                builder.addDnsServer(server)
            }
            /* add routes depending on whether split tunneling is allowed or not,
			 * that is, whether we have to handle and block non-VPN traffic */
            if (mSplitTunneling and VpnProfile.SPLIT_TUNNELING_BLOCK_IPV4 === 0) {
                if (mIPv4Seen) {    /* split tunneling is used depending on the routes and configuration */
                    val ranges = IPRangeSet()
                    if (mIncludedSubnetsv4.size() > 0) {
                        ranges.add(mIncludedSubnetsv4)
                    } else {
                        ranges.addAll(mRoutesIPv4)
                    }
                    ranges.remove(mExcludedSubnets)
                    for (subnet in ranges.subnets()) {
                        try {
                            builder.addRoute(subnet.getFrom(), subnet.getPrefix())
                        } catch (e: java.lang.IllegalArgumentException) {    /* some Android versions don't seem to like multicast addresses here,
							 * ignore it for now */
                            if (!subnet.getFrom().isMulticastAddress()) {
                                throw e
                            }
                        }
                    }
                } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {    /* allow traffic that would otherwise be blocked to bypass the VPN */
                    builder.allowFamily(OsConstants.AF_INET)
                }
            } else if (mIPv4Seen) {    /* only needed if we've seen any addresses.  otherwise, traffic
				 * is blocked by default (we also install no routes in that case) */
                builder.addRoute("0.0.0.0", 0)
            }
            /* same thing for IPv6 */
            if (mSplitTunneling and VpnProfile.SPLIT_TUNNELING_BLOCK_IPV6 === 0) {
                if (mIPv6Seen) {
                    val ranges = IPRangeSet()
                    if (mIncludedSubnetsv6.size() > 0) {
                        ranges.add(mIncludedSubnetsv6)
                    } else {
                        ranges.addAll(mRoutesIPv6)
                    }
                    ranges.remove(mExcludedSubnets)
                    for (subnet in ranges.subnets()) {
                        try {
                            builder.addRoute(subnet.getFrom(), subnet.getPrefix())
                        } catch (e: java.lang.IllegalArgumentException) {
                            if (!subnet.getFrom().isMulticastAddress()) {
                                throw e
                            }
                        }
                    }
                } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                    builder.allowFamily(OsConstants.AF_INET6)
                }
            } else if (mIPv6Seen) {
                builder.addRoute("::", 0)
            }
            /* apply selected applications */
            if (mSelectedApps.size > 0 &&
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP
            ) {
                when (mAppHandling) {
                    SelectedAppsHandling.SELECTED_APPS_EXCLUDE -> for (app in mSelectedApps) {
                        try {
                            builder.addDisallowedApplication(app)
                        } catch (e: PackageManager.NameNotFoundException) {
                            // possible if not configured via GUI or app was uninstalled
                        }
                    }

                    SelectedAppsHandling.SELECTED_APPS_ONLY -> for (app in mSelectedApps) {
                        try {
                            builder.addAllowedApplication(app)
                        } catch (e: PackageManager.NameNotFoundException) {
                            // possible if not configured via GUI or app was uninstalled
                        }
                    }

                    else -> {}
                }
            }
            builder.setMtu(mMtu)
        }

        @kotlin.Throws(java.net.UnknownHostException::class)
        private fun isIPv6(address: String?): Boolean {
            val addr: java.net.InetAddress = Utils.parseInetAddress(address)
            if (addr is java.net.Inet4Address) {
                return false
            } else if (addr is java.net.Inet6Address) {
                return true
            }
            return false
        }
    }

    /**
     * Function called via JNI to determine information about the Android version.
     */
    private fun getAndroidVersion(): String? {
        var version = "Android " + Build.VERSION.RELEASE + " - " + Build.DISPLAY
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            version += "/" + Build.VERSION.SECURITY_PATCH
        }
        return version
    }

    /**
     * Function called via JNI to determine information about the device.
     */
    private fun getDeviceString(): String? {
        return ((Build.MODEL + " - " + Build.BRAND).toString() + "/" + Build.PRODUCT).toString() + "/" + Build.MANUFACTURER
    }

    /**
     * Add a permanent notification while we are connected to avoid the service getting killed by
     * the system when low on memory.
     */
    private fun addNotification() {
//        mHandler.post(object : java.lang.Runnable {
//            fun run() {
//                mShowNotification = true
//                startForeground(
//                    org.strongswan.android.logic.CharonVpnService.VPN_STATE_NOTIFICATION_ID,
//                    buildNotification(false)
//                )
//            }
//        })
    }

    /**
     * Remove the permanent notification.
     */
    private fun removeNotification() {
//        mHandler.post(object : java.lang.Runnable {
//            fun run() {
//                mShowNotification = false
//                stopForeground(true)
//            }
//        })
    }

    /**
     * Set an error on the state service. Called by the handler thread and any
     * of charon's threads.
     *
     * @param error error state
     */
    private fun setError(error: ErrorState) {
//        synchronized(mServiceLock) {
//            if (mService != null) {
//                mService.setError(error)
//            }
//        }
    }

    enum class State {
        DISABLED, CONNECTING, CONNECTED, DISCONNECTING
    }

    enum class ErrorState {
        NO_ERROR, AUTH_FAILED, PEER_AUTH_FAILED, LOOKUP_FAILED, UNREACHABLE, GENERIC_ERROR, PASSWORD_MISSING, CERTIFICATE_UNAVAILABLE
    }
}