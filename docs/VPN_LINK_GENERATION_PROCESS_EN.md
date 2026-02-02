# VPN Link Generation Process in Amnezia Client

## Overview

Amnezia Client uses a special `vpn://` link format for sharing VPN configurations. This document describes how these links are created and how you can convert a regular WireGuard configuration into a `vpn://` link format.

## How Link Generation Works

### Encoding Process (Config → vpn:// link)

Code location: `/client/ui/controllers/exportController.cpp`

```cpp
// Lines 52-54, 94-96
QByteArray compressedConfig = QJsonDocument(serverConfig).toJson();
compressedConfig = qCompress(compressedConfig, 8);
m_config = QString("vpn://%1").arg(QString(compressedConfig.toBase64(
    QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));
```

**Steps:**
1. **JSON Serialization**: Server configuration is converted to JSON
2. **zlib Compression**: JSON is compressed using `qCompress()` (compression level 8)
3. **Base64 URL-safe Encoding**: Compressed data is encoded to Base64 with flags:
   - `Base64UrlEncoding` - URL-safe encoding (uses `-` and `_` instead of `+` and `/`)
   - `OmitTrailingEquals` - removes trailing `=` characters
4. **Add Prefix**: `vpn://` prefix is added to the result

### Decoding Process (vpn:// link → Config)

Code location: `/client/ui/controllers/importController.cpp`

```cpp
// Lines 156-161
config.replace("vpn://", "");
QByteArray ba = QByteArray::fromBase64(config.toUtf8(), 
    QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
QByteArray baUncompressed = qUncompress(ba);
if (!baUncompressed.isEmpty()) {
    ba = baUncompressed;
}
```

**Steps:**
1. **Remove Prefix**: Strip `vpn://`
2. **Base64 Decode**: URL-safe Base64 is decoded
3. **zlib Decompress**: Data is decompressed using `qUncompress()`
4. **JSON Parse**: Result is parsed as JSON configuration

## Amnezia Configuration Format

The Amnezia configuration is **NOT just a WireGuard config**, but a special JSON structure with server metadata. The format includes:

```json
{
  "hostName": "89.125.213.14",
  "defaultContainer": "amnezia-awg",
  "dns1": "1.1.1.1",
  "dns2": "8.8.8.8",
  "containers": [
    {
      "container": "amnezia-awg",
      "awg": {
        "port": "46811",
        "client_priv_key": "QI1ESrtAWzg4I6M8v8roRRdqldRCosjR6zpgFp1FRnM=",
        "client_pub_key": "...",
        "client_ip": "10.8.1.2/24",
        "psk_key": "yaYGl/gM1vNml0ST+RWkAQnc3+eC9iZ9TPyz3jvuIFc=",
        "server_pub_key": "ARATMWdjtitj3/MO8tCq7mMA7XL84SucUq+mKccNsTs=",
        "Jc": 6,
        "Jmin": 10,
        "Jmax": 50,
        "S1": 123,
        "S2": 136,
        "H1": 1043813656,
        "H2": 1394807736,
        "H3": 850386757,
        "H4": 714960491
      }
    }
  ]
}
```

## Why Simple Base64 Doesn't Work?

Simple Base64 encoding of a WireGuard config doesn't work for the following reasons:

1. **Data Format**: Amnezia uses JSON structure, not WireGuard's text INI format
2. **Compression**: Data is compressed with zlib before encoding
3. **URL-safe Base64**: A special Base64 variant without trailing `=` is used
4. **Container Structure**: Configuration needs to be wrapped in a structure with container and server metadata

## How to Create a vpn:// Link from WireGuard Config

### Using Python Script (Recommended)

We've provided a ready-to-use Python script:

```bash
# Use with default example configuration
python3 docs/wireguard_to_vpn_link.py

# Or specify your own config file
python3 docs/wireguard_to_vpn_link.py --config /path/to/wireguard.conf

# Decode a vpn:// link back to JSON
python3 docs/wireguard_to_vpn_link.py --decode 'vpn://...'
```

### Manual Python Implementation

```python
import json
import zlib
import base64

def wireguard_to_vpn_link(
    private_key, address, public_key, preshared_key, 
    endpoint, dns1, dns2,
    jc, jmin, jmax, s1, s2, h1, h2, h3, h4
):
    # Parse endpoint
    host_name, port = endpoint.split(':')
    
    # Create JSON structure
    server_config = {
        "hostName": host_name,
        "defaultContainer": "amnezia-awg",
        "dns1": dns1,
        "dns2": dns2,
        "containers": [
            {
                "container": "amnezia-awg",
                "awg": {
                    "port": port,
                    "client_priv_key": private_key,
                    "client_ip": address,
                    "psk_key": preshared_key,
                    "server_pub_key": public_key,
                    "Jc": jc,
                    "Jmin": jmin,
                    "Jmax": jmax,
                    "S1": s1,
                    "S2": s2,
                    "H1": str(h1),
                    "H2": str(h2),
                    "H3": str(h3),
                    "H4": str(h4)
                }
            }
        ]
    }
    
    # Serialize to compact JSON
    json_data = json.dumps(server_config, separators=(',', ':')).encode('utf-8')
    
    # Compress with zlib (level 8)
    compressed = zlib.compress(json_data, 8)
    
    # Encode to URL-safe Base64 without padding
    base64_encoded = base64.urlsafe_b64encode(compressed).decode('ascii').rstrip('=')
    
    return f"vpn://{base64_encoded}"

# Example usage with provided config
vpn_link = wireguard_to_vpn_link(
    private_key="QI1ESrtAWzg4I6M8v8roRRdqldRCosjR6zpgFp1FRnM=",
    address="10.8.1.2/24",
    public_key="ARATMWdjtitj3/MO8tCq7mMA7XL84SucUq+mKccNsTs=",
    preshared_key="yaYGl/gM1vNml0ST+RWkAQnc3+eC9iZ9TPyz3jvuIFc=",
    endpoint="89.125.213.14:46811",
    dns1="1.1.1.1",
    dns2="8.8.8.8",
    jc=6, jmin=10, jmax=50,
    s1=123, s2=136,
    h1=1043813656, h2=1394807736, h3=850386757, h4=714960491
)

print(vpn_link)
```

## Decoding vpn:// Links

### Python example:

```python
import json
import zlib
import base64

def decode_vpn_link(vpn_link):
    # Remove vpn:// prefix
    encoded = vpn_link.replace('vpn://', '')
    
    # Add padding if needed
    padding = 4 - (len(encoded) % 4)
    if padding != 4:
        encoded += '=' * padding
    
    # Decode from URL-safe Base64
    compressed = base64.urlsafe_b64decode(encoded)
    
    # Decompress zlib
    json_data = zlib.decompress(compressed)
    
    # Parse JSON
    config = json.loads(json_data)
    
    return config

# Example usage
config = decode_vpn_link("vpn://...")
print(json.dumps(config, indent=2))
```

## Important Notes

1. **AmneziaWG Parameters**: Parameters `Jc`, `Jmin`, `Jmax`, `S1`, `S2`, `H1-H4` are specific to AmneziaWG obfuscation
2. **Container Type**: Use `"amnezia-awg"` for AmneziaWG or `"amnezia-wg"` for regular WireGuard
3. **DNS Servers**: Specified separately in the root JSON object
4. **Compression Required**: Without zlib compression, the link won't work
5. **URL-safe Base64**: Regular Base64 won't work, URL-safe variant is required

## Source Code References

- Link generation: `/client/ui/controllers/exportController.cpp` (lines 52-54, 94-96)
- Link import: `/client/ui/controllers/importController.cpp` (lines 156-161)
- WireGuard configurator: `/client/configurators/wireguard_configurator.cpp`
- QR code utilities: `/client/core/qrCodeUtils.cpp`

## Testing

The provided configuration was successfully converted to:

```
vpn://eNplUF1PgzAU_S99nTJKCxQSHwhxbirTAWZ-xCwIFWF8jRZ0LPvvtjjjg7nJ7b3n3nNyew7go2Z8GZUU2IBYCtR0RYNIgRicgYS-R13B3briUVbRVqxEZUWHLDqPPlO5UDEoQKiM8QNoUkgZQwDxL5cB--Xw1_6Tktk-gKZuuZhhg0CpFxcZrfimabN-s6V7MVkt4GXQcmc9pHhheKQnbe37ya5IfLdmuW8MTTpr4MyvvIs_gayRZ6riKKhoU01-jtG2p-2m6d5Oyo7vhN46yXnGczT17gh3d2bpOebjLcFBFz_sJuVNHC9ZyKRyw7Yn4j56uiqmqQf7ZVmoQTjx11tnVcVoQl0re7bC-_2A8r5bzGJJvI6BbYinzCpgQ1VW0RewdVEFwk2oIVEIFyESW_PRXxUjIlrdEPS5NBgiCxPVNNGIIGm5riJimLopASwAE2LLULEFwfH4evwG1xCNOA
```

This link can be:
- Imported into Amnezia Client
- Converted to a QR code for scanning
- Shared with other users
