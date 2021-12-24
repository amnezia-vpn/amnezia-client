# shadowsocks

![Build & Test](https://github.com/shadowsocks/shadowsocks-rust/workflows/Build%20&%20Test/badge.svg)
![Build Releases](https://github.com/shadowsocks/shadowsocks-rust/workflows/Build%20Releases/badge.svg)
[![License](https://img.shields.io/github/license/zonyitoo/shadowsocks-rust.svg)](https://github.com/zonyitoo/shadowsocks-rust)
[![crates.io](https://img.shields.io/crates/v/shadowsocks-rust.svg)](https://crates.io/crates/shadowsocks-rust)
[![Release](https://img.shields.io/github/release/shadowsocks/shadowsocks-rust.svg)](https://github.com/shadowsocks/shadowsocks-rust/releases)
[![Dependabot Status](https://api.dependabot.com/badges/status?host=github&repo=shadowsocks%2fshadowsocks-rust)](https://dependabot.com)

This is a port of [shadowsocks](https://github.com/shadowsocks/shadowsocks).

shadowsocks is a fast tunnel proxy that helps you bypass firewalls.

| Library | Description |
| ------- | ----------- |
| [**shadowsocks**](https://crates.io/crates/shadowsocks) | [![crates.io](https://img.shields.io/crates/v/shadowsocks.svg)](https://crates.io/crates/shadowsocks) [![docs.rs](https://img.shields.io/docsrs/shadowsocks)](https://docs.rs/shadowsocks) shadowsocks core protocol |
| [**shadowsocks-service**](https://crates.io/crates/shadowsocks-service) | [![crates.io](https://img.shields.io/crates/v/shadowsocks-service.svg)](https://crates.io/crates/shadowsocks-service) [![docs.rs](https://img.shields.io/docsrs/shadowsocks-service)](https://docs.rs/shadowsocks-service) Services for serving shadowsocks |
| [**shadowsocks-rust**](https://crates.io/crates/shadowsocks-rust) | [![crates.io](https://img.shields.io/crates/v/shadowsocks-rust.svg)](https://crates.io/crates/shadowsocks-rust) Binaries running common shadowsocks services |

## Build & Install

### Optional Features

* `trust-dns` - Uses [`trust-dns-resolver`](https://crates.io/crates/trust-dns-resolver) as DNS resolver instead of `tokio`'s builtin.

* `local-http` - Allow using HTTP protocol for `sslocal`

  * `local-http-native-tls` - Support HTTPS with [`native-tls`](https://crates.io/crates/native-tls)
  
  * `local-http-rustls` - Support HTTPS with [`rustls`](https://crates.io/crates/rustls)

* `local-tunnel` - Allow using tunnel protocol for `sslocal`

* `local-socks4` - Allow using SOCKS4/4a protocol for `sslocal`

* `local-redir` - Allow using redir (transparent proxy) protocol for `sslocal`

* `stream-cipher` - Enable deprecated stream ciphers. WARN: stream ciphers are UNSAFE!

#### Memory Allocators

This project uses system (libc) memory allocator (Rust's default). But it also allows you to use other famous allocators by features:

* `jemalloc` - Uses [jemalloc](http://jemalloc.net/) as global memory allocator
* `mimalloc` - Uses [mi-malloc](https://microsoft.github.io/mimalloc/) as global memory allocator
* `tcmalloc` - Uses [TCMalloc](https://google.github.io/tcmalloc/overview.html) as global memory allocator. It tries to link system-wide tcmalloc by default, use vendored from source with `tcmalloc-vendored`.

### **crates.io**

Install from [crates.io](https://crates.io/crates/shadowsocks-rust):

```bash
cargo install shadowsocks-rust
```

then you can find `sslocal` and `ssserver` in `$CARGO_HOME/bin`.

### **Download release**

Download static-linked build [here](https://github.com/shadowsocks/shadowsocks-rust/releases).

* `build-windows`: Build for `x86_64-pc-windows-msvc`
* `build-linux`: Build for `x86_64-unknown-linux-gnu`, Debian 9 (Stretch), GLIBC 2.18
* `build-docker`: Build for `x86_64-unknown-linux-musl`, `x86_64-pc-windows-gnu`, ... (statically linked)

### **Build from source**

Use cargo to build. NOTE: **RAM >= 2GiB**

```bash
cargo build --release
```

Then `sslocal` and `ssserver` will appear in `./target/(debug|release)/`, it works similarly as the two binaries in the official ShadowSocks' implementation.

```bash
make install TARGET=release
```

Then `sslocal`, `ssserver`, `ssmanager` and `ssurl` will be installed to `/usr/local/bin` (variable PREFIX).

For Windows users, if you have encountered any problem in building, check and discuss in [#102](https://github.com/shadowsocks/shadowsocks-rust/issues/102).

### **target-cpu optimization**

If you are building for your current CPU platform (for example, build and run on your personal computer), it is recommended to set `target-cpu=native` feature to let `rustc` generate and optimize code for the CPU running the compiler.

```bash
export RUSTFLAGS="-C target-cpu=native"
```

### **Build standalone binaries**

Requirements:

* Docker

```bash
./build/build-release
```

Then `sslocal`, `ssserver`, `ssmanager` and `ssurl` will be packaged in

* `./build/shadowsocks-${VERSION}-stable.x86_64-unknown-linux-musl.tar.xz`
* `./build/shadowsocks-${VERSION}-stable.x86_64-pc-windows-gnu.zip`

Read `Cargo.toml` for more details.

## Getting Started

Create a ShadowSocks' configuration file. Example

```jsonc
{
    "server": "my_server_ip",
    "server_port": 8388,
    "password": "mypassword",
    "method": "aes-256-gcm",
    // ONLY FOR `sslocal`
    // Delete these lines if you are running `ssserver` or `ssmanager`
    "local_address": "127.0.0.1",
    "local_port": 1080
}
```

Detailed explanation could be found in [shadowsocks' documentation](https://github.com/shadowsocks/shadowsocks/wiki).

In shadowsocks-rust, we also have an extended configuration file format, which is able to define more than one server. You can also disable individual servers.

```jsonc
{
    "servers": [
        {
            "address": "127.0.0.1",
            "port": 8388,
            "password": "hello-world",
            "method": "aes-256-gcm",
            "timeout": 7200
        },
        {
            "address": "127.0.0.1",
            "port": 8389,
            "password": "hello-kitty",
            "method": "chacha20-ietf-poly1305"
        },
        {
            "disabled": true,
            "address": "eg.disable.me",
            "port": 8390,
            "password": "hello-internet",
            "method": "chacha20-ietf-poly1305"
        }
    ],
    // ONLY FOR `sslocal`
    // Delete these lines if you are running `ssserver` or `ssmanager`
    "local_port": 1080,
    "local_address": "127.0.0.1"
}
```

`sslocal` automatically selects the best server with the lowest latency and the highest availability.

Start Shadowsocks client and server with:

```bash
sslocal -c config.json
ssserver -c config.json
```

If you Build it with Cargo:

```bash
cargo run --bin sslocal -- -c config.json
cargo run --bin ssserver -- -c config.json
```

List all available arguments with `-h`.

## Usage

### Socks5 Local client

```bash
# Read local client configuration from file
sslocal -c /path/to/shadowsocks.json

# Pass all parameters via command line
sslocal -b "127.0.0.1:1080" -s "[::1]:8388" -m "aes-256-gcm" -k "hello-kitty" --plugin "v2ray-plugin" --plugin-opts "server;tls;host=github.com"

# Pass server with SIP002 URL
sslocal -b "127.0.0.1:1080" --server-url "ss://YWVzLTI1Ni1nY206cGFzc3dvcmQ@127.0.0.1:8388/?plugin=v2ray-plugin%3Bserver%3Btls%3Bhost%3Dgithub.com"
```

### HTTP Local client

```bash
# Read local client configuration from file
sslocal -c /path/to/shadowsocks.json --protocol http
```

All parameters are the same as Socks5 client, except `--protocol http`.

### Tunnel Local client

```bash
# Read local client configuration from file
# Set 127.0.0.1:8080 as the target for forwarding to
sslocal -c /path/to/shadowsocks.json -f "127.0.0.1:8080" --protocol tunnel
```

### Transparent Proxy Local client

**NOTE**: This is currently only supports

* Linux (with `iptables` targets `REDIRECT` and `TPROXY`)
* BSDs (with `pf`), such as OS X 10.10+, FreeBSD, ...

```bash
# Read local client configuration from file
sslocal -c /path/to/shadowsocks.json --protocol redir
```

Redirects connections with `iptables` configurations to the port that `sslocal` is listening on.

### Server

```bash
# Read server configuration from file
ssserver -c /path/to/shadowsocks.json

# Pass all parameters via command line
ssserver -s "[::]:8388" -m "aes-256-gcm" -k "hello-kitty" --plugin "v2ray-plugin" --plugin-opts "server;tls;host=github.com"
```

### Server Manager

Supported [Manage Multiple Users](https://github.com/shadowsocks/shadowsocks/wiki/Manage-Multiple-Users) API:

* `add` - Starts a server instance
* `remove` - Deletes an existing server instance
* `list` - Lists all current running servers
* `ping` - Lists all servers' statistic data

NOTE: `stat` command is not supported. Because servers are running in the same process with the manager itself.

```bash
# Start it just with --manager-address command line parameter
ssmanager --manager-address "127.0.0.1:6100"

# For *nix system, manager can bind to unix socket address
ssmanager --manager-address "/tmp/shadowsocks-manager.sock"

# You can also provide a configuration file
#
# `manager_address` key must be provided in the configuration file
ssmanager -c /path/to/shadowsocks.json

# Create one server by UDP
echo 'add: {"server_port":8388,"password":"hello-kitty"}' | nc -u '127.0.0.1' '6100'

# Close one server by unix socket
echo 'remove: {"server_port":8388}' | nc -Uu '/tmp/shadowsocks-manager.sock'
```

For manager UI, check more details in the [shadowsocks-manager](https://github.com/shadowsocks/shadowsocks-manager) project.

Example configuration:

```jsonc
{
    // Required option
    // Address that ssmanager is listening on
    "manager_address": "127.0.0.1",
    "manager_port": 6100,

    // Or bind to a Unix Domain Socket
    "manager_address": "/tmp/shadowsocks-manager.sock",

    "servers": [
        // These servers will be started automatically when ssmanager is started
    ],

    // Outbound socket binds to this IP address
    // For choosing different network interface on the same machine
    "local_address": "xxx.xxx.xxx.xxx",

    // Other options that may be passed directly to new servers
}
```

## Configuration

```jsonc
{
    // LOCAL: Listen address. This is exactly the same as `locals[0]`
    // SERVER: Bind address for remote sockets, mostly used for choosing interface
    //         Don't set it if you don't know what's this for.
    "local_address": "127.0.0.1",
    "local_port": 1080,

    // Extended multiple local configuration
    "locals": [
        {
            // Basic configuration, a SOCKS5 local server
            "local_address": "127.0.0.1",
            "local_port": 1080,
            // OPTIONAL. Setting the `mode` for this specific local server instance.
            // If not set, it will derive from the outer `mode`
            "mode": "tcp_and_udp"
        },
        {
            // SOCKS5, SOCKS4/4a local server
            "protocol": "socks",
            // Listen address
            "local_address": "127.0.0.1",
            "local_port": 1081,
            // OPTIONAL. Enables UDP relay
            "mode": "tcp_and_udp",
            // OPTIONAL. Customizing the UDP's binding address. Depending on `mode`, if
            // - TCP is enabled, then SOCKS5's UDP Association command will return this address
            // - UDP is enabled, then SOCKS5's UDP server will listen to this address.
            "local_udp_address": "127.0.0.1",
            "local_udp_port": 2081
        },
        {
            // Tunnel local server (feature = "local-tunnel")
            "protocol": "tunnel",
            // Listen address
            "local_address": "127.0.0.1",
            "local_port": 5353,
            // Forward address, the target of this tunnel
            // In this example, this will build a `127.0.0.1:5353` -> `8.8.8.8:53` tunnel
            "forward_address": "8.8.8.8",
            "forward_port": 53,
            // OPTIONAL. Customizing whether to start TCP and UDP tunnel
            "mode": "tcp_only"
        },
        {
            // HTTP local server (feature = "local-http")
            "protocol": "http",
            // Listen address
            "local_address": "127.0.0.1",
            "local_port": 3128
        },
        {
            // DNS local server (feature = "local-dns")
            // This DNS works like China-DNS, it will send requests to `local_dns` and `remote_dns` and choose by ACL rules
            "protocol": "dns",
            // Listen address
            "local_address": "127.0.0.1",
            "local_port": 53,
            // Local DNS address, DNS queries will be sent directly to this address
            "local_dns_address": "114.114.114.114",
            // OPTIONAL. Local DNS's port, 53 by default
            "local_dns_port": 53,
            // Remote DNS address, DNS queries will be sent through ssserver to this address
            "remote_dns_address": "8.8.8.8",
            // OPTIONAL. Remote DNS's port, 53 by default
            "remote_dns_port": 53
        }
    ],

    // Server configuration
    // listen on [::] for dual stack support
    "server": "::",
    // Change to use your custom port number
    "server_port": 8388,
    "method": "aes-256-gcm",
    "password": "your-password",
    "plugin": "v2ray-plugin",
    "plugin_opts": "mode=quic;host=github.com",
    // Server: TCP socket timeout in seconds.
    // Client: TCP connection timeout in seconds.
    // Omit this field if you don't have specific needs.
    "timeout": 7200,

    // Extended multiple server configuration
    // LOCAL: Choosing the best server to connect dynamically
    // SERVER: Creating multiple servers in one process
    "servers": [
        {
            // Fields are the same as the single server's configuration
            
            // Individual servers can be disabled
            // "disabled": true,
            "address": "0.0.0.0",
            "port": 8389,
            "method": "aes-256-gcm",
            "password": "your-password",
            "plugin": "...",
            "plugin_opts": "...",
            "timeout": 7200,
        }
    ],

    // Global configurations for UDP associations
    "udp_timeout": 300, // Timeout for UDP associations (in seconds), 5 minutes by default
    "udp_max_associations": 512, // Maximum UDP associations to be kept in one server, unlimited by default

    // Options for Manager
    "manager_address": "127.0.0.1", // Could be a path to UNIX socket, /tmp/shadowsocks-manager.sock
    "manager_port": 5300, // Not needed for UNIX socket

    // DNS server's address for resolving domain names
    // For *NIX and Windows, it uses system's configuration by default
    //
    // Value could be IP address of DNS server, for example, "8.8.8.8".
    // DNS client will automatically request port 53 with both TCP and UDP protocol.
    //
    // It also allows some pre-defined well-known public DNS servers:
    // - google (TCP, UDP)
    // - cloudflare (TCP, UDP)
    // - cloudflare_tls (TLS), enable by feature "dns-over-tls"
    // - cloudflare_https (HTTPS), enable by feature "dns-over-https"
    // - quad9 (TCP, UDP)
    // - quad9_tls (TLS), enable by feature "dns-over-tls"
    //
    // The field is only effective if feature "trust-dns" is enabled.
    "dns": "google",

    // Mode, could be one of the
    // - tcp_only
    // - tcp_and_udp
    // - udp_only
    "mode": "tcp_only",

    // TCP_NODELAY
    "no_delay": false,

    // Soft and Hard limit of file descriptors on *NIX systems
    "nofile": 10240,

    // Try to resolve domain name to IPv6 (AAAA) addresses first
    "ipv6_first": false
}
```

## Supported Ciphers

### AEAD Ciphers

* `chacha20-ietf-poly1305`
* `aes-128-gcm`, `aes-256-gcm`

### Stream Ciphers

* `plain` or `none` (No encryption, only used for debugging or with plugins that ensure transport security)

<details><summary>Deprecated</summary>
<p>

* `table`
* `aes-128-cfb`, `aes-128-cfb1`, `aes-128-cfb8`, `aes-128-cfb128`
* `aes-192-cfb`, `aes-192-cfb1`, `aes-192-cfb8`, `aes-192-cfb128`
* `aes-256-cfb`, `aes-256-cfb1`, `aes-256-cfb8`, `aes-256-cfb128`
* `aes-128-ctr`
* `aes-192-ctr`
* `aes-256-ctr`
* `camellia-128-cfb`, `camellia-128-cfb1`, `camellia-128-cfb8`, `camellia-128-cfb128`
* `camellia-192-cfb`, `camellia-192-cfb1`, `camellia-192-cfb8`, `camellia-192-cfb128`
* `camellia-256-cfb`, `camellia-256-cfb1`, `camellia-256-cfb8`, `camellia-256-cfb128`
* `rc4-md5`
* `chacha20-ietf`

</p>
</details>

## ACL

`sslocal`, `ssserver`, and `ssmanager` support ACL file with syntax like [shadowsocks-libev](https://github.com/shadowsocks/shadowsocks-libev). Some examples could be found in [here](https://github.com/shadowsocks/shadowsocks-libev/tree/master/acl).

### Available sections

* For local servers (`sslocal`, `ssredir`, ...)
  * Modes:
    * `[bypass_all]` - ACL runs in `BlackList` mode. Bypasses all addresses that didn't match any rules.
    * `[proxy_all]` - ACL runs in `WhiteList` mode. Proxies all addresses that didn't match any rules.
  * Rules:
    * `[bypass_list]` - Rules for connecting directly
    * `[proxy_list]` - Rules for connecting through proxies
* For remote servers (`ssserver`)
  * Modes:
    * `[reject_all]` - ACL runs in `BlackList` mode. Rejects all clients that didn't match any rules.
    * `[accept_all]` - ACL runs in `WhiteList` mode. Accepts all clients that didn't match any rules.
  * Rules:
    * `[white_list]` - Rules for accepted clients
    * `[black_list]` - Rules for rejected clients
    * `[outbound_block_list]` - Rules for blocking outbound addresses.

### Example

```ini
# SERVERS
# For ssserver, accepts requests from all clients by default
[accept_all]

# Blocks these clients
[black_list]
1.2.3.4
127.0.0.1/8

# Disallow these outbound addresses
[outbound_block_list]
127.0.0.1/8
::1
(^|\.)baidu.com

# CLIENTS
# For sslocal, ..., bypasses all targets by default
[bypass_all]

# Proxy these addresses
[proxy_list]
(^|\.)google.com
8.8.8.8
```

## Useful Tools

1. `ssurl` is for encoding and decoding ShadowSocks URLs (SIP002). Example:

  ```plain
  ss://YWVzLTI1Ni1jZmI6cGFzc3dvcmQ@127.0.0.1:8388/?plugin=obfs-local%3Bobfs%3Dhttp%3Bobfs-host%3Dwww.baidu.com
  ```

## Notes

It supports the following features:

* [x] SOCKS5 CONNECT command
* [x] SOCKS5 UDP ASSOCIATE command (partial)
* [x] SOCKS4/4a CONNECT command
* [x] Various crypto algorithms
* [x] Load balancing (multiple servers) and server delay checking
* [x] [SIP004](https://github.com/shadowsocks/shadowsocks-org/issues/30) AEAD ciphers
* [x] [SIP003](https://github.com/shadowsocks/shadowsocks-org/issues/28) Plugins
* [x] [SIP002](https://github.com/shadowsocks/shadowsocks-org/issues/27) Extension ss URLs
* [x] HTTP Proxy Supports ([RFC 7230](http://tools.ietf.org/html/rfc7230) and [CONNECT](https://tools.ietf.org/html/draft-luotonen-web-proxy-tunneling-01))
* [x] Defend against replay attacks, [shadowsocks/shadowsocks-org#44](https://github.com/shadowsocks/shadowsocks-org/issues/44)
* [x] Manager APIs, supporting [Manage Multiple Users](https://github.com/shadowsocks/shadowsocks/wiki/Manage-Multiple-Users)
* [x] ACL (Access Control List)
* [x] Support HTTP/HTTPS Proxy protocol

## TODO

* [x] Documentation
* [x] Extend configuration format
* [x] Improved logging format (waiting for the new official log crate)
* [x] Support more ciphers without depending on `libcrypto` (waiting for an acceptable Rust crypto lib implementation)
* [x] Windows support.
* [x] Build with stable `rustc`.
* [x] Support HTTP Proxy protocol
* [x] AEAD ciphers. (proposed in [SIP004](https://github.com/shadowsocks/shadowsocks-org/issues/30), still under discussion)
* [x] Choose server based on delay #152

## License

[The MIT License (MIT)](https://opensource.org/licenses/MIT)

Copyright (c) 2014 Y. T. CHUNG

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
