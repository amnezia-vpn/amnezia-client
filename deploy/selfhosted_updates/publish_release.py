#!/usr/bin/env python3
"""Build and optionally upload a self-hosted Amnezia update channel release."""

from __future__ import annotations

import argparse
import base64
import json
import os
import re
import shutil
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path
from urllib.error import HTTPError
from urllib.parse import parse_qs, urlparse
from urllib.request import Request, urlopen
from urllib.request import urlretrieve


SCRIPT_DIR = Path(__file__).resolve().parent
MAKE_MANIFEST = SCRIPT_DIR / "make_manifest.py"
INSTALL_HOST = SCRIPT_DIR / "install_server_update_host.sh"

KNOWN_PATTERNS = {
    "windows-x64": "AmneziaVPN_{version}_windows_x64.exe",
    "linux-x64": "AmneziaVPN_{version}_linux_x64.run",
    "macos-x64": "AmneziaVPN_{version}_macos_x64.pkg",
    "android": "AmneziaVPN_{version}_android9+_universal.apk",
    "android-arm64-v8a": "AmneziaVPN_{version}_android9+_arm64-v8a.apk",
    "android-armeabi-v7a": "AmneziaVPN_{version}_android9+_armeabi-v7a.apk",
    "android-x86": "AmneziaVPN_{version}_android9+_x86.apk",
    "android-x86_64": "AmneziaVPN_{version}_android9+_x86_64.apk",
}
KNOWN_PATTERN_ALIASES = {
    "windows-x64": ["AmneziaVPN_{version}_x64.exe"],
    "macos-x64": ["AmneziaVPN_{version}_macos.pkg"],
}
IOS_IPA_PATTERN = "AmneziaVPN_{version}_ios.ipa"
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
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


def sh_quote(value: str) -> str:
    return "'" + value.replace("'", "'\"'\"'") + "'"


def validate_release_version(value: str) -> str:
    normalized = value.strip()
    if not VERSION_RE.fullmatch(normalized):
        raise SystemExit("--version must be a release version in x.y.z.w numeric format")
    return normalized


def parse_platform_values(values: list[str], label: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for value in values:
        if "=" not in value:
            raise SystemExit(f"{label} must be platform=value, got {value!r}")
        platform, item = value.split("=", 1)
        platform = platform.strip()
        item = item.strip()
        if not platform or not item:
            raise SystemExit(f"{label} must contain non-empty platform and value: {value!r}")
        if platform in result:
            raise SystemExit(f"{label} contains duplicate platform: {platform}")
        result[platform] = item
    return result


def artifact_filenames(platform: str, version: str) -> list[str]:
    patterns = [KNOWN_PATTERNS[platform], *KNOWN_PATTERN_ALIASES.get(platform, [])]
    return [pattern.format(version=version) for pattern in patterns]


def discover_artifacts(artifact_dir: Path, version: str) -> dict[str, Path]:
    artifacts: dict[str, Path] = {}
    for platform in KNOWN_PATTERNS:
        for filename in artifact_filenames(platform, version):
            direct_candidate = artifact_dir / filename
            if direct_candidate.is_file():
                artifacts[platform] = direct_candidate.resolve()
                break
            matches = sorted(path for path in artifact_dir.rglob(filename) if path.is_file())
            if matches:
                artifacts[platform] = matches[0].resolve()
                break
    return artifacts


def discover_ios_ipa(artifact_dir: Path, version: str) -> Path | None:
    filename = IOS_IPA_PATTERN.format(version=version)
    direct_candidate = artifact_dir / filename
    if direct_candidate.is_file():
        return direct_candidate.resolve()
    matches = sorted(path for path in artifact_dir.rglob(filename) if path.is_file())
    return matches[0].resolve() if matches else None


def download_known_release_assets(repo: str, version: str, artifact_dir: Path, required_platforms: set[str]) -> None:
    artifact_dir.mkdir(parents=True, exist_ok=True)
    download_filenames = {platform: artifact_filenames(platform, version) for platform in KNOWN_PATTERNS}
    download_filenames["ios"] = [IOS_IPA_PATTERN.format(version=version)]
    for platform, filenames in download_filenames.items():
        if any((artifact_dir / filename).exists() for filename in filenames):
            continue
        last_error: HTTPError | None = None
        for filename in filenames:
            target = artifact_dir / filename
            url = f"https://github.com/{repo}/releases/download/{version}/{filename}"
            print(f"Downloading {url}", flush=True)
            try:
                urlretrieve(url, target)
                last_error = None
                break
            except HTTPError as error:
                last_error = error
                target.unlink(missing_ok=True)
        if last_error:
            if platform in required_platforms:
                raise last_error
            print(f"Skipping missing optional release asset: {' or '.join(filenames)}", flush=True)


def required_release_asset_platforms(
    required_platforms: list[str],
    external_platforms: set[str],
    explicit_artifact_platforms: set[str],
) -> set[str]:
    return set(required_platforms) - external_platforms - explicit_artifact_platforms


def missing_platform_messages(missing_platforms: list[str], artifact_dir: Path, version: str) -> list[str]:
    messages: list[str] = []
    linux_archive = f"AmneziaVPN_{version}_linux_x64.tar"
    has_linux_archive = (artifact_dir / linux_archive).is_file() or any(
        path.is_file() for path in artifact_dir.rglob(linux_archive)
    )
    for platform in missing_platforms:
        if platform == "linux-x64" and has_linux_archive:
            messages.append(
                f"{platform} (found {linux_archive}, but Linux auto-install requires the fork CI .run artifact)"
            )
        else:
            messages.append(platform)
    return messages


def run(command: list[str], *, stdin_path: Path | None = None) -> None:
    if stdin_path:
        with stdin_path.open("rb") as stdin:
            subprocess.run(command, stdin=stdin, check=True)
    else:
        subprocess.run(command, check=True)


def run_capture(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, check=True, text=True, capture_output=True)


def b64url_decode(value: str) -> bytes:
    return base64.urlsafe_b64decode(value + "=" * (-len(value) % 4))


def command_parts(command: str) -> list[str]:
    parts = shlex.split(command)
    if not parts:
        raise SystemExit("empty command")
    return parts


def fetch_github_release_metadata(repo: str, version: str, token: str | None) -> tuple[str, str]:
    request = Request(
        f"https://api.github.com/repos/{repo}/releases/tags/{version}",
        headers={"Accept": "application/vnd.github+json"},
    )
    if token:
        request.add_header("Authorization", f"Bearer {token}")
    with urlopen(request) as response:
        payload = json.loads(response.read().decode("utf-8"))
    return payload.get("published_at", ""), payload.get("body", "") or ""


def is_sha256_hex(value: object) -> bool:
    return isinstance(value, str) and bool(SHA256_RE.fullmatch(value.lower()))


def is_allowed_external_update_url(platform: str, url: str) -> bool:
    parsed = urlparse(url)
    if not parsed.scheme:
        return False
    if platform == "ios":
        if parsed.scheme == "https":
            return bool(parsed.netloc)
        if parsed.scheme == "itms-apps":
            return bool(parsed.netloc)
        if parsed.scheme == "itms-services":
            manifest_urls = parse_qs(parsed.query).get("url", [])
            if not manifest_urls:
                return False
            manifest_url = urlparse(manifest_urls[0])
            return manifest_url.scheme == "https" and bool(manifest_url.netloc)
        return False
    return parsed.scheme in {"http", "https"} and bool(parsed.netloc)


def verify_public_key_matches_private(public_key_base64: str, private_key: Path) -> None:
    if any(ch.isspace() for ch in public_key_base64):
        raise SystemExit("SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64 must not contain whitespace or line breaks")
    try:
        public_key_pem = base64.b64decode(public_key_base64, validate=True)
    except Exception as exc:
        raise SystemExit("SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64 must be a single-line base64-encoded PEM public key") from exc
    if b"BEGIN PUBLIC KEY" not in public_key_pem:
        raise SystemExit("SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64 must decode to a PEM public key")

    with tempfile.TemporaryDirectory() as tmp:
        tmp_dir = Path(tmp)
        public_key_path = tmp_dir / "public.pem"
        normalized_public_key_path = tmp_dir / "normalized-public.pem"
        derived_public_key_path = tmp_dir / "derived-public.pem"
        public_key_path.write_bytes(public_key_pem)
        try:
            run_capture([
                openssl_command(),
                "pkey",
                "-pubin",
                "-in",
                str(public_key_path),
                "-pubout",
                "-out",
                str(normalized_public_key_path),
            ])
        except subprocess.CalledProcessError as exc:
            raise SystemExit("SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64 must decode to a valid PEM public key") from exc
        try:
            run_capture([openssl_command(), "pkey", "-in", str(private_key), "-pubout", "-out", str(derived_public_key_path)])
        except subprocess.CalledProcessError as exc:
            raise SystemExit("SELFHOSTED_UPDATE_PRIVATE_KEY must be a valid PEM private key") from exc
        if normalized_public_key_path.read_bytes() != derived_public_key_path.read_bytes():
            raise SystemExit("SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64 does not match SELFHOSTED_UPDATE_PRIVATE_KEY")


def publish_files_remote_command(server_dir: str, remote_tmp: str) -> str:
    files_dir = server_dir.rstrip("/") + "/files"
    commands = [
        f"sudo mkdir -p {sh_quote(server_dir)} {sh_quote(files_dir)}",
        (
            f"for f in {sh_quote(remote_tmp + '/files')}/*; do "
            "[ -e \"$f\" ] || continue; "
            "name=${f##*/}; "
            f"sudo cp -a \"$f\" {sh_quote(files_dir)}/\"$name.tmp\"; "
            f"sudo mv -f {sh_quote(files_dir)}/\"$name.tmp\" {sh_quote(files_dir)}/\"$name\"; "
            "done"
        ),
    ]
    return " && ".join(commands)


def publish_manifest_remote_command(server_dir: str, remote_tmp: str) -> str:
    commands = [
        f"sudo cp -a {sh_quote(remote_tmp + '/manifest.json')} {sh_quote(server_dir + '/manifest.json.tmp')}",
        f"sudo mv -f {sh_quote(server_dir + '/manifest.json.tmp')} {sh_quote(server_dir + '/manifest.json')}",
        f"rm -rf {sh_quote(remote_tmp)}",
    ]
    return " && ".join(commands)


def verify_manifest(
    manifest_path: Path,
    private_key: Path,
    expected_version: str,
    required_platforms: set[str],
    auto_install: bool,
) -> None:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schema") != "amnezia-selfhosted-update-v1":
        raise SystemExit(f"Unexpected manifest schema: {manifest.get('schema')!r}")
    if manifest.get("signatureAlgorithm") != "Ed25519":
        raise SystemExit(f"Unexpected manifest signature algorithm: {manifest.get('signatureAlgorithm')!r}")

    payload_bytes = b64url_decode(manifest.get("payload", ""))
    signature = base64.b64decode(manifest.get("signature", ""))
    payload = json.loads(payload_bytes.decode("utf-8"))
    if payload.get("schema") != 1:
        raise SystemExit(f"Unexpected manifest payload schema: {payload.get('schema')!r}")
    if payload.get("version") != expected_version:
        raise SystemExit(
            f"Generated manifest version {payload.get('version')!r} does not match requested version {expected_version!r}"
        )
    platforms = payload.get("platforms", {})
    missing = sorted(required_platforms - set(platforms))
    if missing:
        raise SystemExit("Generated manifest is missing required platforms: " + ", ".join(missing))
    for platform, artifact in platforms.items():
        if not isinstance(artifact, dict):
            raise SystemExit(f"Generated manifest platform {platform} must be an object")
        url = artifact.get("url")
        parsed_url = urlparse(url) if isinstance(url, str) else None
        if not isinstance(url, str) or not parsed_url:
            raise SystemExit(f"Generated manifest platform {platform} is missing a URL")
        sha256 = artifact.get("sha256")
        size = artifact.get("size")
        if artifact.get("openExternal"):
            if not parsed_url.scheme:
                raise SystemExit(f"Generated manifest platform {platform} external URL must be absolute")
            if not is_allowed_external_update_url(platform, url):
                raise SystemExit(f"Generated manifest platform {platform} external URL has unsupported scheme")
            if sha256 is not None and not is_sha256_hex(sha256):
                raise SystemExit(f"Generated manifest platform {platform} has invalid sha256")
            if size is not None and (not isinstance(size, int) or size <= 0):
                raise SystemExit(f"Generated manifest platform {platform} has invalid size")
        else:
            if parsed_url.scheme:
                if parsed_url.scheme not in {"http", "https"} or not parsed_url.netloc:
                    raise SystemExit(f"Generated manifest platform {platform} URL must use http(s)")
            elif not url.startswith("files/") or ".." in Path(url).parts:
                raise SystemExit(f"Generated manifest platform {platform} relative URL must stay under files/")
            if not is_sha256_hex(sha256):
                raise SystemExit(f"Generated manifest platform {platform} is missing or has invalid sha256")
            if not isinstance(size, int) or size <= 0:
                raise SystemExit(f"Generated manifest platform {platform} is missing a positive size")
    if auto_install:
        if payload.get("autoInstall") is not True:
            raise SystemExit("Generated manifest is missing top-level autoInstall=true")
        for platform, artifact in platforms.items():
            if artifact.get("autoInstall") is not True:
                raise SystemExit(f"Generated manifest is missing autoInstall=true for {platform}")

    with tempfile.TemporaryDirectory() as tmp:
        tmp_dir = Path(tmp)
        payload_path = tmp_dir / "payload.json"
        signature_path = tmp_dir / "payload.sig"
        public_key_path = tmp_dir / "public.pem"
        payload_path.write_bytes(payload_bytes)
        signature_path.write_bytes(signature)
        run([openssl_command(), "pkey", "-in", str(private_key), "-pubout", "-out", str(public_key_path)])
        run([
            openssl_command(),
            "pkeyutl",
            "-verify",
            "-rawin",
            "-pubin",
            "-inkey",
            str(public_key_path),
            "-in",
            str(payload_path),
            "-sigfile",
            str(signature_path),
        ])


def upload_release(args: argparse.Namespace, out_dir: Path) -> None:
    remote_tmp = f"/tmp/amnezia-client-updates-{args.version.replace('/', '_')}-{os.getpid()}"
    server_dir = args.server_dir
    ssh = command_parts(args.ssh)
    scp = command_parts(args.scp)

    run(ssh + [args.server, f"rm -rf {sh_quote(remote_tmp)} && mkdir -p {sh_quote(remote_tmp)}"])
    run(scp + [str(out_dir / "manifest.json"), args.server + ":" + remote_tmp + "/manifest.json"])
    run(scp + ["-r", str(out_dir / "files"), args.server + ":" + remote_tmp + "/files"])
    run(ssh + [args.server, publish_files_remote_command(server_dir, remote_tmp)])

    if not args.no_install_host:
        run(ssh + [args.server, f"sh -s -- {sh_quote(server_dir)}"], stdin_path=INSTALL_HOST)

    run(ssh + [args.server, publish_manifest_remote_command(server_dir, remote_tmp)])


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--version", required=True)
    parser.add_argument("--release-date", default="")
    parser.add_argument("--changelog-file", type=Path)
    parser.add_argument("--base-url", default="http://172.29.172.252:17865")
    parser.add_argument("--private-key", type=Path, required=True)
    parser.add_argument("--public-key-base64", default=os.environ.get("SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64", ""))
    parser.add_argument("--artifact-dir", type=Path, default=Path("deploy/build"))
    parser.add_argument("--out-dir", type=Path)
    parser.add_argument("--artifact", action="append", default=[], help="Explicit platform=path, overrides autodiscovery")
    parser.add_argument("--external", action="append", default=[], help="platform=url, for TestFlight/App Store/MDM/etc.")
    parser.add_argument("--include-platform", action="append", default=[], help="Only include these platforms in the generated manifest")
    parser.add_argument("--ios-ipa", type=Path)
    parser.add_argument("--ios-bundle-id", default="org.amnezia.AmneziaVPN")
    parser.add_argument("--ios-bundle-version")
    parser.add_argument("--ios-title", default="AmneziaVPN")
    parser.add_argument("--require-platform", action="append", default=[])
    parser.add_argument("--download-github-release", action="store_true")
    parser.add_argument("--github-release-metadata", action="store_true")
    parser.add_argument("--github-repo", default="amnezia-vpn/amnezia-client")
    parser.add_argument("--github-token", default=os.environ.get("GITHUB_TOKEN"))
    parser.add_argument("--auto-install", action="store_true")
    parser.add_argument("--server", help="SSH target, for example root@203.0.113.10")
    parser.add_argument("--server-dir", default="/opt/amnezia/client-updates")
    parser.add_argument("--ssh", default="ssh")
    parser.add_argument("--scp", default="scp")
    parser.add_argument("--no-install-host", action="store_true")
    args = parser.parse_args()
    args.version = validate_release_version(args.version)

    private_key = args.private_key.expanduser().resolve()
    if args.server and not args.public_key_base64:
        raise SystemExit("--public-key-base64 or SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64 is required when publishing to a server")
    if args.public_key_base64:
        verify_public_key_matches_private(args.public_key_base64, private_key)

    artifact_dir = args.artifact_dir.expanduser().resolve()
    explicit_artifacts = {
        platform: Path(path).expanduser().resolve()
        for platform, path in parse_platform_values(args.artifact, "--artifact").items()
    }
    externals = parse_platform_values(args.external, "--external")
    if args.download_github_release:
        download_known_release_assets(
            args.github_repo,
            args.version,
            artifact_dir,
            required_release_asset_platforms(args.require_platform, set(externals), set(explicit_artifacts)),
        )

    artifacts = discover_artifacts(artifact_dir, args.version)
    artifacts.update(explicit_artifacts)
    ios_ipa = args.ios_ipa.expanduser().resolve() if args.ios_ipa else discover_ios_ipa(artifact_dir, args.version)
    if "ios" in externals:
        ios_ipa = None
    if args.include_platform:
        included_platforms = set(args.include_platform)
        artifacts = {
            platform: path
            for platform, path in artifacts.items()
            if platform in included_platforms
        }
        externals = {
            platform: url
            for platform, url in externals.items()
            if platform in included_platforms
        }
        if "ios" not in included_platforms:
            ios_ipa = None

    available_platforms = set(artifacts) | set(externals)
    if ios_ipa:
        available_platforms.add("ios")

    missing = [platform for platform in args.require_platform if platform not in available_platforms]
    if ios_ipa and not args.ios_bundle_id:
        missing.append("ios-bundle-id")
    if missing:
        raise SystemExit(
            "Missing required update artifacts/settings: "
            + ", ".join(missing_platform_messages(missing, artifact_dir, args.version))
        )

    out_dir = (args.out_dir or Path("dist") / "selfhosted-updates" / args.version).expanduser().resolve()
    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True)

    generated_changelog: Path | None = None
    release_date = args.release_date
    if args.github_release_metadata:
        metadata_release_date, metadata_changelog = fetch_github_release_metadata(
            args.github_repo, args.version, args.github_token
        )
        if not release_date:
            release_date = metadata_release_date
        if not args.changelog_file and metadata_changelog:
            generated_changelog = out_dir / ".github-release-changelog.txt"
            generated_changelog.write_text(metadata_changelog.replace("\r", ""), encoding="utf-8")

    command = [
        sys.executable,
        str(MAKE_MANIFEST),
        "--version",
        args.version,
        "--release-date",
        release_date,
        "--base-url",
        args.base_url,
        "--private-key",
        str(private_key),
        "--out-dir",
        str(out_dir),
    ]
    changelog_file = args.changelog_file or generated_changelog
    if changelog_file:
        command += ["--changelog-file", str(changelog_file.expanduser().resolve())]
    if args.auto_install:
        command.append("--auto-install")
    for platform, path in sorted(artifacts.items()):
        command += ["--artifact", f"{platform}={path}"]
    for platform, url in externals.items():
        command += ["--external", f"{platform}={url}"]
    if ios_ipa:
        command += [
            "--ios-ipa",
            str(ios_ipa),
            "--ios-bundle-id",
            args.ios_bundle_id,
            "--ios-title",
            args.ios_title,
        ]
        if args.ios_bundle_version:
            command += ["--ios-bundle-version", args.ios_bundle_version]

    run(command)
    verify_manifest(out_dir / "manifest.json", private_key, args.version, set(args.require_platform), args.auto_install)

    print("Published manifest platforms:", ", ".join(sorted(available_platforms)), flush=True)
    print("Verified self-hosted update manifest signature and required platforms", flush=True)
    print(f"Output: {out_dir}", flush=True)

    if args.server:
        upload_release(args, out_dir)
        print(f"Uploaded to {args.server}:{args.server_dir}", flush=True)

    return 0


if __name__ == "__main__":
    sys.exit(main())
