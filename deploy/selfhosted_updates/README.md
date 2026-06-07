# Self-hosted update channel

This directory contains tooling for the private update channel used by the
server-managed Amnezia fork.

Runtime update behavior lives in the client C++/Qt code:

- `client/core/controllers/updateController.cpp` checks and verifies the signed
  manifest, downloads artifacts, validates sha256/size, and launches the
  platform installer.
- `client/vpnConnection.cpp` keeps the update endpoint inside the managed VPN
  route set.
- `client/platforms/android/*` handles the Android APK installer handoff.

The helper files in this directory do not ship inside the app. Local release
automation uses them after platform builds to generate the signed manifest,
verify artifacts, and publish files to the update server.

The client checks signed manifests from:

- `http://<default-self-hosted-server>:17865/manifest.json`
- `http://<SELFHOSTED_UPDATE_SYNC_HOST>:17865/manifest.json`

On this release workstation the verified VPN client-facing host is
`10.8.1.0`, so local builds set `SELFHOSTED_UPDATE_SYNC_HOST=10.8.1.0`.
That is a concrete host address, not the CIDR route `10.8.1.0/1`. The Docker
bridge endpoint `172.29.172.252` may still exist server-side, but do not use it
as the compiled fallback unless a representative client can actually reach it.
The published manifest keeps local artifact URLs relative under `files/`, so
artifacts resolve from whichever manifest host the client reached.

## Release freeze automation

`.github/workflows/upstream-release-freeze.yml` is the daily guard for this
fork branch. It watches the latest published upstream GitHub Release whose tag
matches `x.y.z.w`; an upstream tag alone is not enough to freeze the fork.

- While no published release newer than `.github/upstream-release-freeze.json`
  `baselineTag` exists, it leaves `feat/server-managed-split-tunnel` unchanged.
  Ordinary `upstream/dev` commits are intentionally not merged between releases.
- When a newer published release appears, it writes `frozen=true` into the state
  file, pushes the branch, and prints the local release command for the
  workstation. The freeze step
  rebuilds the target branch from the upstream release tag and reapplies the
  fork patch, so post-release `upstream/dev` commits are not retained. Because
  this intentionally rewrites the target branch to the release base, it pushes
  the frozen branch with `--force-with-lease`.
- After the state is frozen, later scheduled runs keep waiting and advance the
  fork only when a newer published upstream release appears.

The baseline currently records `4.8.15.4`, matching the latest published
upstream GitHub Release observed when this automation was added. Use the workflow input
`FORCE_FREEZE_TAG` only for a deliberate manual freeze.

## Key setup

Generate an Ed25519 signing key:

```bash
openssl genpkey -algorithm Ed25519 -out selfhosted-update-private.pem
openssl pkey -in selfhosted-update-private.pem -pubout -out selfhosted-update-public.pem
```

Build clients with the public key embedded as base64 PEM:

```bash
export SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64="$(base64 -w0 selfhosted-update-public.pem)"
```

Keep `selfhosted-update-private.pem` off client devices and logs.

`SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64` is required at build time because the
clients must embed that public key to accept the signed private update manifest.
The private signing key is used only by the local publisher.

## Local release build

Self-hosted releases are built locally on the release workstation. The Windows
release client can carry the update payload and upload it to the Amnezia VPN
server after installation. The default local release set is:

- `windows-x64`
- `linux-x64`
- `android-arm64-v8a`

macOS and iOS are intentionally not part of this local self-hosted release set.
Android releases are intentionally arm64-v8a only for Android 9+ devices.

Prepare the release workstation first:

```powershell
powershell -ExecutionPolicy Bypass -File deploy\selfhosted_updates\setup_release_workstation.ps1
```

By default this only reports missing dependencies and writes
`dist\selfhosted-release-env.ps1`. Re-run with `-InstallMissing` to install
missing dependencies that are available non-interactively: WSL Java under
`~/.local/jdk-17`, Conan and aqtinstall through WSL `pip`, Linux Android
command-line tools/SDK/NDK under `WSL_ANDROID_HOME`, desktop Qt through
`linux desktop`, Linux Qt Installer Framework `qt.tools.ifw.47`, and the
Android arm64-v8a Qt kit through `all_os android`.
Android builds run inside WSL, so `WSL_ANDROID_HOME` must point to a
Linux Android SDK/NDK; the Windows SDK in `ANDROID_HOME` is not enough because
its NDK does not include the `linux-x86_64` compiler toolchain. If `aqtinstall`
does not publish the required Android kit for the selected Qt version, install
it with Qt MaintenanceTool and rerun preflight. Re-run with
`-GenerateUpdateKeys` to create the Ed25519 self-hosted update signing keypair
under `C:\keys`. On the release workstation, also run
`-GenerateAndroidKeystore` once to create `C:\keys\android-release.keystore`
and `C:\keys\android-release-keystore.env.ps1`. Future Android updates must be
signed by this same key, so keep both files backed up and private.

If Qt downloads time out, set `QT_MIRROR_BASE` or pass `-QtMirrorBase`; the
default mirror is `https://mirrors.20i.com/pub/qt.io`.

Run from the repository root:

```powershell
. .\dist\selfhosted-release-env.ps1

$env:SELFHOSTED_UPDATE_BASE_URL = "http://SERVER_IP:17865"
$env:SELFHOSTED_UPDATE_SERVER = "root@SERVER_IP"
$env:SELFHOSTED_UPDATE_SSH_PRIVATE_KEY_PATH = "C:\keys\server-upload-key"
# Optional override; setup writes this automatically.
$env:WSL_ANDROID_HOME = "/home/<wsl-user>/Android/sdk"
# Optional override for Linux .run packaging; must be a Linux IFW root, not C:\Qt.
$env:WSL_QIF_ROOT_PATH = "/home/<wsl-user>/Qt/Tools/QtInstallerFramework/4.7"

# Android APKs must be signed with the same key as installed clients.
# setup_release_workstation.ps1 dot-sources this file automatically after
# -GenerateAndroidKeystore has created it:
. C:\keys\android-release-keystore.env.ps1

powershell -ExecutionPolicy Bypass -File deploy\selfhosted_updates\local_release.ps1
```

The script calls `deploy\build.bat` for Windows and `deploy/build.sh` through
WSL for Linux and Android. It copies release artifacts into
`dist\selfhosted-local-artifacts\<version>`, generates and verifies the signed
manifest, then rebuilds a separate Windows release client with that payload
installed under `selfhosted_updates` next to `AmneziaVPN.exe`. That bundled
installer is written to
`dist\selfhosted-windows-client\<version>\AmneziaVPN_<version>_windows_x64_selfhosted.exe`.
Install that file on the release workstation. On startup, the Windows client
uses the saved self-hosted admin SSH credentials to upload `files/`, refresh the
update-host container, and switch `manifest.json` on the server last. Use
`-NoPublish` to stop the old direct workstation upload path; the bundled Windows
client upload still works after installation. Use `-NoBundleUpdatesInWindowsClient`
only when you intentionally need a thin Windows installer without embedded
payload. `-SkipBuild` skips rebuilding platform artifacts, but still rebuilds
the bundled Windows release client from the existing manifest payload unless
`-NoBundleUpdatesInWindowsClient` is also set.

The Windows artifact inside the manifest is the thin Windows installer from
`dist\selfhosted-local-artifacts\<version>`. The self-hosted release workstation
installer is built after manifest generation and contains that thin Windows
artifact plus Linux and Android artifacts. This avoids a recursive package where
the signed manifest would need to hash an installer that contains the manifest
that hashes the installer.

Run a fast preflight before the release build:

```powershell
powershell -ExecutionPolicy Bypass -File deploy\selfhosted_updates\local_release.ps1 -Preflight
```

Preflight does not build or upload anything. It verifies required local commands,
self-hosted signing inputs, WSL readiness for Linux/Android builds, Android
signing key inputs, Linux Qt Installer Framework inside WSL, and SSH inputs
when publishing is enabled.

## Manifest build

Example:

```bash
python deploy/selfhosted_updates/make_manifest.py \
  --version 4.8.16.0 \
  --release-date 2026-06-06 \
  --base-url http://172.29.172.252:17865 \
  --private-key selfhosted-update-private.pem \
  --out-dir dist/selfhosted-updates \
  --artifact windows-x64=deploy/build/AmneziaVPN_4.8.16.0_windows_x64.exe \
  --artifact linux-x64=deploy/build/AmneziaVPN_4.8.16.0_linux_x64.run \
  --artifact android-arm64-v8a=deploy/build-android-arm64-v8a/client/android-build/AmneziaVPN_4.8.16.0_android9+_arm64-v8a.apk \
  --auto-install
```

Upload the generated directory contents to `/opt/amnezia/client-updates` on the
self-hosted server and serve it on port `17865`.

Server-side setup:

```bash
scp -r dist/selfhosted-updates/* root@SERVER:/opt/amnezia/client-updates/
ssh root@SERVER 'sh -s' < deploy/selfhosted_updates/install_server_update_host.sh
```

The one-command publisher wraps manifest generation, upload, and server setup:

```bash
python deploy/selfhosted_updates/publish_release.py \
  --version 4.8.16.0 \
  --private-key selfhosted-update-private.pem \
  --public-key-base64 "$SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64" \
  --artifact-dir deploy/build \
  --base-url http://SERVER_IP:17865 \
  --server root@SERVER_IP \
  --require-platform windows-x64 \
  --require-platform linux-x64 \
  --require-platform android-arm64-v8a \
  --auto-install
```

It autodetects the normal release artifact names, including
installable upstream aliases such as
`AmneziaVPN_<version>_x64.exe`. It does
not treat upstream `linux_x64.tar` archives as Linux auto-installers; Linux
auto-install requires the fork CI `.run` artifact or an explicit
`--artifact linux-x64=...run`. The publisher accepts repeated
`--artifact platform=path` plus `--external platform=url` overrides. Before any upload, the publisher
verifies that the public key embedded in built clients matches the private
signing key, then verifies the generated manifest schema, required platforms,
`autoInstall` flags when enabled, and the Ed25519 signature.
`--public-key-base64` can also be supplied through
`SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64`; it is required when publishing to a
server.

For local publishing configure:

- `SELFHOSTED_UPDATE_BASE_URL`: client-facing base URL, for example
  `http://SERVER_IP:17865`.
- `SELFHOSTED_UPDATE_SERVER`: SSH target, for example `root@SERVER_IP`.
- `SELFHOSTED_UPDATE_PRIVATE_KEY_PATH`: local path to the Ed25519 update
  signing key.
- `SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64`: base64 PEM public key embedded in
  built clients; it must match `SELFHOSTED_UPDATE_PRIVATE_KEY_PATH`.
- `SELFHOSTED_UPDATE_SYNC_HOST`: compiled fallback host for the private
  manifest endpoint. On this workstation it is `10.8.1.0`; do not include a
  CIDR suffix such as `/1`.
- `SELFHOSTED_UPDATE_SSH_PRIVATE_KEY_PATH`: local SSH key path for upload.
- `SELFHOSTED_UPDATE_SERVER_DIR`: optional, defaults to
  `/opt/amnezia/client-updates`.

The signed manifest sets `autoInstall=true`; clients will start the platform
installer once per version/platform/artifact identity. This respects OS rules:
Windows/Linux can launch installers, and Android opens Package Installer.

By default the helper publishes `0.0.0.0:17865` on the server host and also
serves `172.29.172.252:17865` on the Amnezia Docker bridge. If an active
Amnezia VPN container is present, the helper also starts a tunnel endpoint in
that container namespace, using the same port. For the current server, the
verified client-facing VPN endpoint is `http://10.8.1.0:17865`. To keep the
public host port closed and serve only through VPN/internal routes, run it with
`AMNEZIA_UPDATE_PUBLISH_HOST_PORT=0`.

The helper autodetects `amnezia-awg2`, `amnezia-awg`, `amnezia-wireguard`, and
`amnezia-openvpn`. To force a specific VPN container network namespace, set
`AMNEZIA_UPDATE_VPN_CONTAINER` to that Docker container name, for example:

```bash
ssh root@SERVER 'AMNEZIA_UPDATE_VPN_CONTAINER=amnezia-awg sh -s' < deploy/selfhosted_updates/install_server_update_host.sh
```

The manifest envelope is signed, and desktop installers are additionally
checked by sha256 before launch. Android APK artifacts are downloaded by the
app, checked by sha256, and then handed to Android Package Installer; if the
unknown-app install permission is missing, the app opens the permission screen
and resumes installation when the user returns. Android can still use an
external store/browser URL when the manifest artifact has `openExternal=true`.
