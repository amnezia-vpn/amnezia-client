package org.amnezia.vpn.util

import org.json.JSONArray
import org.json.JSONObject

inline fun <reified T> JSONArray.asSequence(): Sequence<T> =
    (0..<length()).asSequence().map { get(it) as T }

fun JSONObject.optStringOrNull(name: String) = optString(name).ifEmpty { null }
