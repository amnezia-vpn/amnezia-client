package org.amnezia.vpn.util

import org.json.JSONArray

inline fun <reified T> JSONArray.asSequence(): Sequence<T> =
    (0..<length()).asSequence().map { get(it) as T }
