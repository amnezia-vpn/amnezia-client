package org.amnezia.vpn.ikev2

enum class VpnType(
    /**
     * The identifier used to store this value in the database
     * @return identifier
     */
    val identifier: String, features: java.util.EnumSet<VpnTypeFeature>
) {
    /* the order here must match the items in R.array.vpn_types */
    IKEV2_EAP("ikev2-eap", java.util.EnumSet.of(VpnTypeFeature.USER_PASS)), IKEV2_CERT(
        "ikev2-cert", java.util.EnumSet.of(
            VpnTypeFeature.CERTIFICATE
        )
    ),
    IKEV2_CERT_EAP(
        "ikev2-cert-eap",
        java.util.EnumSet.of(VpnTypeFeature.USER_PASS, VpnTypeFeature.CERTIFICATE)
    ),
    IKEV2_EAP_TLS(
        "ikev2-eap-tls", java.util.EnumSet.of(
            VpnTypeFeature.CERTIFICATE
        )
    ),
    IKEV2_BYOD_EAP("ikev2-byod-eap", java.util.EnumSet.of(VpnTypeFeature.USER_PASS, VpnTypeFeature.BYOD));

    /**
     * Features of a VPN type.
     */
    enum class VpnTypeFeature {
        /** client certificate is required  */
        CERTIFICATE,

        /** username and password are required  */
        USER_PASS,

        /** enable BYOD features  */
        BYOD
    }

    private val mFeatures: java.util.EnumSet<VpnTypeFeature>

    /**
     * Enum which provides additional information about the supported VPN types.
     *
     * @param id identifier used to store and transmit this specific type
     * @param features of the given VPN type
     * @param certificate true if a client certificate is required
     */
    init {
        mFeatures = features
    }

    /**
     * Checks whether a feature is supported/required by this type of VPN.
     *
     * @return true if the feature is supported/required
     */
    fun has(feature: VpnTypeFeature?): Boolean {
        return mFeatures.contains(feature)
    }

    companion object {
        /**
         * Get the enum entry with the given identifier.
         *
         * @param identifier get the enum entry with this identifier
         * @return the enum entry, or the default if not found
         */
        fun fromIdentifier(identifier: String): VpnType {
            for (type in VpnType.values()) {
                if (identifier == type.identifier) {
                    return type
                }
            }
            return IKEV2_EAP
        }
    }
}