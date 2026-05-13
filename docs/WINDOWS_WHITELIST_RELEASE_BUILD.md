# Windows whitelist split-tunnel release build

## Artifacts

Primary installer for users with an existing AmneziaVPN Qt Installer Framework installation:

```text
C:\amnezia-client\deploy\build\AmneziaVPN-4.8.16.0-win64.exe
```

Additional MSI artifact:

```text
C:\amnezia-client\deploy\build\AmneziaVPN-4.8.16.0-win64.msi
```

The IFW `.exe` is preferred for upgrading old AmneziaVPN installations that contain `maintenancetool.exe`.

## Included changes

- Windows app split tunneling supports include-only mode: only listed apps use the VPN tunnel.
- `AmneziaVPN-service.exe` sends the split policy IOCTL before applying app configuration.
- `mullvad-split-tunnel.sys` understands include-only policy and prevents whitelist processes from inheriting bypass from non-whitelist parents.
- The installer includes `enable-dev-split-driver.ps1` and `enable-dev-split-driver.cmd`.

## Development-signed driver setup

For this development build, target machines must allow the self-signed kernel driver:

```powershell
cd "C:\Program Files\AmneziaVPN"
.\enable-dev-split-driver.ps1
```

If the script enables test-signing mode, reboot Windows before testing split tunneling.

See `docs/WINDOWS_DEV_SIGNED_DRIVER.md` for details and Secure Boot limitations.

## Verification performed on the build machine

- Release build completed successfully.
- IFW installer generated successfully.
- WiX MSI generated successfully.
- Installer artifacts were Authenticode-signed with the local development code-signing certificate.
- Packaged `mullvad-split-tunnel.sys` signature verified with `signtool verify /pa`.
- IFW package staging contains:
  - `AmneziaVPN-service.exe`
  - `mullvad-split-tunnel.sys`
  - `amnezia_xray.dll`
  - `Qt6Concurrent.dll`
  - `enable-dev-split-driver.ps1`
  - `enable-dev-split-driver.cmd`

## Production note

For broad distribution without Windows Test Mode, replace the development self-signed driver with a driver signed through Microsoft's driver signing flow.
