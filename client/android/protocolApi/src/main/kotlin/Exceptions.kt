package org.amnezia.vpn.protocol

sealed class ProtocolException(message: String? = null, cause: Throwable? = null) : Exception(message, cause)

class LoadLibraryException(message: String? = null, cause: Throwable? = null) : ProtocolException(message, cause)
class VpnNotAuthorizedException(message: String? = null, cause: Throwable? = null) : ProtocolException(message, cause)
class BadConfigException(message: String? = null, cause: Throwable? = null) : ProtocolException(message, cause)

class VpnStartException(message: String? = null, cause: Throwable? = null) : ProtocolException(message, cause)
