#!/usr/bin/env python3
from __future__ import annotations

import base64
import contextlib
import hashlib
import io
import json
import os
import plistlib
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[1]
sys.path.insert(0, str(SCRIPT_DIR))

import publish_release  # noqa: E402
import release_freeze  # noqa: E402
import make_manifest  # noqa: E402


def find_openssl() -> str | None:
    candidates = [
        shutil.which("openssl"),
        r"C:\Program Files\Git\usr\bin\openssl.exe",
        r"C:\Program Files\Git\mingw64\bin\openssl.exe",
    ]
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return str(candidate)
    return None


def find_sh() -> str | None:
    candidates = [
        shutil.which("sh"),
        r"C:\Program Files\Git\bin\sh.exe",
        r"C:\Program Files\Git\usr\bin\sh.exe",
    ]
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return str(candidate)
    return None


def find_powershell() -> str | None:
    candidates = [
        shutil.which("pwsh"),
        shutil.which("powershell"),
        r"C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe",
    ]
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return str(candidate)
    return None


def find_wsl() -> str | None:
    return shutil.which("wsl.exe") or shutil.which("wsl")


def to_wsl_path(path: Path) -> str:
    wsl = find_wsl()
    if not wsl:
        raise unittest.SkipTest("WSL is required")
    result = subprocess.run([wsl, "wslpath", "-a", str(path).replace("\\", "/")], text=True, capture_output=True)
    if result.returncode != 0:
        raise AssertionError(result.stderr + result.stdout)
    return result.stdout.strip()


def find_git() -> str | None:
    return shutil.which("git")


def run_git(cwd: Path, *args: str, stdout: object | None = None) -> subprocess.CompletedProcess[str]:
    command = [find_git() or "git", *args]
    return subprocess.run(command, cwd=cwd, check=True, text=True, stdout=stdout, stderr=subprocess.PIPE)


def manifest_payload(manifest_path: Path) -> dict[str, object]:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    payload_bytes = base64.urlsafe_b64decode(manifest["payload"] + "=" * (-len(manifest["payload"]) % 4))
    return json.loads(payload_bytes.decode("utf-8"))


def sha256_hex_for_text(value: str) -> str:
    return hashlib.sha256(value.encode("utf-8")).hexdigest()


def planned_action(args: object) -> str:
    output = io.StringIO()
    with contextlib.redirect_stdout(output):
        release_freeze.plan(args)
    return json.loads(output.getvalue())["action"]


def assert_no_duplicate_yaml_keys(test_case: unittest.TestCase, path: Path) -> None:
    stack: list[tuple[int, set[str]]] = []
    for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        if not raw_line.strip() or raw_line.lstrip().startswith("#"):
            continue
        if raw_line.lstrip() != raw_line:
            indent = len(raw_line) - len(raw_line.lstrip(" "))
        else:
            indent = 0
        if raw_line[indent:].startswith("- "):
            continue
        stripped = raw_line.strip()
        if ":" not in stripped or stripped.startswith(("|", ">")):
            continue
        key = stripped.split(":", 1)[0].strip().strip("'\"")
        if not key or " " in key:
            continue
        while stack and stack[-1][0] >= indent:
            stack.pop()
        if not stack or stack[-1][0] != indent:
            stack.append((indent, set()))
        keys = stack[-1][1]
        test_case.assertNotIn(key, keys, f"Duplicate YAML key {key!r} in {path}:{line_number}")
        keys.add(key)


def read_workflow_if_enabled(path: Path) -> str:
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8")


def shell_absolute_path(path: Path) -> str:
    resolved = path.resolve()
    if os.name != "nt":
        return str(resolved)
    drive = resolved.drive.rstrip(":").lower()
    tail = resolved.as_posix().split(":", 1)[1].lstrip("/")
    return f"/{drive}/{tail}"


class ReleaseFreezeTests(unittest.TestCase):
    def test_plan_wait_freeze_and_advance_after_frozen(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            state_path = Path(tmp) / "state.json"
            base_args = {
                "state_file": state_path,
                "upstream_repo": "amnezia-vpn/amnezia-client",
                "target_branch": "feat/server-managed-split-tunnel",
                "baseline_tag": "4.8.16.0",
            }

            self.assertFalse(release_freeze.is_newer("4.8.16.0", "4.8.16.0"))
            self.assertTrue(release_freeze.is_newer("4.8.17.0", "4.8.16.0"))

            wait_args = type("Args", (), {**base_args, "latest_tag": "4.8.16.0", "force_freeze_tag": ""})()
            freeze_args = type("Args", (), {**base_args, "latest_tag": "4.8.17.0", "force_freeze_tag": ""})()

            wait_state = release_freeze.read_state(state_path)
            self.assertFalse(wait_state["frozen"])
            self.assertEqual(planned_action(wait_args), "wait")
            self.assertEqual(planned_action(freeze_args), "freeze")

            record_args = type(
                "Args",
                (),
                {
                    **base_args,
                    "latest_tag": "4.8.17.0",
                    "action": "freeze",
                    "release_tag": "4.8.17.0",
                    "release_sha": "deadbeef",
                    "upstream_dev_sha": "cafebabe",
                },
            )()
            self.assertEqual(release_freeze.record(record_args), 0)
            frozen_state = release_freeze.read_state(state_path)
            self.assertTrue(frozen_state["frozen"])
            self.assertEqual(frozen_state["frozenTag"], "4.8.17.0")
            self.assertEqual(frozen_state["baselineTag"], "4.8.17.0")
            frozen_args = type("Args", (), {**base_args, "latest_tag": "4.8.17.0", "force_freeze_tag": ""})()
            next_release_args = type("Args", (), {**base_args, "latest_tag": "4.8.18.0", "force_freeze_tag": ""})()
            self.assertEqual(planned_action(frozen_args), "already-frozen")
            self.assertEqual(planned_action(next_release_args), "freeze")

    def test_plan_rejects_invalid_release_tags(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            state_path = Path(tmp) / "state.json"
            base_args = {
                "state_file": state_path,
                "upstream_repo": "amnezia-vpn/amnezia-client",
                "target_branch": "feat/server-managed-split-tunnel",
                "baseline_tag": "4.8.16.0",
            }

            with self.assertRaises(SystemExit):
                planned_action(type("Args", (), {**base_args, "latest_tag": "dev", "force_freeze_tag": ""})())

            with self.assertRaises(SystemExit):
                planned_action(type("Args", (), {**base_args, "latest_tag": "4.8.16.0", "force_freeze_tag": "release"})())

    def test_plan_rejects_baseline_newer_than_latest_release(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            state_path = Path(tmp) / "state.json"
            base_args = {
                "state_file": state_path,
                "upstream_repo": "amnezia-vpn/amnezia-client",
                "target_branch": "feat/server-managed-split-tunnel",
                "baseline_tag": "4.8.16.0",
                "latest_tag": "4.8.15.4",
            }
            args = type(
                "Args",
                (),
                {
                    **base_args,
                    "force_freeze_tag": "",
                },
            )()

            with self.assertRaises(SystemExit) as context:
                planned_action(args)
            self.assertIn("is newer than --latest-tag", str(context.exception))

            forced_args = type("Args", (), {**base_args, "force_freeze_tag": "4.8.16.0"})()
            self.assertEqual(planned_action(forced_args), "freeze")

    @unittest.skipUnless(find_git(), "git is required to exercise release freeze patch semantics")
    def test_patch_based_freeze_does_not_keep_post_release_upstream_commits(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)
            run_git(repo, "init", "-q")
            run_git(repo, "config", "user.name", "test")
            run_git(repo, "config", "user.email", "test@example.invalid")

            (repo / "app.txt").write_text("release-base\n", encoding="utf-8")
            run_git(repo, "add", ".")
            run_git(repo, "commit", "-q", "-m", "release base")
            run_git(repo, "tag", "4.8.17.0")

            run_git(repo, "checkout", "-q", "-b", "upstream/dev")
            (repo / "app.txt").write_text("post-release-upstream\n", encoding="utf-8")
            (repo / "post-release-only.txt").write_text("must not be retained\n", encoding="utf-8")
            run_git(repo, "add", ".")
            run_git(repo, "commit", "-q", "-m", "post release upstream dev")

            run_git(repo, "checkout", "-q", "-b", "feat/server-managed-split-tunnel")
            (repo / "fork-feature.txt").write_text("self-hosted updater\n", encoding="utf-8")
            run_git(repo, "add", ".")
            run_git(repo, "commit", "-q", "-m", "fork feature")

            patch_path = repo / "fork.patch"
            with patch_path.open("w", encoding="utf-8") as stream:
                run_git(repo, "diff", "--binary", "upstream/dev...HEAD", stdout=stream)

            run_git(repo, "checkout", "-q", "-B", "feat/server-managed-split-tunnel", "refs/tags/4.8.17.0")
            run_git(repo, "apply", "--index", "--3way", str(patch_path))
            run_git(repo, "commit", "-q", "-m", "apply fork patch to release")

            self.assertEqual((repo / "app.txt").read_text(encoding="utf-8"), "release-base\n")
            self.assertEqual((repo / "fork-feature.txt").read_text(encoding="utf-8"), "self-hosted updater\n")
            self.assertFalse((repo / "post-release-only.txt").exists())


class SourceContractTests(unittest.TestCase):
    def test_manifest_url_validation_rejects_cidr_routes(self) -> None:
        self.assertEqual(make_manifest.validate_release_version(" 4.8.16.0 "), "4.8.16.0")
        self.assertEqual(publish_release.validate_release_version("4.8.16.0"), "4.8.16.0")
        for invalid_version in ("4.8.16", "4.8.16.0-beta", "release", "4.8.16.0\nnext"):
            with self.assertRaises(SystemExit):
                make_manifest.validate_release_version(invalid_version)
            with self.assertRaises(SystemExit):
                publish_release.validate_release_version(invalid_version)

        self.assertEqual(
            make_manifest.validate_base_url("http://172.29.172.252:17865/"),
            "http://172.29.172.252:17865",
        )
        self.assertEqual(
            make_manifest.validate_base_url("https://updates.example.invalid/1"),
            "https://updates.example.invalid/1",
        )
        for invalid_base_url in (
            "https://user:pass@updates.example.invalid",
            "https://updates.example.invalid/update?token=secret",
            "https://updates.example.invalid/update#manifest",
        ):
            with self.assertRaises(SystemExit):
                make_manifest.validate_base_url(invalid_base_url)
        with self.assertRaises(SystemExit) as no_scheme:
            make_manifest.validate_base_url("10.8.1.0/1")
        self.assertIn("http(s) endpoint URL", str(no_scheme.exception))

        with self.assertRaises(SystemExit) as cidr_path:
            make_manifest.validate_base_url("http://10.8.1.0/1")
        self.assertIn("not a CIDR route", str(cidr_path.exception))

        with self.assertRaises(SystemExit) as relative_external:
            make_manifest.validate_external_url("ios", "/files/app.plist")
        self.assertIn("must be absolute", str(relative_external.exception))

        with self.assertRaises(SystemExit) as ios_http_external:
            make_manifest.validate_external_url(
                "ios",
                "itms-services://?action=download-manifest&url=http%3A%2F%2F172.29.172.252%3A17865%2Ffiles%2Fapp.plist",
            )
        self.assertIn("must use HTTPS", str(ios_http_external.exception))

        self.assertEqual(
            make_manifest.validate_external_url(
                "ios",
                "itms-services://?action=download-manifest&url=https%3A%2F%2Fupdates.example.invalid%2Ffiles%2Fapp.plist",
            ),
            "itms-services://?action=download-manifest&url=https%3A%2F%2Fupdates.example.invalid%2Ffiles%2Fapp.plist",
        )
        self.assertEqual(
            make_manifest.validate_external_url("ios", "itms-apps://apps.apple.com/app/id123456789"),
            "itms-apps://apps.apple.com/app/id123456789",
        )
        with self.assertRaises(SystemExit) as ios_itms_apps_without_host:
            make_manifest.validate_external_url("ios", "itms-apps:///app/id123456789")
        self.assertIn("must include a host", str(ios_itms_apps_without_host.exception))

        with self.assertRaises(SystemExit) as ios_itms_services_without_manifest_host:
            make_manifest.validate_external_url(
                "ios",
                "itms-services://?action=download-manifest&url=https%3A%2F%2F%2Ffiles%2Fapp.plist",
            )
        self.assertIn("must use HTTPS with a host", str(ios_itms_services_without_manifest_host.exception))

        with self.assertRaises(SystemExit) as ios_http_base:
            make_manifest.require_https_base_url_for_ios_ota("http://172.29.172.252:17865")
        self.assertIn("--ios-ipa requires --base-url to use HTTPS", str(ios_http_base.exception))

        with self.assertRaises(SystemExit) as android_file_external:
            make_manifest.validate_external_url("android", "file:///tmp/AmneziaVPN.apk")
        self.assertIn("must use HTTP or HTTPS", str(android_file_external.exception))
        self.assertEqual(
            make_manifest.validate_external_url("android", "https://updates.example.invalid/files/AmneziaVPN.apk"),
            "https://updates.example.invalid/files/AmneziaVPN.apk",
        )

    def test_manifest_tool_rejects_duplicate_platforms(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            first = root / "first.exe"
            second = root / "second.exe"
            first.write_bytes(b"first")
            second.write_bytes(b"second")

            with self.assertRaises(SystemExit) as duplicate_artifact:
                make_manifest.parse_artifact([
                    f"windows-x64={first}",
                    f"windows-x64={second}",
                ])
            self.assertIn("duplicate artifact platform: windows-x64", str(duplicate_artifact.exception))

            with self.assertRaises(SystemExit) as duplicate_publish_value:
                publish_release.parse_platform_values(
                    [
                        f"windows-x64={first}",
                        f"windows-x64={second}",
                    ],
                    "--artifact",
                )
            self.assertIn("duplicate platform: windows-x64", str(duplicate_publish_value.exception))

    def test_manifest_tool_rejects_duplicate_output_filenames(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            first_dir = root / "first"
            second_dir = root / "second"
            first_dir.mkdir()
            second_dir.mkdir()
            first = first_dir / "AmneziaVPN.bin"
            second = second_dir / "AmneziaVPN.bin"
            first.write_bytes(b"first")
            second.write_bytes(b"second")

            old_argv = sys.argv
            sys.argv = [
                "make_manifest.py",
                "--version",
                "9.9.9.9",
                "--base-url",
                "https://updates.example.invalid",
                "--private-key",
                str(root / "missing.pem"),
                "--artifact",
                f"windows-x64={first}",
                "--artifact",
                f"linux-x64={second}",
                "--out-dir",
                str(root / "out"),
            ]
            try:
                with self.assertRaises(SystemExit) as duplicate_output:
                    make_manifest.main()
            finally:
                sys.argv = old_argv
            self.assertIn("duplicate artifact output filename", str(duplicate_output.exception))

    def test_android_apk_install_handoff_controls_auto_install_marker(self) -> None:
        activity = (REPO_ROOT / "client/android/src/org/amnezia/vpn/AmneziaActivity.kt").read_text(encoding="utf-8")
        android_controller_cpp = (REPO_ROOT / "client/platforms/android/android_controller.cpp").read_text(encoding="utf-8")
        android_controller_h = (REPO_ROOT / "client/platforms/android/android_controller.h").read_text(encoding="utf-8")
        update_controller = (REPO_ROOT / "client/core/controllers/updateController.cpp").read_text(encoding="utf-8")
        update_controller_h = (REPO_ROOT / "client/core/controllers/updateController.h").read_text(encoding="utf-8")
        signal_handlers = (REPO_ROOT / "client/core/controllers/coreSignalHandlers.cpp").read_text(encoding="utf-8")
        client_cmake = (REPO_ROOT / "client/CMakeLists.txt").read_text(encoding="utf-8")
        client_3rdparty_cmake = (REPO_ROOT / "client/cmake/3rdparty.cmake").read_text(encoding="utf-8")
        openssl_recipe = (REPO_ROOT / "recipes/openssl/conanfile.py").read_text(encoding="utf-8")

        qt_android_controller = (REPO_ROOT / "client/android/src/org/amnezia/vpn/qt/QtAndroidController.kt").read_text(encoding="utf-8")

        self.assertIn("private const val APK_INSTALL_FAILED = 0", activity)
        self.assertIn("private const val APK_INSTALL_STARTED = 1", activity)
        self.assertIn("private const val APK_INSTALL_PERMISSION_SETTINGS_OPENED = 2", activity)
        self.assertIn("fun installApk(fileName: String): Int", activity)
        self.assertIn("private fun startApkInstaller(fileName: String, openSettingsIfBlocked: Boolean): Int", activity)
        self.assertIn("pendingInstallApkPath?.let { outState.putString(KEY_PENDING_INSTALL_APK_PATH, it) }", activity)
        self.assertIn("pendingInstallApkPath != null && !installApkDeliveryScheduled && canRequestPackageInstall()", activity)
        self.assertIn("Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES", activity)
        self.assertIn("return APK_INSTALL_PERMISSION_SETTINGS_OPENED", activity)
        self.assertIn("QtAndroidController.onApkInstallerStarted(fileName)", activity)
        self.assertIn("external fun onApkInstallerStarted(fileName: String)", qt_android_controller)
        self.assertIn("int installApk(const QString &fileName);", android_controller_h)
        self.assertIn("void apkInstallerStarted(QString fileName);", android_controller_h)
        self.assertIn('callActivityMethod<jint>("installApk", "(Ljava/lang/String;)I"', android_controller_cpp)
        self.assertIn('{"onApkInstallerStarted", "(Ljava/lang/String;)V"', android_controller_cpp)
        self.assertIn("emit AndroidController::instance()->apkInstallerStarted", android_controller_cpp)
        self.assertIn("InstallerHandoffResult", update_controller_h)
        self.assertIn("m_androidApkInstallPermissionPending || !m_appSettingsRepository", update_controller)
        self.assertIn("kAndroidApkInstallPermissionWaitMs", update_controller)
        self.assertIn("kAndroidApkInstallPermissionSettingsOpened", update_controller)
        self.assertIn("InstallerHandoffResult::PendingPermission", update_controller)
        self.assertIn("void UpdateController::onAndroidApkInstallerStarted", update_controller)
        self.assertIn("bool isSha256Hex(const QString &value)", update_controller)
        self.assertIn("Self-hosted update artifact is missing or has invalid sha256", update_controller)
        self.assertIn("bool isHttpOrHttpsUrl(const QUrl &url)", update_controller)
        self.assertIn("Self-hosted update artifact URL must use http(s)", update_controller)
        self.assertIn("Self-hosted update artifact is missing or has invalid size", update_controller)
        self.assertIn("!artifact.openExternally && artifact.size <= 0", update_controller)
        self.assertIn("bool isAllowedExternalUpdateUrl(const QUrl &url)", update_controller)
        self.assertIn("#if defined(Q_OS_IOS)\n        if (scheme == QStringLiteral(\"http\")) {\n            return false;\n        }\n#endif", update_controller)
        self.assertIn("#include <QUrlQuery>", update_controller)
        self.assertIn("const QUrlQuery query(url);", update_controller)
        self.assertIn('query.queryItemValue(QStringLiteral("url"))', update_controller)
        self.assertIn('manifestUrl.scheme().toLower() == QStringLiteral("https")', update_controller)
        self.assertIn('if (scheme == QStringLiteral("itms-apps")) {\n            return !url.host().isEmpty();\n        }', update_controller)
        self.assertIn("#else\n        return false;\n#endif", update_controller)
        self.assertIn("bool decodeStrictBase64", update_controller)
        self.assertIn("Self-hosted external update URL has unsupported scheme", update_controller)
        self.assertIn("Update URL has unsupported external scheme", update_controller)
        self.assertIn("constexpr int kInstallerTransferTimeoutMs = 30 * 60 * 1000;", update_controller)
        self.assertIn('installerPath + QStringLiteral(".download")', update_controller)
        self.assertIn("&QIODevice::readyRead", update_controller)
        self.assertIn("hash->addData(chunk);", update_controller)
        self.assertIn("const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();", update_controller)
        self.assertIn("reply->error() != QNetworkReply::NoError || statusCode < 200 || statusCode >= 300", update_controller)
        self.assertIn("Self-hosted installer size differs from manifest", update_controller)
        self.assertIn("if (m_selectedArtifact.size >= 0 && *bytesWritten != m_selectedArtifact.size) {\n            logger.error()", update_controller)
        self.assertIn("QFile::rename(partialPath, installerPath)", update_controller)
        self.assertNotIn("expectedSha256Matches", update_controller + update_controller_h)
        self.assertIn("startBackgroundUpdateChecks();", update_controller)
        self.assertIn("QTimer::singleShot(kInitialBackgroundUpdateCheckMs, this, &UpdateController::checkForUpdates);", update_controller)
        self.assertIn("m_backgroundUpdateTimer->setInterval(kBackgroundUpdateCheckIntervalMs);", update_controller)
        self.assertIn("QTimer* m_backgroundUpdateTimer", update_controller_h)
        self.assertIn("bool m_selfHostedInstallInProgress = false;", update_controller_h)
        self.assertIn("bool m_androidApkInstallPermissionPending = false;", update_controller_h)
        self.assertIn("void finishSelfHostedInstallerAttempt(InstallerHandoffResult result);", update_controller_h)
        self.assertIn("#include <QDate>", update_controller)
        self.assertIn("QString selfHostedAutoInstallAttemptMarker() const;", update_controller_h)
        self.assertIn("QString UpdateController::selfHostedAutoInstallAttemptMarker() const", update_controller)
        self.assertIn("QDate::currentDate().toString(Qt::ISODate)", update_controller)
        self.assertIn("const QString attemptMarker = selfHostedAutoInstallAttemptMarker();", update_controller)
        self.assertIn("selfHostedUpdateLastAutoInstallAttempt() != attemptMarker", update_controller)
        self.assertIn("m_pendingAutoInstallAttemptId = selfHostedAutoInstallAttemptMarker();", update_controller)
        self.assertIn("m_updateCheckRunning || m_selfHostedInstallInProgress || m_androidApkInstallPermissionPending || !m_appSettingsRepository", update_controller)
        self.assertIn("Self-hosted update installer handoff is already in progress", update_controller)
        self.assertIn("m_selfHostedInstallInProgress = true;", update_controller)
        self.assertIn("void UpdateController::finishSelfHostedInstallerAttempt(InstallerHandoffResult result)", update_controller)
        self.assertIn("m_selfHostedInstallInProgress = false;", update_controller)
        self.assertIn("isSelfHostedUpdateChannelConfigured()", update_controller)
        self.assertIn("if (isSelfHostedUpdateChannelConfigured())", update_controller)
        self.assertIn('add_definitions(-DSELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64="$ENV{SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64}")', client_cmake)
        self.assertIn("SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64", update_controller)
        self.assertIn("bool decodeStrictBase64(const QByteArray &encoded, QByteArray::Base64Options options, QByteArray &decoded)", update_controller)
        self.assertIn("QByteArray::fromBase64Encoding(\n                encoded, options | QByteArray::AbortOnBase64DecodingErrors)", update_controller)
        self.assertIn("signature.size() != 64", update_controller)
        self.assertIn("Self-hosted update manifest payload is too large", update_controller)
        self.assertIn("decodeStrictBase64(QByteArray(SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64)", update_controller)
        self.assertNotIn("QByteArray::fromBase64(QByteArray(SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64))", update_controller)
        self.assertIn("find_package(OpenSSL REQUIRED)", client_3rdparty_cmake)
        self.assertIn("OpenSSL::Crypto", client_3rdparty_cmake)
        self.assertIn('self.options.shared and self.settings.os == "Android"', openssl_recipe)
        self.assertIn('self.cpp_info.components["ssl"].libs = ["ssl_3"]', openssl_recipe)
        self.assertIn('self.cpp_info.components["crypto"].libs = ["crypto_3"]', openssl_recipe)
        self.assertIn("target_link_libraries(${PROJECT} PRIVATE ${LIBS})", client_cmake)
        self.assertIn('endpoint.contains(QStringLiteral("://"))', update_controller)
        self.assertIn('#include "core/utils/constants/configKeys.h"', update_controller)
        self.assertIn("serverJson.value(configKey::serverRoutingRulesSyncHost).toString()", update_controller)
        self.assertIn("normalizedSelfHostedManifestUrl", update_controller)
        self.assertIn("url.setPath(path + manifestPath)", update_controller)
        self.assertIn("url.setPort(amnezia::protocols::selfHostedUpdates::syncPort);", update_controller)
        self.assertIn("url.setPath(normalizedPath);", update_controller)
        self.assertNotIn('return QStringLiteral("http://%1:%2%3")', update_controller)
        self.assertNotIn("if (manifestUrls.isEmpty()) {\n        fetchGatewayUrl();", update_controller)
        self.assertIn("if (manifestUrls.isEmpty()) {\n        finishUpdateCheck();", update_controller)
        self.assertIn("if (urlIndex < 0 || urlIndex >= manifestUrls.size()) {\n        finishUpdateCheck();", update_controller)
        self.assertIn("scheduleDesktopQuitAfterInstallerStart();", update_controller)
        self.assertIn("amnApp->forceQuit();", update_controller)
        self.assertIn("kDesktopQuitAfterInstallerStartMs", update_controller)
        self.assertIn("if (runWindowsInstaller(installerPath) == 0) {\n                scheduleDesktopQuitAfterInstallerStart();", update_controller)
        self.assertIn("if (runMacInstaller(installerPath) == 0) {\n                scheduleDesktopQuitAfterInstallerStart();", update_controller)
        self.assertIn("if (runLinuxInstaller(installerPath) == 0) {\n                scheduleDesktopQuitAfterInstallerStart();", update_controller)
        self.assertIn("ConnectionController::connectionStateChanged", signal_handlers)
        self.assertIn("state != Vpn::ConnectionState::Connected", signal_handlers)
        self.assertIn("QTimer::singleShot(5000, m_coreController->m_updateController, &UpdateController::checkForUpdates);", signal_handlers)

    def test_selfhosted_publish_defaults_to_local_non_apple_platforms(self) -> None:
        workflow_paths = (
            REPO_ROOT / ".github/workflows/deploy.yml",
            REPO_ROOT / ".github/workflows/upstream-release-freeze.yml",
            REPO_ROOT / ".github/workflows/tag-deploy.yml",
        )
        for workflow_path in workflow_paths:
            if workflow_path.exists():
                assert_no_duplicate_yaml_keys(self, workflow_path)

        deploy_workflow = read_workflow_if_enabled(REPO_ROOT / ".github/workflows/deploy.yml")
        freeze_workflow = read_workflow_if_enabled(REPO_ROOT / ".github/workflows/upstream-release-freeze.yml")
        tag_deploy_workflow = read_workflow_if_enabled(REPO_ROOT / ".github/workflows/tag-deploy.yml")
        readme = (REPO_ROOT / "deploy/selfhosted_updates/README.md").read_text(encoding="utf-8")
        gitignore = (REPO_ROOT / ".gitignore").read_text(encoding="utf-8")

        if deploy_workflow:
            self.assertNotIn("SELFHOSTED_", deploy_workflow)
            self.assertNotIn("Publish-Selfhosted-Updates", deploy_workflow)
            self.assertNotIn("Validate-Selfhosted-Inputs", deploy_workflow)
            self.assertNotIn("publish_release.py", deploy_workflow)
            self.assertNotIn("default: 'windows-x64 linux-x64 macos-x64 ios", deploy_workflow)
        if tag_deploy_workflow:
            self.assertNotIn("SELFHOSTED_", tag_deploy_workflow)
        self.assertFalse((REPO_ROOT / ".github/workflows/selfhosted-update-publish.yml").exists())
        local_release = (REPO_ROOT / "deploy/selfhosted_updates/local_release.ps1").read_text(encoding="utf-8")
        setup_release = (REPO_ROOT / "deploy/selfhosted_updates/setup_release_workstation.ps1").read_text(encoding="utf-8")
        build_bat = (REPO_ROOT / "deploy/build.bat").read_text(encoding="utf-8")
        build_sh = (REPO_ROOT / "deploy/build.sh").read_text(encoding="utf-8")
        platform_settings = (REPO_ROOT / "cmake/platform_settings.cmake").read_text(encoding="utf-8")
        android_cmake = (REPO_ROOT / "client/cmake/android.cmake").read_text(encoding="utf-8")
        android_gradle = (REPO_ROOT / "client/android/build.gradle.kts").read_text(encoding="utf-8")
        client_cmake = (REPO_ROOT / "client/CMakeLists.txt").read_text(encoding="utf-8")
        protocol_constants = (REPO_ROOT / "client/core/utils/constants/protocolConstants.h").read_text(encoding="utf-8")
        update_controller = (REPO_ROOT / "client/core/controllers/updateController.cpp").read_text(encoding="utf-8")
        update_controller_h = (REPO_ROOT / "client/core/controllers/updateController.h").read_text(encoding="utf-8")
        update_ui_controller = (REPO_ROOT / "client/ui/controllers/updateUiController.cpp").read_text(encoding="utf-8")
        update_ui_controller_h = (REPO_ROOT / "client/ui/controllers/updateUiController.h").read_text(encoding="utf-8")
        connection_ui_controller = (REPO_ROOT / "client/ui/controllers/connectionUiController.cpp").read_text(encoding="utf-8")
        about_page = (REPO_ROOT / "client/ui/qml/Pages2/PageSettingsAbout.qml").read_text(encoding="utf-8")
        qif_component_script = (REPO_ROOT / "deploy/installer/qif/componentscript.js").read_text(encoding="utf-8")
        bootstrapper = (REPO_ROOT / "client/core/controllers/selfhosted/selfHostedUpdateBootstrapper.cpp").read_text(encoding="utf-8")
        bootstrapper_h = (REPO_ROOT / "client/core/controllers/selfhosted/selfHostedUpdateBootstrapper.h").read_text(encoding="utf-8")
        core_controller = (REPO_ROOT / "client/core/controllers/coreController.cpp").read_text(encoding="utf-8")
        core_signal_handlers = (REPO_ROOT / "client/core/controllers/coreSignalHandlers.cpp").read_text(encoding="utf-8")
        app_cpp = (REPO_ROOT / "client/amneziaApplication.cpp").read_text(encoding="utf-8")
        app_h = (REPO_ROOT / "client/amneziaApplication.h").read_text(encoding="utf-8")
        main_cpp = (REPO_ROOT / "client/main.cpp").read_text(encoding="utf-8")
        ssh_session_h = (REPO_ROOT / "client/core/utils/selfhosted/sshSession.h").read_text(encoding="utf-8")
        ssh_session_cpp = (REPO_ROOT / "client/core/utils/selfhosted/sshSession.cpp").read_text(encoding="utf-8")
        server_scripts_qrc = (REPO_ROOT / "client/server_scripts/serverScripts.qrc").read_text(encoding="utf-8")
        self.assertIn("local_release.ps1", readme)
        self.assertIn("setup_release_workstation.ps1", readme)
        self.assertIn("dist/selfhosted-release-env.ps1", gitignore)
        self.assertIn("dist/selfhosted-local-artifacts/", gitignore)
        self.assertIn("dist/selfhosted-updates/", gitignore)
        self.assertIn("dist/selfhosted-windows-client/", gitignore)
        self.assertIn("aqtinstall.log", gitignore)
        self.assertIn("*.jks", gitignore)
        self.assertIn("*.keystore", gitignore)
        self.assertIn("android-release-keystore.env.ps1", gitignore)
        self.assertIn("selfhosted-update-private.pem", gitignore)
        self.assertIn("get_android_toolchain_dir", build_sh)
        self.assertIn('$QT_ROOT_PATH/android/lib/cmake/Qt6/qt.toolchain.cmake', build_sh)
        self.assertIn('"-o=openssl/*:no_asm=True"', platform_settings)
        self.assertIn('WIN32 AND (CONAN_NO_REMOTE', platform_settings)
        self.assertIn("AMNEZIA_BUILD_JOBS_STRIPPED", platform_settings)
        self.assertIn('MATCHES "^[1-9][0-9]*$"', platform_settings)
        self.assertIn('set "AMNEZIA_BUILD_JOBS=%BUILD_JOBS%"', build_bat)
        self.assertIn('export AMNEZIA_BUILD_JOBS="$BUILD_JOBS"', build_sh)
        self.assertIn("CL_MPCount=%BUILD_JOBS%", build_bat)
        self.assertIn("qt_internal_android_armeabi-v7a_configure", build_sh)
        self.assertIn("qt_internal_android_x86_configure", build_sh)
        self.assertIn("qt_internal_android_x86_64_configure", build_sh)
        self.assertIn("libxray-aar-copy.lock", android_cmake)
        self.assertIn("configure_file(${AMNEZIA_LIBXRAY_PATH}", android_cmake)
        self.assertIn("SELFHOSTED_UPDATE_SYNC_HOST", client_cmake)
        self.assertIn("SELFHOSTED_UPDATE_BUNDLE_DIR", client_cmake)
        self.assertIn('DESTINATION "selfhosted_updates"', client_cmake)
        self.assertIn("QT_ANDROID_SHADERTOOLS_LIB", android_cmake)
        self.assertIn("QT_ANDROID_EXTRA_LIBS", android_cmake)
        self.assertIn("Resolve-AndroidShaderToolsLib", local_release)
        self.assertIn("#define SELFHOSTED_UPDATE_SYNC_HOST", protocol_constants)
        self.assertNotIn('QStringLiteral("macos-', update_controller)
        self.assertNotIn('QStringLiteral("ios-', update_controller)
        self.assertIn("SELFHOSTED_UPDATE_SYNC_HOST", readme)
        self.assertNotIn('Qt.openUrlExternally("https://github.com/amnezia-vpn/desktop-client/releases/latest")', about_page)
        self.assertIn("UpdateController.checkForUpdates()", about_page)
        self.assertIn("manualUpdateCheckNoUpdates", update_ui_controller_h)
        self.assertIn("isUpdateCheckRunning()", update_controller_h)
        self.assertIn("bool checkForUpdates()", update_controller_h)
        self.assertIn("updateCheckFinished(updateAvailable)", update_controller)
        self.assertIn("wasUpdateCheckRunning", update_ui_controller)
        self.assertIn("onUpdateCheckFinished(false)", update_ui_controller)
        self.assertIn('QCoreApplication::translate("ConnectionController", "Connected")', connection_ui_controller)
        self.assertNotIn('m_connectionStateText = tr("Connected")', connection_ui_controller)
        self.assertNotIn('"--publish-bundled-updates-once"', qif_component_script)
        self.assertNotIn("Published bundled self-hosted updates", qif_component_script)
        self.assertIn("remote_tmp=$(mktemp -d /tmp/amnezia-client-updates.XXXXXX) && ", bootstrapper)
        self.assertNotIn('set -eu\\n"\n                                                                   "remote_tmp=', bootstrapper)
        self.assertIn("[ValidateSet(\"windows\", \"linux\", \"android\")]", local_release)
        self.assertIn('"windows-x64"', local_release)
        self.assertIn('"linux-x64"', local_release)
        self.assertIn('"android-arm64-v8a"', local_release)
        self.assertNotIn('"android-armeabi-v7a"', local_release)
        self.assertNotIn('"android-x86"', local_release)
        self.assertNotIn('"android-x86_64"', local_release)
        self.assertNotIn('"ios"', local_release)
        self.assertNotIn('"macos"', local_release)
        self.assertIn("androidManifestAttribute(\"versionCode\")", android_gradle)
        self.assertIn("androidManifestAttribute(\"versionName\")", android_gradle)
        self.assertIn("deploy\\build.bat", local_release)
        self.assertIn("--installer ifw -arch x64", local_release)
        self.assertNotIn("--installer all -arch x64", local_release)
        self.assertIn("run_repo_build_sh --source", local_release)
        self.assertIn("[int] $BuildJobs = 0", local_release)
        self.assertIn('[string] $SyncHost = $(if ($env:SELFHOSTED_UPDATE_SYNC_HOST)', local_release)
        self.assertIn("Resolve-BuildJobs", local_release)
        self.assertIn("Assert-ReleaseInputs", local_release)
        self.assertIn("export AMNEZIA_BUILD_JOBS=", local_release)
        self.assertIn("export CMAKE_BUILD_PARALLEL_LEVEL=", local_release)
        self.assertIn("export MAKEFLAGS=", local_release)
        self.assertIn("export SELFHOSTED_UPDATE_SYNC_HOST=$(Quote-Sh $SyncHost)", local_release)
        self.assertIn("Get-RequiredAndroidBuildToolsRevision", local_release)
        self.assertIn("export GRADLE_OPTS=", local_release)
        self.assertIn("--jobs $buildJobs", local_release)
        self.assertIn("run_repo_build_sh --target android --sign --abi arm64-v8a", local_release)
        self.assertIn("--build `\"`$build_dir`\" --jobs $buildJobs", local_release)
        self.assertNotIn("run_repo_build_sh --target android --sign --aab", local_release)
        self.assertNotIn("build-android-universal", local_release)
        self.assertIn('Join-Path $RepoRoot "deploy\\build-android-arm64-v8a"', local_release)
        self.assertIn('$env:CONAN_NO_REMOTE = "1"', local_release)
        self.assertIn('export AWG_ANDROID_GRADLE_USER_HOME="$HOME/.cache/amnezia/awg-android-gradle"', local_release)
        self.assertIn('find "$HOME/.conan2/p/t" -mindepth 1 -maxdepth 1 -exec rm -rf {} +', local_release)
        self.assertIn("rename_artifact()", local_release)
        self.assertIn("Missing fresh Android artifact", local_release)
        self.assertIn("AmneziaVPN_*_android9+_arm64-v8a.apk", local_release)
        self.assertIn("Remove-UnsupportedAndroidArtifacts", local_release)
        self.assertIn("tr -d '\\r' < \"$source_script\"", local_release)
        self.assertIn("Android local auto-update builds require", local_release)
        self.assertIn("[switch] $Preflight", local_release)
        self.assertIn("[switch] $NoBundleUpdatesInWindowsClient", local_release)
        self.assertIn("Build-WindowsInstaller $OutDir", local_release)
        self.assertIn("dist\\selfhosted-windows-client\\$Version", local_release)
        self.assertIn("windows_x64_selfhosted.exe", local_release)
        self.assertIn("SELFHOSTED_UPDATE_BUNDLE_DIR", local_release)
        self.assertIn("Assert-LocalReleasePrerequisites", local_release)
        self.assertIn("Assert-WslReady", local_release)
        self.assertIn("GetTempFileName", local_release)
        self.assertIn('Invoke-External "wsl.exe" @("bash", $tempScriptWsl)', local_release)
        self.assertIn("[System.Text.UTF8Encoding]::new($false)", local_release)
        self.assertIn('export PATH="$HOME/.local/jdk-17/bin:$HOME/.local/bin:$PATH"', local_release)
        self.assertIn('Assert-WslCommand "conan"', local_release)
        self.assertIn("[string] $WslAndroidHome", local_release)
        self.assertIn("Resolve-WslAndroidHome", local_release)
        self.assertIn("Assert-WslAndroidSdkReady", local_release)
        self.assertIn("Assert-WslQifReady", local_release)
        self.assertIn("WSL_QIF_ROOT_PATH", local_release)
        self.assertIn("linux-x86_64/bin/clang", local_release)
        self.assertIn("Qt6RemoteObjects", local_release)
        self.assertIn("Qt6RemoteObjectsTools", local_release)
        self.assertIn("Qt6Core5Compat", local_release)
        self.assertIn("qtremoteobjects", local_release)
        self.assertIn("Assert-JavaForWsl", local_release)
        self.assertIn("Assert-AndroidQtKit", local_release)
        self.assertIn('Test-QtTargetKit $QtRootPath "android"', local_release)
        self.assertIn("Install either '$QtRootPath\\android' or '$QtRootPath\\android_arm64_v8a'", local_release)
        self.assertIn("Test-WindowsJavaHome", local_release)
        self.assertIn("java_shim_dir", local_release)
        self.assertIn("windows_java_home", local_release)
        self.assertIn("$androidExportScript = $androidExports -join", local_release)
        self.assertIn("Ensure-WslJava", setup_release)
        self.assertIn('Assert-ExistingFile $env:QT_ANDROID_KEYSTORE_PATH "QT_ANDROID_KEYSTORE_PATH"', local_release)
        self.assertIn('Assert-QtTargetKit $qtRootPath "gcc_64"', local_release)
        self.assertIn('"android_arm64_v8a"', local_release)
        self.assertIn("export QT_ROOT_PATH=", local_release)
        self.assertIn("export QIF_ROOT_PATH=", local_release)
        self.assertIn("[switch] $InstallMissing", setup_release)
        self.assertIn("[switch] $GenerateUpdateKeys", setup_release)
        self.assertIn("[switch] $GenerateAndroidKeystore", setup_release)
        self.assertIn("$QtMirrorBase", setup_release)
        self.assertIn("QT_MIRROR_BASE", setup_release)
        self.assertIn("-b $(Quote-Sh $QtMirrorBase)", setup_release)
        self.assertIn("python3 -m aqt install-qt", setup_release)
        self.assertIn("--timeout 30", setup_release)
        self.assertIn("Test-AqtQtVersionAvailable $QtHost $Target", setup_release)
        self.assertIn("Test-AqtQtArchAvailable $QtHost $Target $aqtArch", setup_release)
        self.assertIn("Ensure-Conan", setup_release)
        self.assertIn("python3 -m pip install --user conan", setup_release)
        self.assertIn("Ensure-WslInstallerFramework", setup_release)
        self.assertIn("qt.tools.ifw.47", setup_release)
        self.assertIn("WSL_QIF_ROOT_PATH", setup_release)
        self.assertIn("[string] $WslAndroidHome", setup_release)
        self.assertIn("commandlinetools-linux-14742923_latest.zip", setup_release)
        self.assertIn("Ensure-WslAndroidSdk", setup_release)
        self.assertIn('"android-36"', setup_release)
        self.assertIn('"36.0.0"', setup_release)
        self.assertIn("[string] $BaseUrl", setup_release)
        self.assertIn("SELFHOSTED_UPDATE_BASE_URL", setup_release)
        self.assertIn("WSL_ANDROID_HOME", setup_release)
        self.assertIn("$QtAndroidModules", setup_release)
        self.assertIn("qtremoteobjects", setup_release)
        self.assertIn("qt5compat", setup_release)
        self.assertIn("Qt6RemoteObjectsTools", setup_release)
        self.assertIn("Qt6Core5Compat", setup_release)
        self.assertIn('if ($KitName -eq "gcc_64")', setup_release)
        self.assertIn('return "linux_gcc_64"', setup_release)
        self.assertIn("Ensure-AndroidQtKits", setup_release)
        self.assertIn('Ensure-QtKit "all_os" "android" "android_arm64_v8a"', setup_release)
        self.assertIn("Test-AndroidQtKit", setup_release)
        self.assertIn("aqtinstall cannot install Qt", setup_release)
        self.assertIn('Ensure-QtKit "linux" "desktop" "gcc_64"', setup_release)
        self.assertNotIn('"android_arm64_v8a", "android_armv7", "android_x86", "android_x86_64"', setup_release)
        self.assertIn("WslJdkUrl", setup_release)
        self.assertIn("~/.local/jdk-17", setup_release)
        self.assertIn("genpkey -algorithm Ed25519", setup_release)
        self.assertIn("GenerateAndroidKeystore", readme)
        self.assertIn("MaintenanceTool", readme)
        self.assertIn("all_os android", readme)
        self.assertIn("QT_MIRROR_BASE", readme)
        self.assertIn("keytool -genkeypair", setup_release)
        self.assertIn("android-release-keystore.env.ps1", setup_release)
        self.assertNotIn("macos", setup_release.lower())
        self.assertNotIn("ios", setup_release.lower())
        self.assertIn("publish_release.py", local_release)
        self.assertIn("[switch] $Publish", local_release)
        self.assertIn("--auto-install", local_release)
        self.assertIn("--public-key-base64", local_release)
        self.assertIn("SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64", local_release)
        self.assertIn("SELFHOSTED_UPDATE_BASE_URL", local_release)
        self.assertIn("SELFHOSTED_UPDATE_SERVER", local_release)
        self.assertIn("selfhosted-windows-client", readme)
        self.assertIn("selfhosted_updates", readme)
        self.assertIn("recursive package", readme)
        self.assertIn("SelfHostedUpdateBootstrapper", core_controller)
        self.assertNotIn("m_selfHostedUpdateBootstrapper->start();", core_controller)
        self.assertIn("state != Vpn::ConnectionState::Connected", core_signal_handlers)
        self.assertIn("m_coreController->m_selfHostedUpdateBootstrapper->start()", core_signal_handlers)
        self.assertIn("SelfHostedUpdateBootstrapper::publishFinished", core_signal_handlers)
        self.assertIn("scheduleUpdateCheck();", core_signal_handlers)
        self.assertIn("return;\n        }\n#endif\n\n        QTimer::singleShot(5000, m_coreController->m_updateController", core_signal_handlers)
        self.assertIn("bool start()", bootstrapper_h)
        self.assertIn("m_publishScheduled", bootstrapper_h)
        self.assertIn("m_publishInProgress", bootstrapper_h)
        self.assertIn("publishFinished(success)", bootstrapper)
        self.assertIn("bool publishNow()", bootstrapper_h)
        self.assertIn("return publishPayload(payload, credentials);", bootstrapper)
        self.assertIn("installOrRefreshUpdateHost", bootstrapper)
        self.assertIn("verifyRemoteUpdateHost", bootstrapper)
        self.assertIn("Remote self-hosted update host verified", bootstrapper)
        self.assertIn("verifyManifestSignature", bootstrapper)
        self.assertIn("fileSha256ByName", bootstrapper_h)
        self.assertIn("mktemp -d /tmp/amnezia-client-updates.XXXXXX", bootstrapper)
        self.assertIn("sudo docker exec amnezia-client-updates", bootstrapper)
        self.assertIn("container_manifest_sha256", bootstrapper)
        self.assertIn("--network host", bootstrapper)
        self.assertIn("host_manifest_sha256", bootstrapper)
        self.assertIn("docker.io/library/busybox:1.36.1", bootstrapper)
        self.assertNotIn("docker.io/library/busybox:latest", bootstrapper)
        self.assertLess(
            bootstrapper.index("if (!installOrRefreshUpdateHost())"),
            bootstrapper.index("Bundled self-hosted update payload is already published"),
        )
        self.assertIn("publish-bundled-updates-once", app_cpp)
        self.assertIn("m_optPublishBundledUpdatesOnce", app_h)
        self.assertIn("isPublishBundledUpdatesOnceCommand", main_cpp)
        self.assertIn("!publishBundledUpdatesOnce", main_cpp)
        self.assertIn("uploadLocalFileToHost", ssh_session_h)
        self.assertIn("scpFileCopy(overwriteMode, localPath, remotePath", ssh_session_cpp)
        self.assertIn("qint64 localFileSize", ssh_client_cpp := (REPO_ROOT / "client/core/utils/selfhosted/sshClient.cpp").read_text(encoding="utf-8"))
        self.assertIn("std::numeric_limits<size_t>::max()", ssh_client_cpp)
        self.assertIn("static_cast<size_t>(localFileSize)", ssh_client_cpp)
        self.assertIn("ssh_channel_get_exit_status", ssh_client_cpp)
        self.assertIn("ErrorCode::ServerCheckFailed", ssh_client_cpp)
        self.assertIn("update_host/install_server_update_host.sh", server_scripts_qrc)
        self.assertIn('QStringLiteral("windows-x64")', bootstrapper)
        self.assertIn('QStringLiteral("linux-x64")', bootstrapper)
        self.assertIn('QStringLiteral("android-arm64-v8a")', bootstrapper)
        self.assertIn("SELFHOSTED_BUNDLED_UPDATE_PAYLOAD_DIR", bootstrapper)
        self.assertIn("QCoreApplication::applicationDirPath()", bootstrapper)
        self.assertIn("selfhosted_updates", bootstrapper)
        self.assertIn("QUrl::FullyDecoded", bootstrapper)
        self.assertIn("Bundled update artifact size mismatch", bootstrapper)
        self.assertIn("Bundled update artifact sha256 mismatch", bootstrapper)
        self.assertIn("isSha256Hex", bootstrapper)
        self.assertIn("uploadLocalFileToHost(credentials, filePath, remotePath)", bootstrapper)
        self.assertIn("manifest.json.tmp", bootstrapper)
        self.assertIn("sha256sum", bootstrapper)
        self.assertIn("hostDirectory", bootstrapper)
        self.assertIn("install_server_update_host.sh", bootstrapper)
        if deploy_workflow:
            self.assertNotIn("needs.Build-iOS.result == 'success'", deploy_workflow)
            self.assertNotIn("needs.Build-MacOS.result == 'success'", deploy_workflow)
            self.assertNotIn("args+=(--require-platform ios)", deploy_workflow)
            for job in ("Linux", "Windows", "Android"):
                bake_job = f"Bake-Prebuilts-{job}"
                self.assertIn(
                    f"(needs.{bake_job}.result == 'success' || needs.{bake_job}.result == 'skipped')",
                    deploy_workflow,
                )
        if freeze_workflow:
            self.assertNotIn("PUBLISH_SELFHOSTED_UPDATES", freeze_workflow)
            self.assertNotIn("RUN_BUILD_AFTER_FREEZE", freeze_workflow)
            self.assertIn('gh api "repos/${UPSTREAM_REPO}/releases/latest"', freeze_workflow)
            self.assertNotIn("git ls-remote --tags --refs upstream", freeze_workflow)
            self.assertNotIn("git tag -l | grep", freeze_workflow)
            self.assertIn("action == 'wait'", freeze_workflow)
            self.assertIn("Ordinary upstream/dev commits are intentionally not merged between releases", freeze_workflow)
            self.assertNotIn("git merge --no-edit upstream/dev", freeze_workflow)
            self.assertNotIn("--action sync", freeze_workflow)
            self.assertIn("git diff --binary upstream/dev...HEAD", freeze_workflow)
            self.assertIn('git checkout -B "$TARGET_BRANCH" "refs/tags/${release_tag}"', freeze_workflow)
            self.assertIn('git apply --index --3way "$fork_patch"', freeze_workflow)
            self.assertIn("apply server-managed fork changes to upstream release", freeze_workflow)
            self.assertIn('git push --force-with-lease origin HEAD:"$TARGET_BRANCH"', freeze_workflow)
            self.assertIn('git rev-parse "refs/tags/${release_tag}^{commit}"', freeze_workflow)
            self.assertNotIn("gh workflow run deploy.yml", freeze_workflow)
            self.assertIn("local_release.ps1 -Version", freeze_workflow)
        self.assertIn("latest published upstream GitHub Release", readme)
        self.assertIn("an upstream tag alone is not enough", readme)
        self.assertIn("post-release `upstream/dev` commits are not retained", readme)

    def test_windows_split_tunnel_does_not_route_empty_peer_endpoints(self) -> None:
        wg_windows = (REPO_ROOT / "client/platforms/windows/daemon/wireguardutilswindows.cpp").read_text(encoding="utf-8")
        route_monitor = (REPO_ROOT / "client/platforms/windows/daemon/windowsroutemonitor.cpp").read_text(encoding="utf-8")
        route_monitor_h = (REPO_ROOT / "client/platforms/windows/daemon/windowsroutemonitor.h").read_text(encoding="utf-8")
        split_tunnel = (REPO_ROOT / "client/platforms/windows/daemon/windowssplittunnel.cpp").read_text(encoding="utf-8")
        windows_daemon = (REPO_ROOT / "client/platforms/windows/daemon/windowsdaemon.cpp").read_text(encoding="utf-8")
        router_win = (REPO_ROOT / "service/server/router_win.cpp").read_text(encoding="utf-8")
        vpn_connection = (REPO_ROOT / "client/vpnConnection.cpp").read_text(encoding="utf-8")

        self.assertIn("if (!config.m_serverIpv4AddrIn.isEmpty())", wg_windows)
        self.assertIn("if (!config.m_serverIpv6AddrIn.isEmpty())", wg_windows)
        self.assertNotIn("addExclusionRoute(IPAddress(config.m_serverIpv6AddrIn));\n  }", wg_windows)
        self.assertIn("addr.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol", route_monitor)
        self.assertIn("prefix.address().protocol() == QAbstractSocket::UnknownNetworkLayerProtocol", route_monitor)
        self.assertIn("if (error == NO_ERROR) {\n    updateCapturedRoutes(family, table);", route_monitor)
        self.assertIn("isOnLinkRoute(row)", route_monitor)
        self.assertIn("ERROR_OBJECT_ALREADY_EXISTS", route_monitor)
        self.assertIn("MIB_IPPROTO_LOCAL", route_monitor)
        self.assertIn("Adopting existing captured route", route_monitor)
        self.assertIn("NotifyRouteChange2(AF_UNSPEC", route_monitor)
        self.assertIn("m_routeChangeTimer.setInterval(300)", route_monitor)
        self.assertIn("std::atomic_bool m_routeChangeQueued", route_monitor_h)
        self.assertIn("std::atomic_int m_pendingRouteChanges", route_monitor_h)
        self.assertIn("notifyRouteChanged", route_monitor)
        self.assertIn("m_routeChangeQueued.compare_exchange_strong", route_monitor)
        self.assertIn("if (!m_routeChangeTimer.isActive())", route_monitor)
        self.assertIn("m_pendingRouteChanges.exchange(0", route_monitor)
        self.assertIn("m_exclusionRoutes[prefix] = data", route_monitor)
        self.assertIn("coalesced notifications", route_monitor)
        self.assertIn("QDir::fromNativeSeparators", split_tunnel)
        self.assertIn("if (dosPaths.isEmpty())", split_tunnel)
        self.assertIn("sizeof(CONFIGURATION_ENTRY) * dosPaths.size()", split_tunnel)
        self.assertIn("header->NumEntries = dosPaths.size()", split_tunnel)
        self.assertIn("if (config.empty())", split_tunnel)
        self.assertIn("std::numeric_limits<USHORT>::max()", split_tunnel)
        self.assertIn("GetLastError() == ERROR_INSUFFICIENT_BUFFER", split_tunnel)
        self.assertIn("m_splitTunnelManager->stop();\n      return true;", windows_daemon.replace("\r\n", "\n"))
        self.assertIn("isRouteAddCandidate", router_win)
        self.assertIn("address.isMulticast()", router_win)
        self.assertIn("minPublicBypassPrefixLength = 16", router_win)
        self.assertIn("minLocalBypassPrefixLength = 24", router_win)
        self.assertIn("MIB_IPFORWARDROW ipfrow = {}", router_win)
        self.assertIn("routeCandidates.size() > 500", router_win)
        self.assertIn("trackManagedRoute", router_win)
        self.assertIn("m_ipForwardRows.insert(routeKey, row)", router_win)
        self.assertIn("DeleteIpForwardEntry(&existing)", router_win)
        self.assertIn("routableSplitTunnelRoutes", vpn_connection)
        self.assertIn("hostAddress.isMulticast()", vpn_connection)
        self.assertIn("minPublicBypassPrefixLength = 16", vpn_connection)
        self.assertIn("minLocalBypassPrefixLength = 24", vpn_connection)
        self.assertIn("splitRoutesKeepingHostsInVpn(ips, protectedHosts)", vpn_connection)
        self.assertIn("if (!reply.waitForFinished() || !reply.returnValue())", vpn_connection)

    def test_windows_dns_prefers_vpn_interface_metric(self) -> None:
        dns_utils = (REPO_ROOT / "client/platforms/windows/daemon/dnsutilswindows.cpp").read_text(encoding="utf-8")
        dns_utils_h = (REPO_ROOT / "client/platforms/windows/daemon/dnsutilswindows.h").read_text(encoding="utf-8")

        self.assertIn("VPN_DNS_INTERFACE_METRIC = 1", dns_utils)
        self.assertIn("preferInterfaceMetric(AF_INET, m_ipv4Metric)", dns_utils)
        self.assertIn("preferInterfaceMetric(AF_INET6, m_ipv6Metric)", dns_utils)
        self.assertIn("row.UseAutomaticMetric = false", dns_utils)
        self.assertIn("SetIpInterfaceEntry(&row)", dns_utils)
        self.assertIn("restoreInterfaceMetric(AF_INET, m_ipv4Metric)", dns_utils)
        self.assertIn("restoreInterfaceMetric(AF_INET6, m_ipv6Metric)", dns_utils)
        self.assertIn("InterfaceMetricState", dns_utils_h)

    def test_site_split_rejects_broad_and_special_bypass_routes(self) -> None:
        router_win = (REPO_ROOT / "service/server/router_win.cpp").read_text(encoding="utf-8")
        vpn_connection = (REPO_ROOT / "client/vpnConnection.cpp").read_text(encoding="utf-8")

        for source in (router_win, vpn_connection):
            self.assertIn("minPublicBypassPrefixLength = 16", source)
            self.assertIn("minLocalBypassPrefixLength = 24", source)
            self.assertIn("routeOverlapsIpv4Range", source)
            self.assertIn("routeOverlapsRange", source)
            self.assertIn("localOrServiceRoute", source)
            self.assertIn("? minLocalBypassPrefixLength", source)
            self.assertIn(": minPublicBypassPrefixLength", source)
            self.assertIn("prefixLength >= minPrefixLength", source)
            self.assertIn("inRange(0x0a000000u, 8)", source)
            self.assertIn("inRange(0xac100000u, 12)", source)
            self.assertIn("inRange(0xc0a80000u, 16)", source)
            self.assertIn("inRange(0x64400000u, 10)", source)
            self.assertIn("routeOverlapsRange(0xc01f0000u, 24)", source)
            self.assertIn("routeOverlapsRange(0xc01fc400u, 24)", source)
            self.assertIn("routeOverlapsRange(0xc034c100u, 24)", source)
            self.assertIn("routeOverlapsRange(0xc0586300u, 24)", source)
            self.assertIn("routeOverlapsRange(0xc0af3000u, 24)", source)
            self.assertIn("routeOverlapsRange(0xc6336400u, 24)", source)
            self.assertIn("routeOverlapsRange(0xcb007100u, 24)", source)
            self.assertIn("routeOverlapsRange(0xe0000000u, 4)", source)
            self.assertIn("routeOverlapsRange(0xf0000000u, 4)", source)

        self.assertIn("enum class SplitTunnelRouteSource", vpn_connection)
        self.assertIn("SplitTunnelRouteSource::Client && !isRoutableSplitTunnelRoute(route)", vpn_connection)
        self.assertIn("SplitTunnelRouteSource::ServerManaged", vpn_connection)
        self.assertIn("iface->routeAddTrustedList(gw, managedIps)", vpn_connection)
        self.assertIn("managedVpnSitesForRouting(activeServerIndex, mode)", vpn_connection)
        self.assertIn("managedVpnSitesForRouting(activeServerIndex, routeMode)", vpn_connection)

        ipc_interface = (REPO_ROOT / "ipc/ipc_interface.rep").read_text(encoding="utf-8")
        ipc_server = (REPO_ROOT / "ipc/ipcserver.cpp").read_text(encoding="utf-8")
        router = (REPO_ROOT / "service/server/router.cpp").read_text(encoding="utf-8")
        self.assertIn("routeAddTrustedList", ipc_interface)
        self.assertIn("Router::routeAddTrustedList", ipc_server)
        self.assertIn("RouterWin::Instance().routeAddTrustedList(gw, ips)", router)
        self.assertIn("validateRoutes && !isRouteAddCandidate(ipWithMask)", router_win)
        self.assertIn("skipping invalid trusted split route", router_win)

    def test_selfhosted_release_documents_own_monotonic_versioning(self) -> None:
        cmake = (REPO_ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        readme = (REPO_ROOT / "deploy/selfhosted_updates/README.md").read_text(encoding="utf-8")

        self.assertIn("set(AMNEZIAVPN_VERSION 4.9.0.8)", cmake)
        self.assertIn("set(APP_ANDROID_VERSION_CODE 2129)", cmake)
        self.assertIn("own monotonically increasing app version", readme)
        self.assertIn("never update backward to an older fork release", readme)

    def test_windows_split_tunnel_creates_driver_dns_sublayer(self) -> None:
        firewall = (REPO_ROOT / "client/platforms/windows/daemon/windowsfirewall.cpp").read_text(encoding="utf-8")
        split_tunnel = (REPO_ROOT / "client/platforms/windows/daemon/windowssplittunnel.cpp").read_text(encoding="utf-8")

        self.assertIn("win-split-tunnel v1.2.5.0 uses this hardcoded DNS sublayer key", firewall)
        self.assertIn("0x60090787, 0xcca1, 0x4937", firewall)
        self.assertIn("Amnezia-SplitTunnel-DNS-Sublayer", firewall)
        self.assertIn("L\"Filters that enforce DNS handling\", 0xFFFE", firewall)
        self.assertIn("FwpmEngineClose0(engineHandle)", firewall)
        self.assertIn("blockTrafficOnPort(53, MED_WEIGHT, \"Block all DNS\",\n                           ST_FW_WINFW_DNS_SUBLAYER_KEY)", firewall)
        self.assertIn("FWP_E_SUBLAYER_NOT_FOUND", firewall)
        self.assertIn("baselineExists && dnsExists", firewall)
        self.assertIn("IOCTL_INITIALIZE CTL_CODE(0x8000, 1, METHOD_NEITHER", split_tunnel)
        self.assertIn("DeviceIoControl(driverIO, IOCTL_INITIALIZE, nullptr, 0, nullptr, 0", split_tunnel)

    def test_deploy_upload_artifacts_have_stable_names(self) -> None:
        deploy_workflow = read_workflow_if_enabled(REPO_ROOT / ".github/workflows/deploy.yml")
        tag_deploy_workflow = read_workflow_if_enabled(REPO_ROOT / ".github/workflows/tag-deploy.yml")
        if not deploy_workflow and not tag_deploy_workflow:
            return
        required_names = {
            "AmneziaVPN_linux_x64_run",
            "AmneziaVPN_windows_x64_msi",
            "AmneziaVPN_windows_x64_exe",
            "AmneziaVPN_android_universal_apk",
            "AmneziaVPN_android_aab",
            "AmneziaVPN_android_arm64-v8a_apk",
            "AmneziaVPN_android_armeabi-v7a_apk",
            "AmneziaVPN_android_x86_apk",
            "AmneziaVPN_android_x86_64_apk",
        }
        for name in required_names:
            self.assertIn(f"name: {name}", deploy_workflow)
        self.assertIn("uses: actions/upload-artifact@v7", tag_deploy_workflow)
        self.assertIn("name: AmneziaVPN_android_release_apk", tag_deploy_workflow)
        self.assertIn("archive: false", tag_deploy_workflow)
        self.assertNotIn("uses: actions/upload-artifact@v3", deploy_workflow + tag_deploy_workflow)
        self.assertNotIn("uses: actions/checkout@v3", tag_deploy_workflow)
        self.assertNotIn("uses: actions/setup-java@v3", tag_deploy_workflow)

        lines = deploy_workflow.splitlines()
        for index, line in enumerate(lines):
            if "uses: actions/upload-artifact@v7" not in line:
                continue
            block = "\n".join(lines[index:index + 8])
            self.assertIn("name:", block, f"upload-artifact@v7 step at line {index + 1} must set a unique name")
        for name in required_names:
            index = next(index for index, line in enumerate(lines) if f"name: {name}" in line)
            block = "\n".join(lines[index:index + 8])
            self.assertIn("if-no-files-found: error", block, f"{name} upload must fail when the expected artifact is missing")

        tag_lines = tag_deploy_workflow.splitlines()
        tag_index = next(index for index, line in enumerate(tag_lines) if "name: AmneziaVPN_android_release_apk" in line)
        tag_block = "\n".join(tag_lines[tag_index:tag_index + 8])
        self.assertIn("if-no-files-found: error", tag_block)

    def test_publish_upload_switches_manifest_last(self) -> None:
        files_command = publish_release.publish_files_remote_command(
            "/opt/amnezia/client-updates",
            "/tmp/amnezia-client-updates-9.9.9.9-123",
        )
        manifest_command = publish_release.publish_manifest_remote_command(
            "/opt/amnezia/client-updates",
            "/tmp/amnezia-client-updates-9.9.9.9-123",
        )
        self.assertNotIn("rm -rf '/opt/amnezia/client-updates/files'", files_command)
        self.assertIn('"$name.tmp"', files_command)
        self.assertNotIn("manifest.json", files_command)
        manifest_copy = "sudo cp -a '/tmp/amnezia-client-updates-9.9.9.9-123/manifest.json' '/opt/amnezia/client-updates/manifest.json.tmp'"
        manifest_switch = "sudo mv -f '/opt/amnezia/client-updates/manifest.json.tmp' '/opt/amnezia/client-updates/manifest.json'"
        self.assertIn(manifest_copy, manifest_command)
        self.assertIn(manifest_switch, manifest_command)
        self.assertLess(manifest_command.index(manifest_copy), manifest_command.index(manifest_switch))

    def test_publish_upload_validates_host_before_manifest_switch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            out_dir = Path(tmp) / "out"
            (out_dir / "files").mkdir(parents=True)
            (out_dir / "manifest.json").write_text("{}", encoding="utf-8")
            calls: list[tuple[list[str], Path | None]] = []

            original_run = publish_release.run
            try:
                publish_release.run = lambda command, stdin_path=None: calls.append((command, stdin_path))  # type: ignore[assignment]
                args = type(
                    "Args",
                    (),
                    {
                        "version": "9.9.9.9",
                        "server": "root@example.invalid",
                        "server_dir": "/opt/amnezia/client-updates",
                        "ssh": "ssh -i key",
                        "scp": "scp -i key",
                        "no_install_host": False,
                    },
                )()
                publish_release.upload_release(args, out_dir)
            finally:
                publish_release.run = original_run

            rendered = [" ".join(command) for command, _stdin_path in calls]
            files_call = next(index for index, command in enumerate(rendered) if "for f in" in command)
            install_host_call = next(index for index, (command, stdin_path) in enumerate(calls) if stdin_path == publish_release.INSTALL_HOST)
            manifest_call = next(index for index, command in enumerate(rendered) if "manifest.json.tmp" in command)
            self.assertLess(files_call, install_host_call)
            self.assertLess(install_host_call, manifest_call)

    def test_update_host_setup_rejects_route_values_for_bridge_host(self) -> None:
        script = (REPO_ROOT / "deploy/selfhosted_updates/install_server_update_host.sh").read_text(encoding="utf-8")
        self.assertIn('HOST_DIRECTORY must be an absolute path', script)
        self.assertIn('AMNEZIA_UPDATE_BRIDGE_HOST must be a single IPv4 address, not a CIDR route', script)
        self.assertIn('AMNEZIA_UPDATE_HOST_BIND must be a single IPv4 address', script)
        self.assertIn('AMNEZIA_UPDATE_HOST_CONTAINER_NAME must not be empty', script)
        self.assertIn('is_ipv4_address "$BRIDGE_HOST"', script)
        self.assertIn('is_ipv4_address "$HOST_BIND"', script)
        self.assertIn('case "$candidate" in', script)
        self.assertIn('""|*/*)', script)
        self.assertIn('is_port "$SYNC_PORT"', script)
        self.assertIn('AMNEZIA_UPDATE_PUBLISH_HOST_PORT must be 0 or 1', script)
        self.assertIn('EXPECTED_SUBNET="172.29.172.0/24"', script)
        self.assertIn('NETWORK_NAME="${CONTAINER_NAME}-net"', script)
        self.assertIn('AUTO_VPN_CONTAINERS="amnezia-awg2 amnezia-awg amnezia-wireguard amnezia-openvpn"', script)
        self.assertIn('AMNEZIA_UPDATE_VPN_CONTAINER must name a running VPN container', script)
        self.assertIn('--network "container:$VPN_CONTAINER"', script)
        self.assertIn("wait_http_ready()", script)
        self.assertIn("wait_host_http_ready()", script)
        self.assertIn("open_host_firewall_port()", script)
        self.assertIn("ufw allow", script)
        self.assertIn("firewall-cmd --add-port", script)
        self.assertIn("iptables -C INPUT -p tcp --dport", script)
        self.assertIn("iptables -I INPUT -p tcp --dport", script)
        self.assertIn("--network host", script)
        self.assertIn("busybox wget -S -O /dev/null", script)
        self.assertIn("grep -q 'HTTP/'", script)
        self.assertIn("Bridge update endpoint did not become ready", script)
        self.assertIn("Tunnel update endpoint did not become ready", script)
        self.assertIn("Host update endpoint did not become ready", script)
        self.assertIn("docker.io/library/busybox:1.36.1", script)
        self.assertNotIn("busybox:latest", script)
        self.assertIn('sudo docker rm -f "$HOST_CONTAINER_NAME"', script)
        self.assertIn('grep "^${CONTAINER_NAME}-vpn-"', script)

    @unittest.skipUnless(find_sh(), "sh is required to exercise the update host setup script")
    def test_update_host_setup_validates_inputs_before_docker_run(self) -> None:
        sh = find_sh()
        assert sh
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            bin_dir = tmp_path / "bin"
            bin_dir.mkdir()
            log_path = tmp_path / "docker.log"
            host_dir = tmp_path / "updates"
            host_dir_arg = shell_absolute_path(host_dir)

            (bin_dir / "sudo").write_text(
                "#!/bin/sh\nexec \"$@\"\n",
                encoding="utf-8",
            )
            (bin_dir / "docker").write_text(
                textwrap.dedent(
                    f"""\
                    #!/bin/sh
                    printf '%s\\n' "$*" >> {log_path.as_posix()!r}
                    if [ "$1" = "network" ] && [ "$2" = "inspect" ]; then
                        if [ "$3" = "-f" ]; then
                            printf '%s\\n' "172.29.172.0/24"
                        fi
                        exit 0
                    fi
                    if [ "$1" = "image" ] && [ "$2" = "inspect" ]; then
                        exit 0
                    fi
                    if [ "$1" = "ps" ]; then
                        if [ -n "$AMNEZIA_FAKE_RUNNING_CONTAINERS" ]; then
                            for name in $AMNEZIA_FAKE_RUNNING_CONTAINERS; do
                                printf '%s\\n' "$name"
                            done
                        else
                            printf '%s\\n' "amnezia-client-updates"
                            printf '%s\\n' "amnezia-client-updates-host"
                        fi
                        exit 0
                    fi
                    exit 0
                    """
                ),
                encoding="utf-8",
            )
            os.chmod(bin_dir / "sudo", 0o755)
            os.chmod(bin_dir / "docker", 0o755)

            env = os.environ.copy()
            env["PATH"] = str(bin_dir) + os.pathsep + env.get("PATH", "")

            relative_host_dir = subprocess.run(
                [sh, str(REPO_ROOT / "deploy/selfhosted_updates/install_server_update_host.sh"), "relative-updates"],
                env=env,
                text=True,
                capture_output=True,
            )
            self.assertNotEqual(relative_host_dir.returncode, 0)
            self.assertIn("HOST_DIRECTORY must be an absolute path", relative_host_dir.stderr)
            self.assertFalse(log_path.exists(), "relative host directory must fail before any docker command is called")

            env["AMNEZIA_UPDATE_BRIDGE_HOST"] = "10.8.1.0/1"
            invalid = subprocess.run(
                [sh, str(REPO_ROOT / "deploy/selfhosted_updates/install_server_update_host.sh"), host_dir_arg],
                env=env,
                text=True,
                capture_output=True,
            )
            self.assertNotEqual(invalid.returncode, 0)
            self.assertIn("must be a single IPv4 address", invalid.stderr)
            self.assertFalse(log_path.exists(), "CIDR bridge host must fail before any docker command is called")

            env["AMNEZIA_UPDATE_BRIDGE_HOST"] = "172.29.172.252"
            env["AMNEZIA_UPDATE_HOST_BIND"] = "0.0.0.0 --privileged"
            invalid_bind = subprocess.run(
                [sh, str(REPO_ROOT / "deploy/selfhosted_updates/install_server_update_host.sh"), host_dir_arg],
                env=env,
                text=True,
                capture_output=True,
            )
            self.assertNotEqual(invalid_bind.returncode, 0)
            self.assertIn("AMNEZIA_UPDATE_HOST_BIND must be a single IPv4 address", invalid_bind.stderr)
            self.assertFalse(log_path.exists(), "invalid host bind must fail before any docker command is called")

            env["AMNEZIA_UPDATE_HOST_BIND"] = "0.0.0.0"
            env["AMNEZIA_UPDATE_VPN_CONTAINER"] = "missing-vpn"
            missing_vpn = subprocess.run(
                [sh, str(REPO_ROOT / "deploy/selfhosted_updates/install_server_update_host.sh"), host_dir_arg],
                env=env,
                text=True,
                capture_output=True,
            )
            self.assertNotEqual(missing_vpn.returncode, 0)
            self.assertIn("must name a running VPN container", missing_vpn.stderr)
            self.assertNotIn("--ip 172.29.172.252", log_path.read_text(encoding="utf-8"))

            env.pop("AMNEZIA_UPDATE_VPN_CONTAINER")
            valid = subprocess.run(
                [sh, str(REPO_ROOT / "deploy/selfhosted_updates/install_server_update_host.sh"), host_dir_arg],
                env=env,
                text=True,
                capture_output=True,
            )
            self.assertEqual(valid.returncode, 0, valid.stderr)
            docker_log = log_path.read_text(encoding="utf-8")
            self.assertIn("--ip 172.29.172.252", docker_log)
            self.assertNotIn("-p 0.0.0.0:17865:17865", docker_log)
            self.assertIn("--name amnezia-client-updates-host", docker_log)
            self.assertIn("exec amnezia-client-updates sh -c", docker_log)
            self.assertIn("http://127.0.0.1:17865/", docker_log)
            self.assertIn("--network host", docker_log)
            self.assertTrue((host_dir / "files").is_dir())

            log_path.unlink()
            env["AMNEZIA_FAKE_RUNNING_CONTAINERS"] = "amnezia-awg amnezia-client-updates amnezia-client-updates-host amnezia-client-updates-vpn-amnezia-awg"
            host_dir_with_vpn = tmp_path / "updates-with-vpn"
            host_dir_with_vpn_arg = shell_absolute_path(host_dir_with_vpn)
            valid_with_vpn = subprocess.run(
                [sh, str(REPO_ROOT / "deploy/selfhosted_updates/install_server_update_host.sh"), host_dir_with_vpn_arg],
                env=env,
                text=True,
                capture_output=True,
            )
            self.assertEqual(valid_with_vpn.returncode, 0, valid_with_vpn.stderr)
            docker_log = log_path.read_text(encoding="utf-8")
            self.assertIn("--network container:amnezia-awg", docker_log)
            self.assertIn("--name amnezia-client-updates-vpn-amnezia-awg", docker_log)
            self.assertIn("exec amnezia-client-updates-vpn-amnezia-awg sh -c", docker_log)


@unittest.skipUnless(find_openssl(), "openssl is required for signed manifest tests")
class ManifestPublisherTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.openssl = find_openssl()
        assert self.openssl
        self.private_key = self.root / "selfhosted-update-private.pem"
        subprocess.run([self.openssl, "genpkey", "-algorithm", "Ed25519", "-out", str(self.private_key)], check=True)
        self.public_key = self.root / "selfhosted-update-public.pem"
        subprocess.run([self.openssl, "pkey", "-in", str(self.private_key), "-pubout", "-out", str(self.public_key)], check=True)
        self.public_key_base64 = base64.b64encode(self.public_key.read_bytes()).decode("ascii")
        self.previous_path = os.environ.get("PATH", "")
        os.environ["PATH"] = str(Path(self.openssl).parent) + os.pathsep + self.previous_path
        self.env = os.environ.copy()

    def tearDown(self) -> None:
        os.environ["PATH"] = self.previous_path
        self.tmp.cleanup()

    def write_artifact(self, name: str, contents: bytes = b"dummy artifact") -> Path:
        artifact_dir = self.root / "artifacts"
        artifact_dir.mkdir(exist_ok=True)
        artifact = artifact_dir / name
        artifact.write_bytes(contents)
        return artifact

    def test_publish_rejects_mismatched_public_key(self) -> None:
        version = "9.9.9.9"
        self.write_artifact(f"AmneziaVPN_{version}_windows_x64.exe", b"windows")
        other_private_key = self.root / "other-private.pem"
        other_public_key = self.root / "other-public.pem"
        subprocess.run([self.openssl, "genpkey", "-algorithm", "Ed25519", "-out", str(other_private_key)], check=True)
        subprocess.run([self.openssl, "pkey", "-in", str(other_private_key), "-pubout", "-out", str(other_public_key)], check=True)

        result = subprocess.run(
            [
                sys.executable,
                str(SCRIPT_DIR / "publish_release.py"),
                "--version",
                version,
                "--private-key",
                str(self.private_key),
                "--public-key-base64",
                base64.b64encode(other_public_key.read_bytes()).decode("ascii"),
                "--artifact-dir",
                str(self.root / "artifacts"),
                "--out-dir",
                str(self.root / "out-mismatched-key"),
                "--base-url",
                "http://172.29.172.252:17865",
                "--require-platform",
                "windows-x64",
            ],
            env=self.env,
            text=True,
            capture_output=True,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("does not match SELFHOSTED_UPDATE_PRIVATE_KEY", result.stderr + result.stdout)

    def test_server_publish_requires_public_key(self) -> None:
        version = "9.9.9.9"
        self.write_artifact(f"AmneziaVPN_{version}_windows_x64.exe", b"windows")

        result = subprocess.run(
            [
                sys.executable,
                str(SCRIPT_DIR / "publish_release.py"),
                "--version",
                version,
                "--private-key",
                str(self.private_key),
                "--artifact-dir",
                str(self.root / "artifacts"),
                "--out-dir",
                str(self.root / "out-server-no-public-key"),
                "--base-url",
                "http://172.29.172.252:17865",
                "--require-platform",
                "windows-x64",
                "--server",
                "root@example.invalid",
            ],
            env={**self.env, "SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64": ""},
            text=True,
            capture_output=True,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("public-key-base64", result.stderr + result.stdout)

    def test_publish_validates_explicit_artifacts_before_clearing_out_dir(self) -> None:
        version = "9.9.9.9"
        out_dir = self.root / "existing-out"
        out_dir.mkdir()
        sentinel = out_dir / "keep.txt"
        sentinel.write_text("old release", encoding="utf-8")

        result = subprocess.run(
            [
                sys.executable,
                str(SCRIPT_DIR / "publish_release.py"),
                "--version",
                version,
                "--private-key",
                str(self.private_key),
                "--artifact",
                f"windows-x64={self.root / 'missing.exe'}",
                "--out-dir",
                str(out_dir),
                "--base-url",
                "http://172.29.172.252:17865",
            ],
            env=self.env,
            text=True,
            capture_output=True,
        )

        self.assertNotEqual(result.returncode, 0)
        self.assertTrue(sentinel.is_file())
        self.assertIn("Explicit update artifact does not exist", result.stderr + result.stdout)

    @unittest.skipUnless(find_powershell(), "PowerShell is required for the local release wrapper smoke test")
    def test_local_release_wrapper_verifies_local_non_apple_artifacts(self) -> None:
        powershell = find_powershell()
        assert powershell
        version = "9.9.9.9"
        for name in (
            f"AmneziaVPN_{version}_windows_x64.exe",
            f"AmneziaVPN_{version}_linux_x64.run",
            f"AmneziaVPN_{version}_android9+_arm64-v8a.apk",
            f"AmneziaVPN_{version}_android9+_universal.apk",
            f"AmneziaVPN_{version}_android9+_armeabi-v7a.apk",
            f"AmneziaVPN_{version}_android9+_x86.apk",
            f"AmneziaVPN_{version}_android9+_x86_64.apk",
        ):
            self.write_artifact(name, f"artifact-{name}".encode("utf-8"))

        out_dir = self.root / "local-release-out"
        result = subprocess.run(
            [
                powershell,
                "-NoProfile",
                "-ExecutionPolicy",
                "Bypass",
                "-File",
                str(SCRIPT_DIR / "local_release.ps1"),
                "-Version",
                version,
                "-SkipBuild",
                "-NoPublish",
                "-NoBundleUpdatesInWindowsClient",
                "-ArtifactDir",
                str(self.root / "artifacts"),
                "-OutDir",
                str(out_dir),
                "-BaseUrl",
                "http://172.29.172.252:17865",
                "-PrivateKey",
                str(self.private_key),
                "-PublicKeyBase64",
                self.public_key_base64,
            ],
            env=self.env,
            text=True,
            capture_output=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        self.assertIn("Verified self-hosted update manifest signature and required platforms", result.stdout)
        payload = manifest_payload(out_dir / "manifest.json")
        self.assertEqual(
            set(payload["platforms"]),
            {
                "windows-x64",
                "linux-x64",
                "android-arm64-v8a",
            },
        )
        self.assertNotIn("ios", payload["platforms"])
        self.assertNotIn("macos-x64", payload["platforms"])
        self.assertNotIn("android", payload["platforms"])
        self.assertNotIn("android-armeabi-v7a", payload["platforms"])
        self.assertNotIn("android-x86", payload["platforms"])
        self.assertNotIn("android-x86_64", payload["platforms"])
        for platform, artifact in payload["platforms"].items():
            self.assertTrue(artifact["url"].startswith("files/"), platform)
            self.assertNotIn("172.29.172.252", artifact["url"])
        for stale_name in (
            f"AmneziaVPN_{version}_android9+_universal.apk",
            f"AmneziaVPN_{version}_android9+_armeabi-v7a.apk",
            f"AmneziaVPN_{version}_android9+_x86.apk",
            f"AmneziaVPN_{version}_android9+_x86_64.apk",
        ):
            self.assertFalse((self.root / "artifacts" / stale_name).exists())

    def test_publish_include_platform_filters_stale_autodiscovered_artifacts(self) -> None:
        version = "9.9.9.9"
        for name in (
            f"AmneziaVPN_{version}_windows_x64.exe",
            f"AmneziaVPN_{version}_linux_x64.run",
            f"AmneziaVPN_{version}_android9+_arm64-v8a.apk",
            f"AmneziaVPN_{version}_android9+_universal.apk",
            f"AmneziaVPN_{version}_android9+_armeabi-v7a.apk",
            f"AmneziaVPN_{version}_android9+_x86.apk",
            f"AmneziaVPN_{version}_android9+_x86_64.apk",
        ):
            self.write_artifact(name, f"artifact-{name}".encode("utf-8"))
        out_dir = self.root / "out-include-platform"

        result = subprocess.run(
            [
                sys.executable,
                str(SCRIPT_DIR / "publish_release.py"),
                "--version",
                version,
                "--private-key",
                str(self.private_key),
                "--public-key-base64",
                self.public_key_base64,
                "--artifact-dir",
                str(self.root / "artifacts"),
                "--out-dir",
                str(out_dir),
                "--base-url",
                "http://172.29.172.252:17865",
                "--require-platform",
                "windows-x64",
                "--require-platform",
                "linux-x64",
                "--require-platform",
                "android-arm64-v8a",
                "--include-platform",
                "windows-x64",
                "--include-platform",
                "linux-x64",
                "--include-platform",
                "android-arm64-v8a",
                "--auto-install",
            ],
            env=self.env,
            text=True,
            capture_output=True,
        )

        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        payload = manifest_payload(out_dir / "manifest.json")
        self.assertEqual(set(payload["platforms"]), {"windows-x64", "linux-x64", "android-arm64-v8a"})
        for artifact in payload["platforms"].values():
            self.assertTrue(artifact["url"].startswith("files/"))
        self.assertIn("Published manifest platforms: android-arm64-v8a, linux-x64, windows-x64", result.stdout)

    @unittest.skipUnless(find_powershell(), "PowerShell is required for the local release wrapper smoke test")
    def test_local_release_preflight_validates_local_inputs_without_building(self) -> None:
        powershell = find_powershell()
        assert powershell

        out_dir = self.root / "preflight-out"
        result = subprocess.run(
            [
                powershell,
                "-NoProfile",
                "-ExecutionPolicy",
                "Bypass",
                "-File",
                str(SCRIPT_DIR / "local_release.ps1"),
                "-Version",
                "9.9.9.9",
                "-Preflight",
                "-NoPublish",
                "-BuildPlatform",
                "windows",
                "-OutDir",
                str(out_dir),
                "-BaseUrl",
                "http://172.29.172.252:17865",
                "-PrivateKey",
                str(self.private_key),
                "-PublicKeyBase64",
                self.public_key_base64,
            ],
            env=self.env,
            text=True,
            capture_output=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        self.assertIn("Preflight OK", result.stdout)
        self.assertFalse((out_dir / "manifest.json").exists())

    @unittest.skipUnless(find_powershell() and find_wsl(), "PowerShell and WSL are required for the Linux local preflight smoke test")
    def test_local_release_linux_preflight_converts_windows_paths_for_wsl(self) -> None:
        powershell = find_powershell()
        assert powershell

        qt_root = self.root / "Qt" / "6.99.0"
        qt_toolchain = qt_root / "gcc_64" / "lib" / "cmake" / "Qt6" / "qt.toolchain.cmake"
        qt_toolchain.parent.mkdir(parents=True)
        qt_toolchain.write_text("# fake qt toolchain\n", encoding="utf-8")
        for module in ("Qt6RemoteObjects", "Qt6Core5Compat"):
            module_config = qt_root / "gcc_64" / "lib" / "cmake" / module / f"{module}Config.cmake"
            module_config.parent.mkdir(parents=True)
            module_config.write_text("# fake module\n", encoding="utf-8")
        qif_root = self.root / "Qt" / "Tools" / "QtInstallerFramework" / "9.9"
        (qif_root / "bin").mkdir(parents=True)
        (qif_root / "bin" / "binarycreator").write_text("#!/bin/sh\n", encoding="utf-8")
        wsl_qif_root = to_wsl_path(qif_root)

        result = subprocess.run(
            [
                powershell,
                "-NoProfile",
                "-ExecutionPolicy",
                "Bypass",
                "-File",
                str(SCRIPT_DIR / "local_release.ps1"),
                "-Version",
                "9.9.9.9",
                "-Preflight",
                "-NoPublish",
                "-BuildPlatform",
                "linux",
                "-BaseUrl",
                "http://172.29.172.252:17865",
                "-PrivateKey",
                str(self.private_key),
                "-PublicKeyBase64",
                self.public_key_base64,
            ],
            env={
                **self.env,
                "QT_ROOT_PATH": str(qt_root / "gcc_64"),
                "QIF_ROOT_PATH": str(qif_root),
                "WSL_QIF_ROOT_PATH": wsl_qif_root,
            },
            text=True,
            capture_output=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        self.assertIn("Preflight OK", result.stdout)
        self.assertNotIn("Failed to convert path to WSL path", result.stderr + result.stdout)

    @unittest.skipUnless(find_powershell() and find_wsl(), "PowerShell and WSL are required for the Android local preflight smoke test")
    def test_local_release_android_preflight_accepts_single_qt6_android_kit(self) -> None:
        powershell = find_powershell()
        assert powershell

        qt_root = self.root / "Qt" / "6.99.0"
        for kit in ("gcc_64", "android"):
            toolchain = qt_root / kit / "lib" / "cmake" / "Qt6" / "qt.toolchain.cmake"
            toolchain.parent.mkdir(parents=True)
            toolchain.write_text("# fake qt toolchain\n", encoding="utf-8")
        host_tools = qt_root / "gcc_64" / "lib" / "cmake" / "Qt6RemoteObjectsTools" / "Qt6RemoteObjectsToolsConfig.cmake"
        host_tools.parent.mkdir(parents=True)
        host_tools.write_text("# fake remote objects tools\n", encoding="utf-8")
        host_core5 = qt_root / "gcc_64" / "lib" / "cmake" / "Qt6Core5Compat" / "Qt6Core5CompatConfig.cmake"
        host_core5.parent.mkdir(parents=True)
        host_core5.write_text("# fake core5 compat\n", encoding="utf-8")
        for module in ("Qt6RemoteObjects", "Qt6Core5Compat"):
            module_config = qt_root / "android" / "lib" / "cmake" / module / f"{module}Config.cmake"
            module_config.parent.mkdir(parents=True)
            module_config.write_text("# fake module\n", encoding="utf-8")
        android_home = self.root / "Android" / "Sdk"
        (android_home / "ndk" / "26.1.10909125").mkdir(parents=True)
        build_tools = android_home / "build-tools" / "36.0.0"
        build_tools.mkdir(parents=True)
        (build_tools / "apksigner").write_text("# fake apksigner\n", encoding="utf-8")
        linux_toolchain = android_home / "ndk" / "26.1.10909125" / "toolchains" / "llvm" / "prebuilt" / "linux-x86_64" / "bin"
        linux_toolchain.mkdir(parents=True)
        for compiler in ("clang", "clang++"):
            compiler_path = linux_toolchain / compiler
            compiler_path.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
            subprocess.run([find_wsl() or "wsl.exe", "chmod", "+x", to_wsl_path(compiler_path)], check=True)
        keystore = self.root / "android-release.keystore"
        keystore.write_bytes(b"fake keystore")

        result = subprocess.run(
            [
                powershell,
                "-NoProfile",
                "-ExecutionPolicy",
                "Bypass",
                "-File",
                str(SCRIPT_DIR / "local_release.ps1"),
                "-Version",
                "9.9.9.9",
                "-Preflight",
                "-NoPublish",
                "-BuildPlatform",
                "android",
                "-BaseUrl",
                "http://172.29.172.252:17865",
                "-PrivateKey",
                str(self.private_key),
                "-PublicKeyBase64",
                self.public_key_base64,
                "-WslAndroidHome",
                to_wsl_path(android_home),
            ],
            env={
                **self.env,
                "QT_ROOT_PATH": str(qt_root),
                "ANDROID_HOME": str(android_home),
                "QT_ANDROID_KEYSTORE_PATH": str(keystore),
                "QT_ANDROID_KEYSTORE_ALIAS": "release",
                "QT_ANDROID_KEYSTORE_STORE_PASS": "password",
                "JAVA_HOME": os.environ.get("JAVA_HOME", ""),
            },
            text=True,
            encoding="utf-8",
            errors="replace",
            capture_output=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        self.assertIn("Preflight OK", result.stdout)

    @unittest.skipUnless(find_powershell() and find_wsl(), "PowerShell and WSL are required for the setup smoke test")
    def test_setup_release_workstation_generates_update_keys_and_env_file(self) -> None:
        powershell = find_powershell()
        assert powershell

        qt_install_dir = self.root / "Qt"
        qt_version = "6.99.0"
        for kit in ("gcc_64", "android_arm64_v8a", "android_armv7", "android_x86", "android_x86_64"):
            toolchain = qt_install_dir / qt_version / kit / "lib" / "cmake" / "Qt6" / "qt.toolchain.cmake"
            toolchain.parent.mkdir(parents=True)
            toolchain.write_text("# fake qt toolchain\n", encoding="utf-8")
        android_home = self.root / "Android" / "Sdk"
        (android_home / "ndk" / "26.1.10909125").mkdir(parents=True)
        build_tools = android_home / "build-tools" / "36.0.0"
        build_tools.mkdir(parents=True)
        (build_tools / "apksigner").write_text("# fake apksigner\n", encoding="utf-8")
        key_dir = self.root / "keys"
        env_file = self.root / "release-env.ps1"

        result = subprocess.run(
            [
                powershell,
                "-NoProfile",
                "-ExecutionPolicy",
                "Bypass",
                "-File",
                str(SCRIPT_DIR / "setup_release_workstation.ps1"),
                "-QtInstallDir",
                str(qt_install_dir),
                "-QtVersion",
                qt_version,
                "-AndroidHome",
                str(android_home),
                "-KeyDir",
                str(key_dir),
                "-EnvFile",
                str(env_file),
                "-GenerateUpdateKeys",
            ],
            env=self.env,
            text=True,
            encoding="utf-8",
            errors="replace",
            capture_output=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        self.assertTrue((key_dir / "selfhosted-update-private.pem").is_file())
        self.assertTrue((key_dir / "selfhosted-update-public.pem").is_file())
        env_text = env_file.read_text(encoding="utf-8-sig")
        self.assertIn("SELFHOSTED_UPDATE_PRIVATE_KEY_PATH", env_text)
        self.assertIn("SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64", env_text)
        self.assertIn("SELFHOSTED_UPDATE_SYNC_HOST = '10.8.1.0'", env_text)
        self.assertIn("QT_ANDROID_KEYSTORE_PATH", env_text)
        self.assertNotIn("keytool -genkeypair", result.stdout + result.stderr + env_text)

    def test_publish_autodetects_ios_ipa_and_sets_auto_install(self) -> None:
        version = "9.9.9.9"
        self.write_artifact(f"AmneziaVPN_{version}_windows_x64.exe", b"windows")
        self.write_artifact(f"AmneziaVPN_{version}_ios.ipa", b"ios ipa")
        out_dir = self.root / "out"

        subprocess.run(
            [
                sys.executable,
                str(SCRIPT_DIR / "publish_release.py"),
                "--version",
                version,
                "--release-date",
                "2026-06-06",
                "--private-key",
                str(self.private_key),
                "--public-key-base64",
                self.public_key_base64,
                "--artifact-dir",
                str(self.root / "artifacts"),
                "--out-dir",
                str(out_dir),
                "--base-url",
                "https://updates.example.invalid",
                "--require-platform",
                "windows-x64",
                "--require-platform",
                "ios",
                "--auto-install",
                "--no-install-host",
            ],
            env=self.env,
            check=True,
            text=True,
            capture_output=True,
        )

        payload = manifest_payload(out_dir / "manifest.json")
        self.assertEqual(payload["schema"], 1)
        self.assertTrue(payload["autoInstall"])
        platforms = payload["platforms"]
        self.assertIn("windows-x64", platforms)
        self.assertIn("ios", platforms)
        self.assertTrue(platforms["windows-x64"]["url"].startswith("files/"))
        self.assertTrue(platforms["ios"]["openExternal"])
        self.assertTrue(platforms["ios"]["autoInstall"])
        self.assertTrue(platforms["ios"]["url"].startswith("itms-services://"))
        plist_path = out_dir / "files" / f"AmneziaVPN_{version}_ios.plist"
        self.assertTrue(plist_path.is_file())
        plist_payload = plistlib.loads(plist_path.read_bytes())
        self.assertEqual(
            plist_payload["items"][0]["metadata"]["bundle-version"],
            "9.9.9",
        )

        publish_release.verify_manifest(out_dir / "manifest.json", self.private_key, version, {"windows-x64", "ios"}, True)

        manifest = json.loads((out_dir / "manifest.json").read_text(encoding="utf-8"))
        payload_bytes = base64.urlsafe_b64decode(manifest["payload"] + "=" * (-len(manifest["payload"]) % 4))
        payload = json.loads(payload_bytes.decode("utf-8"))
        del payload["platforms"]["windows-x64"]["sha256"]
        tampered_payload = json.dumps(payload, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")
        manifest["payload"] = base64.urlsafe_b64encode(tampered_payload).decode("ascii").rstrip("=")
        tampered_manifest = out_dir / "missing-sha-manifest.json"
        tampered_manifest.write_text(json.dumps(manifest), encoding="utf-8")
        with self.assertRaises(SystemExit) as missing_sha:
            publish_release.verify_manifest(tampered_manifest, self.private_key, version, {"windows-x64", "ios"}, True)
        self.assertIn("windows-x64 is missing or has invalid sha256", str(missing_sha.exception))

        payload["platforms"]["windows-x64"]["sha256"] = "z" * 64
        tampered_payload = json.dumps(payload, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")
        manifest["payload"] = base64.urlsafe_b64encode(tampered_payload).decode("ascii").rstrip("=")
        tampered_manifest.write_text(json.dumps(manifest), encoding="utf-8")
        with self.assertRaises(SystemExit) as invalid_sha:
            publish_release.verify_manifest(tampered_manifest, self.private_key, version, {"windows-x64", "ios"}, True)
        self.assertIn("windows-x64 is missing or has invalid sha256", str(invalid_sha.exception))

        payload["platforms"]["windows-x64"]["sha256"] = sha256_hex_for_text("windows")
        payload["platforms"]["windows-x64"]["url"] = "file:///tmp/AmneziaVPN.exe"
        tampered_payload = json.dumps(payload, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")
        manifest["payload"] = base64.urlsafe_b64encode(tampered_payload).decode("ascii").rstrip("=")
        tampered_manifest.write_text(json.dumps(manifest), encoding="utf-8")
        with self.assertRaises(SystemExit) as invalid_url_scheme:
            publish_release.verify_manifest(tampered_manifest, self.private_key, version, {"windows-x64", "ios"}, True)
        self.assertIn("windows-x64 URL must use http(s)", str(invalid_url_scheme.exception))

        payload["platforms"]["windows-x64"]["url"] = "https://updates.example.invalid/files/AmneziaVPN.exe"
        payload["version"] = "0.0.0.1"
        tampered_payload = json.dumps(payload, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")
        manifest["payload"] = base64.urlsafe_b64encode(tampered_payload).decode("ascii").rstrip("=")
        tampered_manifest.write_text(json.dumps(manifest), encoding="utf-8")
        with self.assertRaises(SystemExit) as wrong_version:
            publish_release.verify_manifest(tampered_manifest, self.private_key, version, {"windows-x64", "ios"}, True)
        self.assertIn("does not match requested version", str(wrong_version.exception))

        payload["version"] = version
        payload["platforms"]["android"] = {
            "url": "file:///tmp/AmneziaVPN.apk",
            "openExternal": True,
            "autoInstall": True,
        }
        tampered_payload = json.dumps(payload, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")
        manifest["payload"] = base64.urlsafe_b64encode(tampered_payload).decode("ascii").rstrip("=")
        tampered_manifest.write_text(json.dumps(manifest), encoding="utf-8")
        with self.assertRaises(SystemExit) as android_external_file:
            publish_release.verify_manifest(tampered_manifest, self.private_key, version, {"windows-x64", "ios"}, True)
        self.assertIn("android external URL has unsupported scheme", str(android_external_file.exception))

        payload["platforms"].pop("android")
        payload["platforms"]["ios"]["url"] = (
            "itms-services://?action=download-manifest&url="
            "http%3A%2F%2F172.29.172.252%3A17865%2Ffiles%2FAmneziaVPN.plist"
        )
        tampered_payload = json.dumps(payload, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")
        manifest["payload"] = base64.urlsafe_b64encode(tampered_payload).decode("ascii").rstrip("=")
        tampered_manifest.write_text(json.dumps(manifest), encoding="utf-8")
        with self.assertRaises(SystemExit) as ios_http_itms:
            publish_release.verify_manifest(tampered_manifest, self.private_key, version, {"windows-x64", "ios"}, True)
        self.assertIn("ios external URL has unsupported scheme", str(ios_http_itms.exception))

    def test_ios_bundle_version_validation(self) -> None:
        self.assertEqual(make_manifest.ios_bundle_version("4.8.16.0"), "4.8.16")
        self.assertEqual(make_manifest.ios_bundle_version("04.08.016"), "4.8.16")
        with self.assertRaises(SystemExit) as explicit_four_part:
            make_manifest.ios_bundle_version("4.8.16.0", explicit=True)
        self.assertIn("one to three numeric components", str(explicit_four_part.exception))
        with self.assertRaises(SystemExit) as non_numeric:
            make_manifest.ios_bundle_version("4.8.beta", explicit=True)
        self.assertIn("only digits and periods", str(non_numeric.exception))

    def test_publish_fails_when_required_platform_is_missing(self) -> None:
        version = "9.9.9.9"
        self.write_artifact(f"AmneziaVPN_{version}_windows_x64.exe", b"windows")

        result = subprocess.run(
            [
                sys.executable,
                str(SCRIPT_DIR / "publish_release.py"),
                "--version",
                version,
                "--private-key",
                str(self.private_key),
                "--artifact-dir",
                str(self.root / "artifacts"),
                "--out-dir",
                str(self.root / "out"),
                "--base-url",
                "https://updates.example.invalid",
                "--require-platform",
                "windows-x64",
                "--require-platform",
                "linux-x64",
                "--no-install-host",
            ],
            env=self.env,
            text=True,
            capture_output=True,
        )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Missing required update artifacts/settings: linux-x64", result.stderr)

    def test_publish_accepts_external_ios_without_ipa(self) -> None:
        version = "9.9.9.9"
        self.write_artifact(f"AmneziaVPN_{version}_windows_x64.exe", b"windows")
        out_dir = self.root / "out-external-ios"

        subprocess.run(
            [
                sys.executable,
                str(SCRIPT_DIR / "publish_release.py"),
                "--version",
                version,
                "--release-date",
                "2026-06-06",
                "--private-key",
                str(self.private_key),
                "--artifact-dir",
                str(self.root / "artifacts"),
                "--out-dir",
                str(out_dir),
                "--base-url",
                "http://172.29.172.252:17865",
                "--require-platform",
                "windows-x64",
                "--require-platform",
                "ios",
                "--external",
                "ios=itms-apps://apps.apple.com/app/id123456789",
                "--auto-install",
                "--no-install-host",
            ],
            env=self.env,
            check=True,
            text=True,
            capture_output=True,
        )

        payload = manifest_payload(out_dir / "manifest.json")
        platforms = payload["platforms"]
        self.assertIn("windows-x64", platforms)
        self.assertIn("ios", platforms)
        self.assertTrue(platforms["windows-x64"]["url"].startswith("files/"))
        self.assertEqual(platforms["ios"]["url"], "itms-apps://apps.apple.com/app/id123456789")
        self.assertTrue(platforms["ios"]["openExternal"])
        self.assertTrue(platforms["ios"]["autoInstall"])
        self.assertFalse((out_dir / "files" / f"AmneziaVPN_{version}_ios.plist").exists())
        publish_release.verify_manifest(out_dir / "manifest.json", self.private_key, version, {"windows-x64", "ios"}, True)

    def test_publish_full_default_platform_manifest_with_external_ios(self) -> None:
        version = "9.9.9.10"
        required_platforms = {
            "windows-x64",
            "linux-x64",
            "macos-x64",
            "ios",
            "android-arm64-v8a",
            "android-armeabi-v7a",
            "android-x86",
            "android-x86_64",
        }
        for platform in required_platforms - {"ios"}:
            pattern = publish_release.KNOWN_PATTERNS[platform]
            self.write_artifact(pattern.format(version=version), platform.encode("utf-8"))
        out_dir = self.root / "out-full-defaults"

        command = [
            sys.executable,
            str(SCRIPT_DIR / "publish_release.py"),
            "--version",
            version,
            "--release-date",
            "2026-06-06",
            "--private-key",
            str(self.private_key),
            "--artifact-dir",
            str(self.root / "artifacts"),
            "--out-dir",
            str(out_dir),
            "--base-url",
            "http://172.29.172.252:17865",
            "--external",
            "ios=itms-apps://apps.apple.com/app/id123456789",
            "--auto-install",
            "--no-install-host",
        ]
        for platform in sorted(required_platforms):
            command += ["--require-platform", platform]

        subprocess.run(
            command,
            env=self.env,
            check=True,
            text=True,
            capture_output=True,
        )

        payload = manifest_payload(out_dir / "manifest.json")
        self.assertEqual(set(payload["platforms"]), required_platforms)
        for platform in required_platforms - {"ios"}:
            self.assertTrue(payload["platforms"][platform]["url"].startswith("files/"))
            self.assertEqual(payload["platforms"][platform]["sha256"], sha256_hex_for_text(platform))
            self.assertGreater(payload["platforms"][platform]["size"], 0)
            self.assertTrue((out_dir / "files" / publish_release.KNOWN_PATTERNS[platform].format(version=version)).is_file())
        self.assertTrue(payload["platforms"]["ios"]["openExternal"])
        self.assertTrue(payload["platforms"]["ios"]["autoInstall"])
        publish_release.verify_manifest(out_dir / "manifest.json", self.private_key, version, required_platforms, True)

    def test_discovers_installable_upstream_release_asset_aliases(self) -> None:
        version = "4.8.15.4"
        self.write_artifact(f"AmneziaVPN_{version}_x64.exe", b"windows")
        self.write_artifact(f"AmneziaVPN_{version}_macos.pkg", b"macos")
        self.write_artifact(f"AmneziaVPN_{version}_linux_x64.tar", b"not a linux auto-installer")
        self.write_artifact(f"AmneziaVPN_{version}_android9+_arm64-v8a.apk", b"android")

        artifacts = publish_release.discover_artifacts(self.root / "artifacts", version)

        self.assertEqual(artifacts["windows-x64"].name, f"AmneziaVPN_{version}_x64.exe")
        self.assertEqual(artifacts["macos-x64"].name, f"AmneziaVPN_{version}_macos.pkg")
        self.assertEqual(artifacts["android-arm64-v8a"].name, f"AmneziaVPN_{version}_android9+_arm64-v8a.apk")
        self.assertNotIn("linux-x64", artifacts)

    def test_linux_tar_release_asset_has_actionable_missing_platform_message(self) -> None:
        version = "4.8.15.4"
        self.write_artifact(f"AmneziaVPN_{version}_x64.exe", b"windows")
        self.write_artifact(f"AmneziaVPN_{version}_linux_x64.tar", b"not a linux auto-installer")

        result = subprocess.run(
            [
                sys.executable,
                str(SCRIPT_DIR / "publish_release.py"),
                "--version",
                version,
                "--private-key",
                str(self.private_key),
                "--artifact-dir",
                str(self.root / "artifacts"),
                "--out-dir",
                str(self.root / "out-linux-tar"),
                "--base-url",
                "https://updates.example.invalid",
                "--require-platform",
                "windows-x64",
                "--require-platform",
                "linux-x64",
                "--no-install-host",
            ],
            env=self.env,
            text=True,
            capture_output=True,
        )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("found AmneziaVPN_4.8.15.4_linux_x64.tar", result.stderr)
        self.assertIn("Linux auto-install requires the fork CI .run artifact", result.stderr)

    def test_external_and_explicit_platforms_do_not_require_release_asset_download(self) -> None:
        required = publish_release.required_release_asset_platforms(
            ["windows-x64", "linux-x64", "ios"],
            {"ios"},
            {"linux-x64"},
        )

        self.assertEqual(required, {"windows-x64"})


if __name__ == "__main__":
    unittest.main()
