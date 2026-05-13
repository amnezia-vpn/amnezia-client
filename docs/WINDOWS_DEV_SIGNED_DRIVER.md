# Windows dev-signed split tunnel driver

This build contains a development-signed `mullvad-split-tunnel.sys` driver for the Windows per-app VPN whitelist mode.

## Important limitation

Windows will not load a self-signed kernel driver in normal production mode. For this development build, every target machine must:

- trust the public development code-signing certificate;
- enable Windows test-signing mode;
- reboot after test-signing is enabled.

For public production distribution, use a Microsoft-compatible kernel driver signing flow instead of this self-signed certificate.

## One-time setup on a target machine

Install or upgrade AmneziaVPN first, then run this from an elevated terminal:

```powershell
cd "C:\Program Files\AmneziaVPN"
.\enable-dev-split-driver.ps1
```

If Windows reports that test-signing was just enabled, reboot Windows before using split tunneling.

After reboot, start AmneziaVPN and enable:

`Only apps from the list should have access via VPN`

Expected behavior:

- apps in the list use the VPN tunnel;
- apps not in the list bypass the VPN.

## Secure Boot

If `bcdedit /set testsigning on` fails, Secure Boot is usually enabled. Disable Secure Boot in UEFI/BIOS for this development build, or use a properly signed production driver.

## Notes

The helper script embeds only the public certificate. It does not contain the private PFX key used to sign the driver.
