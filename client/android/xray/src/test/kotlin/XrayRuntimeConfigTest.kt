package org.amnezia.vpn.protocol.xray

import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Test

class XrayRuntimeConfigTest {

    @Test
    fun `replaces vnext endpoint with IPv4 while preserving TLS and WebSocket identity`() {
        val hostName = "edge.example.com"
        val config = JSONObject(
            """
            {
              "outbounds": [
                {
                  "settings": {
                    "vnext": [
                      {"address": "$hostName", "port": 443}
                    ]
                  },
                  "streamSettings": {
                    "security": "tls",
                    "tlsSettings": {
                      "serverName": "$hostName"
                    },
                    "wsSettings": {
                      "headers": {
                        "Host": "$hostName"
                      },
                      "path": "/$hostName/socket"
                    }
                  }
                }
              ],
              "remark": "connect-via-$hostName"
            }
            """.trimIndent()
        )

        replaceResolvedEndpointAddresses(config, hostName, "203.0.113.10")

        val outbound = config.getJSONArray("outbounds").getJSONObject(0)
        assertEquals(
            "203.0.113.10",
            outbound.getJSONObject("settings")
                .getJSONArray("vnext")
                .getJSONObject(0)
                .getString("address"),
        )
        assertEquals(
            hostName,
            outbound.getJSONObject("streamSettings")
                .getJSONObject("tlsSettings")
                .getString("serverName"),
        )
        assertEquals(
            hostName,
            outbound.getJSONObject("streamSettings")
                .getJSONObject("wsSettings")
                .getJSONObject("headers")
                .getString("Host"),
        )
        assertEquals(
            "/$hostName/socket",
            outbound.getJSONObject("streamSettings")
                .getJSONObject("wsSettings")
                .getString("path"),
        )
        assertEquals("connect-via-$hostName", config.getString("remark"))
    }

    @Test
    fun `replaces servers endpoint with IPv6 while preserving identity fields`() {
        val hostName = "edge.example.com"
        val config = JSONObject(
            """
            {
              "outbounds": [
                {
                  "settings": {
                    "servers": [
                      {"address": "$hostName", "port": 443}
                    ]
                  },
                  "streamSettings": {
                    "tlsSettings": {"serverName": "$hostName"},
                    "wsSettings": {"headers": {"Host": "$hostName"}}
                  }
                }
              ]
            }
            """.trimIndent()
        )

        replaceResolvedEndpointAddresses(config, hostName, "2001:db8::1234")

        val outbound = config.getJSONArray("outbounds").getJSONObject(0)
        assertEquals(
            "2001:db8::1234",
            outbound.getJSONObject("settings")
                .getJSONArray("servers")
                .getJSONObject(0)
                .getString("address"),
        )
        assertEquals(
            hostName,
            outbound.getJSONObject("streamSettings")
                .getJSONObject("tlsSettings")
                .getString("serverName"),
        )
        assertEquals(
            hostName,
            outbound.getJSONObject("streamSettings")
                .getJSONObject("wsSettings")
                .getJSONObject("headers")
                .getString("Host"),
        )
    }

    @Test
    fun `replaces matching addresses across vnext and servers only`() {
        val hostName = "edge.example.com"
        val config = JSONObject(
            """
            {
              "outbounds": [
                {
                  "settings": {
                    "vnext": [
                      {"address": "$hostName"},
                      {"address": "other.example.com"},
                      {"address": "198.51.100.7"}
                    ]
                  }
                },
                {
                  "settings": {
                    "servers": [
                      {"address": "$hostName"},
                      {"address": "backup.example.com"}
                    ]
                  }
                },
                {
                  "settings": {
                    "vnext": [
                      "not-an-object",
                      {"address": "prefix-$hostName"}
                    ]
                  }
                }
              ]
            }
            """.trimIndent()
        )

        replaceResolvedEndpointAddresses(config, hostName, "203.0.113.10")

        val outbounds = config.getJSONArray("outbounds")
        val vnext = outbounds.getJSONObject(0)
            .getJSONObject("settings")
            .getJSONArray("vnext")
        val servers = outbounds.getJSONObject(1)
            .getJSONObject("settings")
            .getJSONArray("servers")
        val unrelated = outbounds.getJSONObject(2)
            .getJSONObject("settings")
            .getJSONArray("vnext")

        assertEquals("203.0.113.10", vnext.getJSONObject(0).getString("address"))
        assertEquals("other.example.com", vnext.getJSONObject(1).getString("address"))
        assertEquals("198.51.100.7", vnext.getJSONObject(2).getString("address"))
        assertEquals("203.0.113.10", servers.getJSONObject(0).getString("address"))
        assertEquals("backup.example.com", servers.getJSONObject(1).getString("address"))
        assertEquals("not-an-object", unrelated.getString(0))
        assertEquals("prefix-$hostName", unrelated.getJSONObject(1).getString("address"))
    }

    @Test
    fun `leaves numeric endpoints and missing optional arrays unchanged`() {
        val hostName = "edge.example.com"
        val config = JSONObject(
            """
            {
              "outbounds": [
                {},
                {"settings": {}},
                {"settings": {"vnext": [{"address": "198.51.100.8"}]}},
                {"settings": {"servers": [{"address": "2001:db8::8"}]}}
              ]
            }
            """.trimIndent()
        )

        replaceResolvedEndpointAddresses(config, hostName, "203.0.113.10")

        val outbounds = config.getJSONArray("outbounds")
        assertEquals(
            "198.51.100.8",
            outbounds.getJSONObject(2)
                .getJSONObject("settings")
                .getJSONArray("vnext")
                .getJSONObject(0)
                .getString("address"),
        )
        assertEquals(
            "2001:db8::8",
            outbounds.getJSONObject(3)
                .getJSONObject("settings")
                .getJSONArray("servers")
                .getJSONObject(0)
                .getString("address"),
        )

        val configWithoutOutbounds = JSONObject().put("remark", hostName)
        replaceResolvedEndpointAddresses(
            configWithoutOutbounds,
            hostName,
            "203.0.113.10",
        )
        assertEquals(hostName, configWithoutOutbounds.getString("remark"))
    }
}
