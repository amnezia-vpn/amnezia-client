#!/usr/bin/env python3
"""Build a signed self-hosted Amnezia update manifest.

The client verifies an Ed25519 signature over the exact payload bytes stored
inside the base64url manifest envelope. This avoids brittle JSON
canonicalization differences between Python and Qt.
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import ipaddress
import json
import os
import plistlib
import shutil
import subprocess
import sys
import tempfile
import re
from pathlib import Path
from urllib.parse import parse_qs, quote, urlparse

VERSION_RE = re.compile(r"^[0-9]+(?:\.[0-9]+){3}$")


def openssl_command() -> str:
    candidates = [
        os.environ.get("OPENSSL"),
        shutil.which("openssl"),
        r"C:\Program Files\Git\usr\bin\openssl.exe",
        r"C:\Program Files\Git\mingw64\bin\openssl.exe",
    ]
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return str(candidate)
    return "openssl"


def b64url(data: bytes) -> str:
    return base64.urlsafe_b64encode(data).decode("ascii").rstrip("=")


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def file_url(base_url: str, filename: str) -> str:
    return f"{base_url.rstrip('/')}/files/{quote(filename)}"


def relative_file_url(filename: str) -> str:
    return f"files/{quote(filename)}"


def itms_services_url(plist_url: str) -> str:
    return f"itms-services://?action=download-manifest&url={quote(plist_url, safe='')}"


def validate_base_url(value: str) -> str:
    normalized = value.strip().rstrip("/")
    parsed = urlparse(normalized)
    if parsed.scheme not in {"http", "https"} or not parsed.netloc or not parsed.hostname:
        raise SystemExit("--base-url must be an http(s) endpoint URL with a host, for example http://172.29.172.252:17865")
    if parsed.username or parsed.password or parsed.query or parsed.fragment:
        raise SystemExit("--base-url must not contain userinfo, query, or fragment parts")
    if "/" in parsed.hostname:
        raise SystemExit("--base-url host must be a single host or IP address, not a CIDR route")
    try:
        ipaddress.ip_address(parsed.hostname)
        host_is_ip = True
    except ValueError:
        host_is_ip = False
    if host_is_ip and parsed.path.count("/") == 1 and parsed.path[1:].isdigit():
        raise SystemExit("--base-url must point to an update endpoint, not a CIDR route such as 10.8.1.0/1")
    return normalized


def validate_release_version(value: str) -> str:
    normalized = value.strip()
    if not VERSION_RE.fullmatch(normalized):
        raise SystemExit("--version must be a release version in x.y.z.w numeric format")
    return normalized


def validate_external_url(platform: str, value: str) -> str:
    normalized = value.strip()
    parsed = urlparse(normalized)
    normalized_platform = platform.strip()
    if not normalized_platform:
        raise SystemExit("external platform must not be empty")
    if not parsed.scheme:
        raise SystemExit(f"external URL for {platform} must be absolute")
    if normalized_platform == "ios":
        validate_ios_external_url(normalized)
    elif parsed.scheme not in {"http", "https"} or not parsed.netloc:
        raise SystemExit(f"external URL for {platform} must use HTTP or HTTPS")
    return normalized


def validate_ios_external_url(value: str) -> None:
    parsed = urlparse(value)
    if parsed.scheme == "itms-services":
        manifest_urls = parse_qs(parsed.query).get("url", [])
        if not manifest_urls:
            raise SystemExit("iOS itms-services URL must include a manifest url query parameter")
        manifest = urlparse(manifest_urls[0])
        if manifest.scheme != "https" or not manifest.netloc:
            raise SystemExit("iOS itms-services manifest URL must use HTTPS with a host")
        return
    if parsed.scheme == "https":
        if not parsed.netloc:
            raise SystemExit("iOS HTTPS external URL must include a host")
        return
    if parsed.scheme == "itms-apps":
        if not parsed.netloc:
            raise SystemExit("iOS itms-apps external URL must include a host")
        return
    raise SystemExit("iOS external URL must use HTTPS, itms-apps, or itms-services with an HTTPS manifest")


def require_https_base_url_for_ios_ota(base_url: str) -> None:
    if urlparse(base_url).scheme != "https":
        raise SystemExit("--ios-ipa requires --base-url to use HTTPS so iOS can install the OTA manifest and IPA")


def ios_bundle_version(value: str, *, explicit: bool = False) -> str:
    parts = value.strip().split(".")
    if not parts or any(not part.isdigit() for part in parts):
        raise SystemExit("iOS bundle version must contain only digits and periods")
    if explicit and len(parts) > 3:
        raise SystemExit("--ios-bundle-version must contain one to three numeric components")
    normalized = parts[:3]
    if not normalized:
        raise SystemExit("iOS bundle version must not be empty")
    return ".".join(str(int(part)) for part in normalized)


def sign_payload(private_key: Path, payload: bytes) -> str:
    with tempfile.TemporaryDirectory() as tmp:
        payload_path = Path(tmp) / "payload.json"
        sig_path = Path(tmp) / "payload.sig"
        payload_path.write_bytes(payload)
        subprocess.run(
            [
                openssl_command(),
                "pkeyutl",
                "-sign",
                "-rawin",
                "-inkey",
                str(private_key),
                "-in",
                str(payload_path),
                "-out",
                str(sig_path),
            ],
            check=True,
        )
        return base64.b64encode(sig_path.read_bytes()).decode("ascii")


def parse_artifact(values: list[str]) -> dict[str, Path]:
    artifacts: dict[str, Path] = {}
    for value in values:
        if "=" not in value:
            raise SystemExit(f"artifact must be platform=path, got {value!r}")
        platform, path = value.split("=", 1)
        platform = platform.strip()
        artifact_path = Path(path).expanduser().resolve()
        if not platform:
            raise SystemExit("artifact platform must not be empty")
        if not artifact_path.is_file():
            raise SystemExit(f"artifact file does not exist: {artifact_path}")
        if platform in artifacts:
            raise SystemExit(f"duplicate artifact platform: {platform}")
        artifacts[platform] = artifact_path
    return artifacts


def write_ios_plist(path: Path, ipa_url: str, bundle_id: str, bundle_version: str, title: str) -> None:
    payload = {
        "items": [
            {
                "assets": [
                    {
                        "kind": "software-package",
                        "url": ipa_url,
                    }
                ],
                "metadata": {
                    "bundle-identifier": bundle_id,
                    "bundle-version": bundle_version,
                    "kind": "software",
                    "title": title,
                },
            }
        ]
    }
    path.write_bytes(plistlib.dumps(payload, sort_keys=True))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--version", required=True)
    parser.add_argument("--release-date", default="")
    parser.add_argument("--changelog-file", type=Path)
    parser.add_argument("--base-url", required=True, help="Example: http://172.29.172.252:17865")
    parser.add_argument("--private-key", type=Path, required=True, help="Ed25519 private key PEM")
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument(
        "--artifact",
        action="append",
        default=[],
        help="platform=path; examples: windows-x64=AmneziaVPN.exe android-arm64-v8a=app.apk",
    )
    parser.add_argument(
        "--external",
        action="append",
        default=[],
        help="platform=url for platforms where the client should open an external installer URL, such as ios=itms-services://...",
    )
    parser.add_argument("--ios-ipa", type=Path, help="Optional iOS enterprise/MDM IPA to copy into files/")
    parser.add_argument("--ios-bundle-id", help="Required with --ios-ipa, for example org.amnezia.AmneziaVPN")
    parser.add_argument("--ios-bundle-version", help="Defaults to --version when --ios-ipa is used")
    parser.add_argument("--ios-title", default="AmneziaVPN")
    parser.add_argument("--auto-install", action="store_true", help="Ask clients to start the OS installer automatically")
    args = parser.parse_args()
    version = validate_release_version(args.version)
    base_url = validate_base_url(args.base_url)

    out_dir = args.out_dir.resolve()
    files_dir = out_dir / "files"
    files_dir.mkdir(parents=True, exist_ok=True)

    platforms: dict[str, dict[str, object]] = {}
    reserved_file_names: dict[str, str] = {}
    def add_platform(platform: str, artifact: dict[str, object]) -> None:
        if platform in platforms:
            raise SystemExit(f"duplicate manifest platform: {platform}")
        platforms[platform] = artifact

    def reserve_file_name(platform: str, file_name: str) -> None:
        owner = reserved_file_names.get(file_name)
        if owner and owner != platform:
            raise SystemExit(f"duplicate artifact output filename {file_name!r} for platforms {owner} and {platform}")
        reserved_file_names[file_name] = platform

    for platform, artifact_path in parse_artifact(args.artifact).items():
        reserve_file_name(platform, artifact_path.name)
        target = files_dir / artifact_path.name
        shutil.copy2(artifact_path, target)
        add_platform(platform, {
            "url": relative_file_url(target.name),
            "sha256": sha256(target),
            "size": target.stat().st_size,
            "autoInstall": args.auto_install,
        })

    if args.ios_ipa:
        require_https_base_url_for_ios_ota(base_url)
        if not args.ios_bundle_id:
            raise SystemExit("--ios-bundle-id is required when --ios-ipa is used")
        ipa_path = args.ios_ipa.expanduser().resolve()
        if not ipa_path.is_file():
            raise SystemExit(f"iOS IPA file does not exist: {ipa_path}")
        reserve_file_name("ios", ipa_path.name)
        reserve_file_name("ios", f"{ipa_path.stem}.plist")
        ipa_target = files_dir / ipa_path.name
        shutil.copy2(ipa_path, ipa_target)
        plist_target = files_dir / f"{ipa_path.stem}.plist"
        ipa_url = file_url(base_url, ipa_target.name)
        plist_url = file_url(base_url, plist_target.name)
        write_ios_plist(
            plist_target,
            ipa_url,
            args.ios_bundle_id,
            ios_bundle_version(args.ios_bundle_version, explicit=True) if args.ios_bundle_version else ios_bundle_version(version),
            args.ios_title,
        )
        add_platform("ios", {
            "url": itms_services_url(plist_url),
            "openExternal": True,
            "autoInstall": args.auto_install,
            "ipaUrl": ipa_url,
            "plistUrl": plist_url,
            "sha256": sha256(ipa_target),
            "size": ipa_target.stat().st_size,
        })

    for value in args.external:
        if "=" not in value:
            raise SystemExit(f"external must be platform=url, got {value!r}")
        platform, url = value.split("=", 1)
        platform = platform.strip()
        add_platform(platform, {
            "url": validate_external_url(platform, url),
            "openExternal": True,
            "autoInstall": args.auto_install,
        })

    if not platforms:
        raise SystemExit("at least one --artifact or --external entry is required")

    changelog = ""
    if args.changelog_file:
        changelog = args.changelog_file.read_text(encoding="utf-8")

    payload = {
        "schema": 1,
        "version": version,
        "releaseDate": args.release_date,
        "changelog": changelog,
        "autoInstall": args.auto_install,
        "platforms": platforms,
    }
    payload_bytes = json.dumps(payload, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")
    manifest = {
        "schema": "amnezia-selfhosted-update-v1",
        "signatureAlgorithm": "Ed25519",
        "payload": b64url(payload_bytes),
        "signature": sign_payload(args.private_key.expanduser().resolve(), payload_bytes),
    }
    (out_dir / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
