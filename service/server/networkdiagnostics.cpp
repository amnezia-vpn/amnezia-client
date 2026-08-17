#include "networkdiagnostics.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTextStream>

namespace {

constexpr int kProcessTimeoutMsec = 30000;
const QString kSnapshotLabel = QStringLiteral("diagnostics");

#if defined(Q_OS_WIN)
const QStringList kSectionOrder = { "system",          "adapters",           "link-type",
                                     "drivers-system",  "drivers-store",      "drivers-net-class",
                                     "routes",          "route-resolution",   "dns",
                                     "ipv6-localhost",  "proxy",              "services-processes",
                                     "firewall",        "wfp-filters" };
#elif defined(Q_OS_LINUX)
const QStringList kSectionOrder = { "system",     "adapters",           "link-type",     "drivers",
                                     "routes",     "route-resolution",   "dns",           "ipv6-localhost",
                                     "proxy",      "services-processes", "firewall",      "netfilter-full" };
#endif

} // namespace

QString NetworkDiagnostics::run()
{
#if !defined(Q_OS_WIN) && !defined(Q_OS_LINUX)
    return QStringLiteral("ERROR: network diagnostics is not supported on this platform");
#else
    // QTemporaryDir() with no template creates a randomly-named, owner-only
    // (0700) directory under the system temp path. Do NOT switch this to a
    // fixed/predictable path: this code runs as root/SYSTEM, and a predictable
    // path that a local non-privileged user could pre-create or symlink ahead
    // of time is a classic TOCTOU/symlink privilege-escalation vector.
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        return QStringLiteral("ERROR: failed to create secure temp directory");
    }

#if defined(Q_OS_WIN)
    const QString resourcePath = QStringLiteral(":/network_diagnostics/network-diagnostics-windows.ps1");
    const QString scriptPath = tempDir.filePath(QStringLiteral("network-diagnostics-windows.ps1"));
#else
    const QString resourcePath = QStringLiteral(":/network_diagnostics/network-diagnostics-linux.sh");
    const QString scriptPath = tempDir.filePath(QStringLiteral("network-diagnostics-linux.sh"));
#endif

    if (!QFile::copy(resourcePath, scriptPath)) {
        return QStringLiteral("ERROR: failed to extract diagnostics script");
    }
    // No chmod +x: the script is always invoked through an explicit
    // interpreter below, so it never needs its own execute bit.

    QProcess process;
    process.setWorkingDirectory(tempDir.path()); // both scripts write ./snapshot-<label>/ relative to CWD

#if defined(Q_OS_WIN)
    process.start(QStringLiteral("powershell"),
                   { "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-WindowStyle", "Hidden",
                     "-File", scriptPath, "-Label", kSnapshotLabel });
#else
    process.start(QStringLiteral("/bin/bash"), { scriptPath, "--label", kSnapshotLabel });
#endif

    if (!process.waitForStarted(5000)) {
        return QStringLiteral("ERROR: failed to start diagnostics script (%1)").arg(process.errorString());
    }
    if (!process.waitForFinished(kProcessTimeoutMsec)) {
        process.kill();
        process.waitForFinished(3000);
        return QStringLiteral("ERROR: diagnostics script timed out after %1 s").arg(kProcessTimeoutMsec / 1000);
    }

    const QString snapshotDir = tempDir.filePath(QStringLiteral("snapshot-%1").arg(kSnapshotLabel));
    QString combined;
    QTextStream out(&combined);
    int sectionsFound = 0;
    for (const QString &section : kSectionOrder) {
        QFile sectionFile(QStringLiteral("%1/%2.txt").arg(snapshotDir, section));
        if (!sectionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            out << "=== " << section << " === (section missing)\n\n";
            continue;
        }
        out << QString::fromUtf8(sectionFile.readAll()) << "\n";
        sectionsFound++;
    }

    if (sectionsFound == 0) {
        return QStringLiteral("ERROR: diagnostics script produced no output (exit code %1)").arg(process.exitCode());
    }

    return combined;
    // tempDir's destructor recursively removes the directory (script + snapshot output).
#endif
}
